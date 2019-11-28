#include <boost/interprocess/shared_memory_object.hpp>  
#include <boost/interprocess/mapped_region.hpp>  
#include "fmp4_server.h"
#include "live555.h"

char eventLoopWatchVariable = 0;

int main(int argc, char** argv) {

	for (int i = 0; i < argc; i++)
	{
		std::cout << "argument[" << i << "] is: " << argv[i] << std::endl;
	}

	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

	FMp4Server fs(env, 9002);
	fs.run_async();

	env->taskScheduler().doEventLoop(&eventLoopWatchVariable);

	fs.stop();

	return 0;

}