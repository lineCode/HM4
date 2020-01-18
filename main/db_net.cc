#include "db_net_immutablefactory.h"
#include "db_net_mutablefactory.h"
#include "db_net_singlelistfactory.h"

#include "pmallocator.h"
#include "stdallocator.h"
#include "arenaallocator.h"

#include "version.h"

// ----------------------------------

#include "protocol/redisprotocol.h"
#include "worker/keyvalueworker.h"
#include "asyncloop.h"

// ----------------------------------

#if defined SELECTOR_EPOOL
	#include "selector/epollselector.h"

	using MySelector	= net::selector::EPollSelector;
#elif defined SELECTOR_KQUEUE
	#include "selector/kqueueselector.h"

	using MySelector	= net::selector::KQueueSelector;
#elif defined SELECTOR_POOL
	#include "selector/pollselector.h"

	using MySelector	= net::selector::PollSelector;
#else
	#error "No net::selector selected!"
#endif

using MyProtocol	= net::protocol::RedisProtocol;

using MyArenaAllocator	= MyAllocator::PMOwnerAllocator<MyAllocator::ArenaAllocator>;
using MySTDAllocator	= MyAllocator::PMOwnerAllocator<MyAllocator::STDAllocator>;

// ----------------------------------

constexpr size_t MB = 1024 * 1024;

constexpr size_t MIN_ARENA_SIZE = 128;

// ----------------------------------

#include "signalguard.h"
#include "mystring.h"
#include "db_net_options.h"

#include <iostream>

namespace{
	MyOptions prepareOptions(int argc, char **argv);

	template<class FACTORY>
	int main2(const MyOptions &opt, FACTORY &&adapter_f);

	void printUsage(const char *cmd);
	void printError(const char *msg);
}

int main(int argc, char **argv){
	MyOptions const opt = prepareOptions(argc, argv);

	size_t const max_memlist_size  = opt.max_memlist_size  * MB;
	size_t const max_memlist_arena = opt.max_memlist_arena < MIN_ARENA_SIZE ? 0 : opt.max_memlist_arena * MB;

	using hm4::listloader::DirectoryListLoader;

	if (opt.immutable == 0 && max_memlist_arena){
		std::clog << "Starting mutable server with ArenaAllocator of " << opt.max_memlist_arena << " MB..."	<< '\n';

		MyArenaAllocator arenaAllocator{ max_memlist_arena };

		return main2(opt, MyMutableDBAdapterFactory{   opt.db_path, max_memlist_size, arenaAllocator } );

	} else if (opt.immutable == 0){
		std::clog << "Starting mutable server with STDAllocator..."	<< '\n';

		MySTDAllocator stdAllocator;

		return main2(opt, MyMutableDBAdapterFactory{   opt.db_path, max_memlist_size, stdAllocator } );

	}else if (DirectoryListLoader::checkIfLoaderNeed(opt.db_path)){
		std::clog << "Starting immutable server..."	<< '\n';
		return main2(opt, MyImmutableDBAdapterFactory{ opt.db_path } );

	}else{
		std::clog << "Starting singlelist server..."	<< '\n';
		return main2(opt, MySingleListDBAdapterFactory{ opt.db_path } );
	}
}

namespace{

	MyOptions prepareOptions(int argc, char **argv){
		auto argImutable = [](const char *s) -> uint16_t{
			switch(s[0]){
			default:
			case 'r': return 1;
			case 'w': return 0;
			}
		};

		MyOptions opt;

		switch(argc){
		case 1 + 1:
			if (! readINIFile(argv[1], opt))
				printError("Can not open config file...");

			break;

		case 3 + 1:
			opt.port	= from_string<uint16_t>(argv[3]);

			opt.immutable	= argImutable(argv[1]);
			opt.db_path	= argv[2];
			break;

		case 2 + 1:
			opt.immutable	= argImutable(argv[1]);
			opt.db_path	= argv[2];
			break;

		default:
			printUsage(argv[0]);
		}

		if (opt.port == 0)
			printError("Can not create server socket on port zero...");

		return opt;
	}

	template<class FACTORY>
	int main2(const MyOptions &opt, FACTORY &&adapter_factory){
		using MyAdapterFactory	= FACTORY;
		using MyDBAdapter	= typename MyAdapterFactory::MyDBAdapter;
		using MyWorker		= net::worker::KeyValueWorker<MyProtocol, MyDBAdapter>;
		using MyLoop		= net::AsyncLoop<MySelector, MyWorker>;

		size_t const max_packet_size = hm4::Pair::maxBytes() * 2;

		int const fd = net::socket_create(net::SOCKET_TCP{}, opt.host, opt.port);

		if (fd < 0)
			printError("Can not create server socket...");

		MyLoop loop{
				/* selector */	MySelector	{ opt.max_clients },
				/* worker */	MyWorker	{ adapter_factory() },
				/* server fd */	{ fd },
				opt.max_clients, opt.timeout, max_packet_size
		};

		SignalGuard guard;

		auto signal_processing = [&adapter_factory](Signal const signal){
			switch(signal){
			case Signal::HUP:
				adapter_factory().refresh(true);
				return true;

			case Signal::INT:
			case Signal::TERM:
				return false;

			default:
				return true;
			}
		};

		while( loop.process() && signal_processing(guard()) );

		return 0;
	}

	// ----------------------------------

	void printError(const char *msg){
		std::cout << msg << '\n';
		exit(1);
	}

	void printUsage(const char *cmd){
		#ifdef NOT_HAVE_CHARCONV
		const char *convert = "sstream";
		#else
		const char *convert = "charconv";
		#endif

		std::cout
			<< "db_net version " << hm4::version::str 							<< '\n'
					<< '\n'
			<< "Build:"	<< '\n'
			<< "\t"		<< "Selector"	<< '\t'	<< MySelector::NAME					<< '\n'
			<< "\t"		<< "Convertion"	<< '\t'	<< convert						<< '\n'
					<< '\n'
			<< "Usage:"	<< '\n'
			<< "\t"		<< cmd	<< " [configuration file] - start server"				<< '\n'
			<< "...or..."	<< '\n'
			<< "\t"		<< cmd	<< " r [lsm_path] [optional tcp port] - start immutable  server"	<< '\n'
			<< "\t"		<< cmd	<< " w [lsm_path] [optional tcp port] - start mutable    server"	<< '\n'

			<< "\t\tPath names must be written with quotes:"	<< '\n'
			<< "\t\tExample directory/file.'*'.db"			<< '\n'
			<< "\t\tThe '*', will be replaced with ID's"		<< '\n'

			<< '\n';

		std::cout
			<< "INI File Usage:"	<< '\n';

		MyOptions::print();

		std::cout
			<< '\n';

		exit(10);
	}

} // anonymous namespace


