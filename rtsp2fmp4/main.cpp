#include "fmp4_server.h"
#include "live555.h"

char eventLoopWatchVariable = 0;

int main(int argc, char** argv) {

	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

	FMp4Server fs(env, 9002);
	fs.run_async();
	
	env->taskScheduler().doEventLoop(&eventLoopWatchVariable);

	fs.stop();
	
	return 0;

}