#include "live555.h"
#include "fmp4_muxer.h"
#include "fmp4_server.h" 
#include <boost/date_time/posix_time/posix_time.hpp>

UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
	return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
	return env << subsession.mediumName() << "/" << subsession.codecName();
}

void usage(UsageEnvironment& env, char const* progName) {
	env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
	env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}


class DummySink : public MediaSink {
public:
	static DummySink* createNew(UsageEnvironment& env,
		MediaSubsession& subsession,
		MyRTSPClient& rtspClient);

private:
	DummySink(UsageEnvironment& env, MediaSubsession& subsession, MyRTSPClient& rtspClient);
	virtual ~DummySink();

	static void afterGettingFrame(void* clientData, unsigned frameSize,
		unsigned numTruncatedBytes,
		struct timeval presentationTime,
		unsigned durationInMicroseconds);
	void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
		struct timeval presentationTime, unsigned durationInMicroseconds);

private:
	virtual Boolean continuePlaying();

private:
	u_int8_t* fReceiveBuffer = NULL;
	MediaSubsession& fSubsession;
	bool find_first_iframe;
	MyRTSPClient& rtspClient;
};

#define RTSP_CLIENT_VERBOSITY_LEVEL 1

void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL) {

	MyRTSPClient* rtspClient = MyRTSPClient::createNew(env, rtspURL);
	if (rtspClient == NULL) {
		env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
		return;
	}
	rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
}

void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((MyRTSPClient*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
			delete[] resultString;
			break;
		}

		char* const sdpDescription = resultString;
		env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

		scs.session = MediaSession::createNew(env, sdpDescription);
		delete[] sdpDescription;
		if (scs.session == NULL) {
			env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
			break;
		}
		else if (!scs.session->hasSubsessions()) {
			env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
			break;
		}

		scs.iter = new MediaSubsessionIterator(*scs.session);
		setupNextSubsession(rtspClient);
		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	shutdownStream(rtspClient);
}

// 海康不支持UDP，但无法从协议反馈中看出来。
#define REQUEST_STREAMING_OVER_TCP true

void setupNextSubsession(RTSPClient* rtspClient) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((MyRTSPClient*)rtspClient)->scs; // alias

	scs.subsession = scs.iter->next();
	if (scs.subsession != NULL) {
		if (!scs.subsession->initiate()) {
			env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
			setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
		}
		else {
			env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
			if (scs.subsession->rtcpIsMuxed()) {
				env << "client port " << scs.subsession->clientPortNum();
			}
			else {
				env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
			}
			env << ")\n";

			// Continue setting up this subsession, by sending a RTSP "SETUP" command:
			rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
		}
		return;
	}

	if (scs.session->absStartTime() != NULL) {
		rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
	}
	else {
		scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
		rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
	}
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir();
		StreamClientState& scs = ((MyRTSPClient*)rtspClient)->scs;

		if (resultCode != 0) {
			env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
			break;
		}

		env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
		if (scs.subsession->rtcpIsMuxed()) {
			env << "client port " << scs.subsession->clientPortNum();
		}
		else {
			env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
		}
		env << ")\n";

		scs.subsession->sink = DummySink::createNew(env, *scs.subsession, *(MyRTSPClient*)rtspClient);

		if (scs.subsession->sink == NULL) {
			env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
				<< "\" subsession: " << env.getResultMsg() << "\n";
			break;
		}

		env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
		scs.subsession->miscPtr = rtspClient;
		scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
			subsessionAfterPlaying, scs.subsession);

		if (scs.subsession->rtcpInstance() != NULL) {
			scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
		}
	} while (0);
	delete[] resultString;

	setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
	Boolean success = False;

	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((MyRTSPClient*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
			break;
		}
		env << *rtspClient << "Started playing session.\n";
		success = true;
	} while (0);
	delete[] resultString;

	if (!success) {
		shutdownStream(rtspClient);
	}
}

void subsessionAfterPlaying(void* clientData) {
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

	Medium::close(subsession->sink);
	subsession->sink = NULL;

	MediaSession& session = subsession->parentSession();
	MediaSubsessionIterator iter(session);
	while ((subsession = iter.next()) != NULL) {
		if (subsession->sink != NULL) return;
	}
	shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData) {
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
	UsageEnvironment& env = rtspClient->envir(); // alias

	env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";

	subsessionAfterPlaying(subsession);
}

