#define FMT_HEADER_ONLY
#include "fmt/printf.h"

#include "factory/singlelist.h"
#include "factory/immutable.h"
#include "factory/mutable.h"
#include "factory/mutablebinlog.h"

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

	int immutableLists(const MyOptions &opt){
		using hm4::listloader::DirectoryListLoader;

		if (DirectoryListLoader::checkIfLoaderNeed(opt.db_path)){
				fmt::print(std::clog, "Starting immutable server...\n");
				return main2(opt, DBAdapterFactory::Immutable{ opt.db_path } );
		}else{
				fmt::print(std::clog, "Starting singlelist server...\n");
				return main2(opt, DBAdapterFactory::SingleList{ opt.db_path } );
		}
	}

	int mutableLists(const MyOptions &opt){
		bool const have_binlog = ! opt.binlog_path.empty();

		size_t const max_memlist_size  = opt.max_memlist_size  * MB;
		size_t const max_memlist_arena = opt.max_memlist_arena < MIN_ARENA_SIZE ? 0 : opt.max_memlist_arena * MB;

		if (max_memlist_arena){
			MyArenaAllocator arenaAllocator{ max_memlist_arena };

			if (have_binlog){
				fmt::print(std::clog, "Starting {} server with {} of {} MB...\n", "mutable binlog", "ArenaAllocator", opt.max_memlist_arena);

				return main2(opt, DBAdapterFactory::MutableBinLog{   opt.db_path, opt.binlog_path, max_memlist_size, arenaAllocator } );
			}else{
				fmt::print(std::clog, "Starting {} server with {} of {} MB...\n", "mutable", "ArenaAllocator", opt.max_memlist_arena);

				return main2(opt, DBAdapterFactory::Mutable{   opt.db_path, max_memlist_size, arenaAllocator } );
			}
		}else{
			MySTDAllocator stdAllocator;

			if (have_binlog){
				fmt::print(std::clog, "Starting {} server with {}...\n", "mutable binlog", "STDAllocator");

				return main2(opt, DBAdapterFactory::MutableBinLog{   opt.db_path, opt.binlog_path, max_memlist_size, stdAllocator } );
			}else{
				fmt::print(std::clog, "Starting {} server with {}...\n", "mutable", "STDAllocator");

				return main2(opt, DBAdapterFactory::Mutable{   opt.db_path, max_memlist_size, stdAllocator } );
			}
		}
	}
}

int main(int argc, char **argv){
	MyOptions const opt = prepareOptions(argc, argv);

	if (opt.immutable)
		return immutableLists(opt);
	else
		return mutableLists(opt);
}

namespace{

	void printUsage(const char *cmd);
	void printError(const char *msg);

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

	template<class Factory>
	int main2(const MyOptions &opt, Factory &&adapter_factory){
		using MyAdapterFactory	= Factory;
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
				adapter_factory().save();
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
		fmt::print(stderr, "{}\n", msg);
		exit(1);
	}

	void printUsage(const char *cmd){
		#ifdef NOT_HAVE_CHARCONV
		const char *convert = "sstream";
		#else
		const char *convert = "charconv";
		#endif

		fmt::print(
			"db_net version {version}\n"
			"\n"
			"Build:\n"
			"\tSelector"	"\t{selector}\n"
			"\tConvertion"	"\t{convert}\n"
			"\n"
			"Usage:\n"
			"\t{cmd} [configuration file] - start server\n"
			"\t...or...\n"
			"\t{cmd} r [lsm_path] [optional tcp port] - start immutable  server\n"
			"\t{cmd} w [lsm_path] [optional tcp port] - start mutable    server\n"
			"\n"
			"\t\tPath names must be written with quotes:\n"
			"\t\tExample directory/file.'*'.db\n"
			"\t\tThe '*', will be replaced with ID's\n"
			"\n"
			,

			fmt::arg("version",	hm4::version::str	),
			fmt::arg("selector",	MySelector::NAME	),
			fmt::arg("convert",	convert			),
			fmt::arg("cmd",		cmd			)
		);

		fmt::print("INI File Usage:\n");

		MyOptions::print();

		fmt::print("\n");

		exit(10);
	}

} // anonymous namespace


