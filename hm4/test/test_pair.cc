#include "pair.h"

using hm4::Pair;

#include "mytest.h"

MyTest mytest;

#include <unistd.h>	// sleep

#if 0

#include "mallocallocator.h"
MyAllocator::MallocAllocator allocator;

#else

#include "arenaallocator.h"
MyAllocator::ArenaAllocator<4096> allocator;

#endif

namespace{
	void pair_test_tombstone();
	void pair_test_raw();
	void pair_test_expired(bool slow = false);
	void pair_test();
	void pair_test_ctor();
}



int main(){
	pair_test_tombstone();
	pair_test_raw();
	pair_test();
	pair_test_expired();
	pair_test_ctor();

	return mytest.end();
}



namespace{

	void pair_test_tombstone(){
		mytest.begin("Pair Tombstone");

		std::string_view const key = "name";

		auto p = Pair::tombstone(allocator, key);

		mytest("tombstone isTombstone",	p->isTombstone()	);
		mytest("tombstone key",		p->getKey() == key	);
		mytest("tombstone val",		p->getVal().empty()	);
	}

	const Pair *raw_memory(){
		static const char blob[] = {
			0x50, 0x00, 0x00, 0x00,		// created, 2012-07-13 11:01:20
			0x00, 0x00, 0x00, 0x00,		// milliseconds
			0x00, 0x00, 0x00, 0x00,		// expires
			0x00, 0x00, 0x00, 0x05,		// vallen
			0x00, 0x04,			// keylen
			'n', 'a', 'm', 'e', '\0',	// key
			'P', 'e', 't', 'e', 'r', '\0'	// val
		};

		return reinterpret_cast<const Pair *>(blob);
	}

	void pair_test_raw(){
		mytest.begin("Pair Raw");

		std::string_view const key = "name";
		std::string_view const val = "Peter";

		const Pair *p = raw_memory();

		mytest("valid",		p->isValid()			);

		mytest("key",		p->getKey() == key		);
		mytest("val",		p->getVal() == val		);

		mytest("cmp key",	p->cmp(key) == 0		);
		mytest("cmp",		p->cmp("~~~ non existent") < 0	);
		mytest("cmp",		p->cmp("!!! non existent") > 0	);
	}

	void pair_test_expired(bool const slow){
		mytest.begin("Pair Expired");

		auto p1 = Pair::create(allocator, "key", "val", 1);

		mytest("not expired",	p1->isValid()				);

		if (slow){
			printf("sleep for 2 sec...\n");
			sleep(2);
			mytest("expired",	! p1->isValid()			);
		}

		auto p2 = Pair::create(allocator, "key", "val", 1, 3600 * 24 /* 1970-01-02 */ );
		mytest("expired",		! p2->isValid()			);
	}

	void pair_test_ctor(){
		mytest.begin("c-tor / d-tor");

		auto a = Pair::create(allocator, "1", "one");

		auto b = Pair::clone(allocator, *a);
		mytest("copy Pair",		b->getKey() == "1"		);
	}

	void pair_test(){
		mytest.begin("Pair");

		std::string_view const key = "abcdef";
		std::string_view const val = "1234567890";

		auto t = Pair::tombstone(allocator, key);

		auto p = Pair::create(allocator, key, val);

		mytest("key",			p->getKey() == key		);
		mytest("val",			p->getVal() == val		);

		mytest("cmp",			p->cmp(key) == 0		);
		mytest("cmp",			p->cmp("~~~ non existent") < 0	);
		mytest("cmp",			p->cmp("!!! non existent") > 0	);

		mytest("cmp",			p->cmp(t->getKey()) == 0	);

		mytest("eq",			p->equals(key)			);
		mytest("!eq",			! p->equals("something")	);

		mytest("valid",			p->isValid()			);
		mytest("valid",			p->isValid(*t)			);
	}

} // anonymous namespace