void shutdownStream(RTSPClient* rtspClient) {
	UsageEnvironment& env = rtspClient->envir();
	StreamClientState& scs = ((MyRTSPClient*)rtspClient)->scs;

	if (scs.session != NULL) {
		Boolean someSubsessionsWereActive = False;
		MediaSubsessionIterator iter(*scs.session);
		MediaSubsession* subsession;

		while ((subsession = iter.next()) != NULL) {
			if (subsession->sink != NULL) {
				if (subsession->rtcpInstance() != NULL) {
					subsession->rtcpInstance()->setByeHandler(NULL, NULL);
				}
				Medium::close(subsession->sink);
				subsession->sink = NULL;
				someSubsessionsWereActive = true;
			}
		}

		if (someSubsessionsWereActive) {
			rtspClient->sendTeardownCommand(*scs.session, [](RTSPClient* rtspClient,
				int resultCode, char* resultString) {
					Medium::close(rtspClient);
				});
		}
	}
}


MyRTSPClient* MyRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL) {
	return new MyRTSPClient(env, rtspURL);
}

MyRTSPClient::MyRTSPClient(UsageEnvironment& env, char const* rtspURL) : RTSPClient(env, rtspURL, 1, NULL, 0, -1)
{

}

MyRTSPClient::~MyRTSPClient() {
}

void MyRTSPClient::connect()
{
	sendDescribeCommand(continueAfterDESCRIBE);
}

StreamClientState::StreamClientState()
	: iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState::~StreamClientState() {
	delete iter;
	if (session != NULL) {
		Medium::close(session);
	}
}


#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 1000000

DummySink* DummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession, MyRTSPClient& rtspClient) {
	return new DummySink(env, subsession, rtspClient);
}

DummySink::DummySink(UsageEnvironment& env, MediaSubsession& subsession, MyRTSPClient& rc)
	: MediaSink(env),
	fSubsession(subsession), rtspClient(rc) {
	find_first_iframe = false;
	fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
	memset(fReceiveBuffer, 0, DUMMY_SINK_RECEIVE_BUFFER_SIZE);
}

DummySink::~DummySink() {
	delete[] fReceiveBuffer;
}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
	struct timeval presentationTime, unsigned durationInMicroseconds) {
	DummySink* sink = (DummySink*)clientData;
	sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
	struct timeval presentationTime, unsigned durationInMicroseconds) {

	if (find_first_iframe == false)
	{
		if (fReceiveBuffer[0] == 0x65)
		{
			find_first_iframe = true;
		}
		else
		{
			continuePlaying();
			return;
		}
	}

	if (fReceiveBuffer[0] == 0x67 || fReceiveBuffer[0] == 0x68 || frameSize == 0)
	{
		continuePlaying();
		return;
	}

	if (strcmp(fSubsession.mediumName(), "video") == 0 &&
		(strcmp(fSubsession.codecName(), "H264") == 0))
	{
		try
		{
			//boost::posix_time::ptime t0 = boost::posix_time::microsec_clock::local_time();
			boost::shared_lock<boost::shared_mutex> l(rtspClient.mu);
			for (auto& p : rtspClient.conns) {
				// hdl : p.first
				// muxer: p.second

				if (p.second.frame_number == 0) {
					unsigned int num = 0;
					SPropRecord* spps = NULL;
					spps = parseSPropParameterSets(fSubsession.fmtp_spropparametersets(), num);
					if (num == 2)
					{
						//send header & data
						uint32_t size;
						uint8_t* data;
						FMp4Info fi;
						fi.sps = spps[0].sPropBytes;
						fi.sps_size = spps[0].sPropLength;
						fi.pps = spps[1].sPropBytes;
						fi.pps_size = spps[1].sPropLength;
						size = p.second.generate_ftyp_moov(data, fi);
						rtspClient.fmp4Server->send(p.first, data, size);
						free(data);
					}
					delete[]spps;
				}
				uint32_t size = frameSize;
				uint8_t* data = fReceiveBuffer;
				size = p.second.generate_moof_mdat(data, size);
				rtspClient.fmp4Server->send(p.first, data, size);
				free(data);
			}
			// boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::local_time();
			// std::cout << "date diff: " << t1.time_of_day().total_milliseconds() - t0.time_of_day().total_milliseconds() << "\n";
		}
		catch (const std::exception & ex)
		{
			envir() << "afterGettingFrame: " << ex.what() << "\n";
		}
	}
	else
	{
		envir() << "medium: " << fSubsession.mediumName() << "\n";
		envir() << "codec: " << fSubsession.codecName() << "\n";
	}
	continuePlaying();
}

Boolean DummySink::continuePlaying() {
	if (fSource == NULL) return False;

	fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
		afterGettingFrame, this,
		onSourceClosure, this);
	return true;
}


