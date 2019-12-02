#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "fmp4_server.h"
#include "live555.h"

#include <iostream>
#include <vector>
using namespace boost::program_options;
using namespace boost::interprocess;
using namespace boost::posix_time;
using namespace boost::asio;
using namespace std;

char* pEventLoopWatchVariable = NULL;

int main(int argc, char** argv) {
	try {
		boost::program_options::options_description opts("rtsp2fmp4 options");
		opts.add_options()
			("help,h", "help info")
			("quit,q", "quit");

		variables_map vm;
		store(parse_command_line(argc, argv, opts), vm);
		notify(vm);
		if (vm.count("help"))
		{
			cout << opts << endl;
			return -1;
		}
		if (vm.count("quit"))
		{
			shared_memory_object shm_obj(open_only, "shared_memory", read_write);
			mapped_region region(shm_obj, read_write);
			pEventLoopWatchVariable = (char*)region.get_address();
			*pEventLoopWatchVariable = 1;
			std::cout << "Exiting service, please wait." << std::endl;
			return 0;
		}

		shared_memory_object shm_obj(create_only, "shared_memory", read_write);
		shm_obj.truncate(sizeof(char));
		mapped_region region(shm_obj, read_write);
		pEventLoopWatchVariable = (char*)region.get_address();
		*pEventLoopWatchVariable = 0;

		TaskScheduler* scheduler = BasicTaskScheduler::createNew();
		UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);
		FMp4Server fs(env, 9002);
		fs.run_async();
		env->taskScheduler().doEventLoop(pEventLoopWatchVariable);
		fs.stop();
		std::cout << "Normal shutdown of the service." << std::endl;
	}
	catch (boost::program_options::error_with_no_option_name & ex)
	{
		cout << ex.what() << endl;
	}
	catch (interprocess_exception & ex) {
		cout << ex.what() << endl;
	}
	shared_memory_object::remove("shared_memory");
	return 0;

}