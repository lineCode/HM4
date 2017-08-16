#include "multi/collectionlist.h"
#include "listloader/directorylistloader.h"

#include "selector/pollselector.h"
#include "protocol/redisprotocol.h"
#include "worker/keyvalueworker.h"
#include "asyncloop.h"

#include "multi/duallist.h"
#include "skiplist.h"
#include "flushlist.h"

#include "idgenerator/idgeneratordate.h"
#include "flusher/diskfileflusher.h"

#include "listdbadapter.h"

struct MyImmutableDBAdapterFactory;
struct MyMutableDBAdapterFactory;

// ----------------------------------

constexpr const char	*HOSTNAME		= "localhost.not.used.yet";
constexpr int		PORT			= 2000;

constexpr size_t	MAX_CLIENTS		= 1024;
constexpr uint32_t	CONNECTION_TIMEOUT	= 30;
const     size_t	MAX_PACKET_SIZE		= hm4::Pair::maxBytes() * 2;

constexpr size_t	MEMLIST_SIZE		= 512 * 1024 * 1024;

// ----------------------------------

using MySelector	= net::selector::PollSelector;
using MyProtocol	= net::protocol::RedisProtocol;

// ----------------------------------

struct MyImmutableDBAdapterFactory{
	using ListLoader	= hm4::listloader::DirectoryListLoader;
	using ImmutableList	= hm4::multi::CollectionList<ListLoader::container_type>;

	using CommandObject	= ListLoader;
	using DBAdapter		= ListDBAdapter<const ImmutableList, CommandObject>;

	using MyDBAdapter	= DBAdapter;

	MyImmutableDBAdapterFactory(const char *path, size_t) :
					loader_(path),
					imList_(*loader_),
					adapter_(imList_, /* cmd */ loader_){}

	MyDBAdapter &operator()(){
		return adapter_;
	}

private:
	ListLoader		loader_;
	ImmutableList		imList_;
	DBAdapter		adapter_;
};

struct MyMutableDBAdapterFactory{
	using ListLoader	= hm4::listloader::DirectoryListLoader;
	using ImmutableList	= hm4::multi::CollectionList<ListLoader::container_type>;

	using MemList		= hm4::SkipList;
	using IDGenerator	= hm4::idgenerator::IDGeneratorDate;
	using Flusher		= hm4::flusher::DiskFileFlusher<IDGenerator>;
	using MutableFlushList	= hm4::FlushList<MemList,Flusher,ListLoader>;

	using DList		= hm4::multi::DualList<MutableFlushList, ImmutableList, /* erase tombstones */ true>;

	using CommandObject	= MutableFlushList;
	using DBAdapter		= ListDBAdapter<DList, CommandObject>;

	using MyDBAdapter	= DBAdapter;

	MyMutableDBAdapterFactory(const char *path, size_t const memListSize) :
					loader_(path),
					imList_(*loader_),
					muflList_(
						memList_,
						Flusher{ IDGenerator{}, path },
						loader_,
						memListSize
					),
					list_(muflList_, imList_),
					adapter_(list_, /* cmd */ muflList_){}

	MyDBAdapter &operator()(){
		return adapter_;
	}

private:
	ListLoader		loader_;
	ImmutableList		imList_;
	MemList			memList_;
	MutableFlushList	muflList_;
	DList			list_;
	DBAdapter		adapter_;
};


// ----------------------------------

static int printUsage(const char *cmd);
static int printError(const char *msg);

// ----------------------------------

#include "signalguard.h"
#include "stou.h"

template<class FACTORY>
static int main2(int const fd1, FACTORY &&adapter_f);

int main(int argc, char **argv){
	if (argc <= 3)
		return printUsage(argv[0]);

	const bool     immutable	= argv[1][0] == 'r';
	const char     *path		= argv[2];
	const uint16_t port		= stou<uint16_t>(argv[3]);

	if (port == 0)
		return printError("Can not create server socket on invalid port.");

	// ----------------------------------

	int const fd1 = net::socket_create(net::SOCKET_TCP{}, HOSTNAME, port);

	if (fd1 < 0)
		return printError("Can not create server socket.");

	if (immutable)
		return main2(fd1, MyImmutableDBAdapterFactory{ path, MEMLIST_SIZE } );
	else
		return main2(fd1, MyMutableDBAdapterFactory{   path, MEMLIST_SIZE } );
}

template<class FACTORY>
static int main2(int const fd1, FACTORY &&adapter_f){
	using MyAdapterFactory	= FACTORY;
	using MyDBAdapter	= typename MyAdapterFactory::MyDBAdapter;
	using MyWorker		= net::worker::KeyValueWorker<MyProtocol, MyDBAdapter>;
	using MyLoop		= net::AsyncLoop<MySelector, MyWorker>;

	MyLoop loop( MySelector{ MAX_CLIENTS }, MyWorker{ adapter_f() }, { fd1 },
							CONNECTION_TIMEOUT, MAX_PACKET_SIZE);

	SignalGuard guard;

	while(loop.process() && guard);

	return 0;
}

// ----------------------------------

#include <iostream>

static int printError(const char *msg){
	std::cout << msg << '\n';
	return 1;
}

static int printUsage(const char *cmd){
	std::cout
		<< "Usage:"	<< '\n'
		<< "\t"		<< cmd	<< " r [lsm_path] [port] - start immutable server"	<< '\n'
		<< "\t"		<< cmd	<< " w [lsm_path] [port] - start mutable   server"	<< '\n'

		<< "\t\tPath names must be written like this:"	<< '\n'
		<< "\t\tExample 'directory/file.*.db'"		<< '\n'

		<< '\n';

	return 10;
}

