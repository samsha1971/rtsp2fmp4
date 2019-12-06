#include "fmp4_server.h"
#include <thread>
#include <iostream>
#include "live555.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <string>
#include <locale>
#include <fstream>

#include <boost/thread/thread.hpp>

#ifdef WIN32
#include <direct.h> 
#else
#include <unistd.h>
#endif

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using namespace websocketpp::http::parser;


#include <cstdlib>
#include <string>

std::wstring s2ws(const std::string& str) {
	if (str.empty()) {
		return L"";
	}
	unsigned len = str.size() + 1;
	setlocale(LC_CTYPE, "zh_CN.UTF-8");
	wchar_t* p = new wchar_t[len];
	mbstowcs(p, str.c_str(), len);
	std::wstring w_str(p);
	delete[] p;
	return w_str;
}

std::string ws2s(const std::wstring& w_str) {
	if (w_str.empty()) {
		return "";
	}
	unsigned len = w_str.size() * 4 + 1;
	setlocale(LC_CTYPE, "zh_CN.UTF-8");
	char* p = new char[len];
	wcstombs(p, w_str.c_str(), len);
	std::string str(p);
	delete[] p;
	return str;
}

std::string getConfig() {

	char db[255];
	std::string path = "";
#ifdef WIN32
	char* t = _getcwd(db, 255);
	path = db;
	path += "\\config.json";
#else
	char* t = getcwd(db, 255);
	path = db;
	path += "/config.json";
#endif

	std::wifstream f(path.data(), std::ifstream::in);
	f.imbue(std::locale("zh_CN.UTF-8"));
	std::wstring ws;
	while (!f.eof())
	{
		wchar_t c;
		f.get(c);
		ws += c;
	}
	f.close();
	std::string s = ws2s(ws);
	return s;
}


FMp4Server::FMp4Server(UsageEnvironment* env, int port) {

	std::string cfg = getConfig();

	rapidjson::Document jsonDoc;
	jsonDoc.Parse(cfg.data());
	rapidjson::Value vs = jsonDoc.GetArray();
	for (uint32_t i = 0; i < vs.Size(); i++)
	{
		const rapidjson::Value& v = vs[i];
		proxy.insert(std::pair<std::string, std::string>(v["source"].GetString(), v["target"].GetString()));
		//printf("%s, %s\n", v["source"].GetString(), v["target"].GetString());
	}

	this->env = env;

	m_server.set_access_channels(websocketpp::log::alevel::none);
	m_server.clear_access_channels(websocketpp::log::alevel::none);
	m_server.init_asio();

	m_server.set_open_handler(bind(&FMp4Server::on_open, this, ::_1));
	m_server.set_close_handler(bind(&FMp4Server::on_close, this, ::_1));
	m_server.set_message_handler(bind(&FMp4Server::on_message, this, ::_1, ::_2));
	m_server.listen(port);
	m_server.start_accept();
}

void FMp4Server::run_sync() {
	m_server.run();
	return;
}
void FMp4Server::run_async() {
	std::thread t1(bind(&server::run, &m_server));
	t1.detach();
	return;
}

void FMp4Server::stop() {
	m_server.stop();
}

void FMp4Server::on_open(connection_hdl hdl) {
	try
	{
		request req = m_server.get_con_from_hdl(hdl)->get_request();
		std::string uri = req.get_uri();
		std::string url = proxy.at(uri);
		bool found = false;
		for (auto rc : m_rcs) {
			if (url == rc->url()) {
				found = true;
				rc->fmp4Server = this;
				FMp4Muxer muxer;
				boost::unique_lock<boost::shared_mutex> l(rc->mu);
				rc->conns.insert(std::pair<connection_hdl, FMp4Muxer>(hdl, muxer));
			}
		}
		if (!found) {
			MyRTSPClient* rc = MyRTSPClient::createNew(*env, url.data());
			if (rc) {
				rc->fmp4Server = this;
				FMp4Muxer muxer;
				boost::unique_lock<boost::shared_mutex> l(rc->mu);
				rc->conns.insert(std::pair<connection_hdl, FMp4Muxer>(hdl, muxer));
				m_rcs.insert(rc);
				rc->connect();
			}
		}
	}
	catch (std::exception & e)
	{
		std::cout << e.what() << std::endl;
	}
	//m_server.send(hdl, fmp4_header, fmp4_header_size, websocketpp::frame::opcode::BINARY);
}

void FMp4Server::on_close(connection_hdl hdl) {

	try
	{
		for (auto rc : m_rcs) {
			boost::unique_lock<boost::shared_mutex> l(rc->mu);
			if (rc->conns.find(hdl) != rc->conns.end()) {

				rc->conns.erase(hdl);
			}
		}

		for (std::set <MyRTSPClient*>::iterator it = m_rcs.begin(); it != m_rcs.end();)
		{
			if ((*it)->conns.size() == 0)
			{
				MyRTSPClient* rc = (MyRTSPClient*)(*it);
				boost::unique_lock<boost::shared_mutex> l(rc->mu);
				m_rcs.erase(it++);
				shutdownStream(rc);
			}
			else
				it++;
		}

	}
	catch (std::exception & e)
	{
		std::cout << e.what() << std::endl;
	}
}

void FMp4Server::on_message(connection_hdl hdl, server::message_ptr msg) {
	/*
	if (msg->get_payload() == "fmp4") {
		for (auto it : m_connections) {
			std::cout << "fmp4 : " << m_connections.size() << "\n";
			m_server.send(hdl, fmp4_header, fmp4_header_size, websocketpp::frame::opcode::BINARY);
		}
	}
	*/
}

void FMp4Server::send(connection_hdl hdl, uint8_t* buf, int buf_size)
{
	try
	{
		m_server.send(hdl, buf, buf_size, websocketpp::frame::opcode::BINARY);
	}
	catch (std::exception & e)
	{
		std::cout << e.what() << std::endl;
	}

}

void send_async_threadfunc(server* server, connection_hdl hdl, uint8_t* buf, int buf_size)
{
	try
	{
		server->send(hdl, buf, buf_size, websocketpp::frame::opcode::BINARY);
		free(buf);
	}
	catch (std::exception & e)
	{
		std::cout << e.what() << std::endl;
	}
}
void FMp4Server::send_async(connection_hdl hdl, uint8_t* buf, int buf_size)
{
	uint8_t* buf2 = (uint8_t*)malloc(buf_size);
	memcpy(buf2, buf, buf_size);
	boost::thread thrd(boost::bind(&send_async_threadfunc, &m_server, hdl, buf2, buf_size));
	thrd.join();
}


/*

FMp4Server wss(9002);

void fmp4_server_batch_send(uint8_t* data, int data_size)
{
	wss.batch_send(data, data_size);
}

void fmp4_server_start() {
	std::thread t1(bind(&server::run, &wss.m_server));
	t1.detach();
	return;
}

void fmp4_server_stop() {
	wss.stop();
}
*/
