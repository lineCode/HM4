#include <cstdint>
#include <iostream>
#include <iomanip>


#include "stringref.h"
#include "stou_safe.h"

class MyOptions{
public:
	uint8_t		immutable		= 1;
	std::string	db_path;

	nullptr_t	host			= nullptr;
	uint16_t	port			= 2000;
	uint32_t	timeout			= 30;

	uint32_t	max_clients		= 512;
	size_t		max_memlist_size	= 100;

private:
	constexpr static const char *OPT_immutable		= "immutable"		;
	constexpr static const char *OPT_db_path        	= "db_path"		;

	constexpr static const char *OPT_host			= "host"		;
	constexpr static const char *OPT_port			= "port"		;
	constexpr static const char *OPT_timeout		= "timeout"		;

	constexpr static const char *OPT_max_clients		= "max_clients"		;
	constexpr static const char *OPT_max_memlist_size	= "max_memlist_size"	;

public:
	void operator()(const StringRef &name, const StringRef &val){
		do_(name, val, OPT_immutable,		immutable		);
		do_(name, val, OPT_db_path,		db_path			);

		do_(name, val, OPT_host,		host			);
		do_(name, val, OPT_port,		port			);
		do_(name, val, OPT_timeout,		timeout			);

		do_(name, val, OPT_max_clients,		max_clients		);
		do_(name, val, OPT_max_memlist_size,	max_memlist_size	);
	}

	static void print(){
		const MyOptions opt;

		print__(OPT_immutable,		opt.immutable,		"Start immutable server (1/0)"		);
		print__(OPT_db_path,		opt.db_path,		"Path to database"			);

		print__(OPT_host,		opt.host,		"TCP host to listen (not working)"	);
		print__(OPT_port,		opt.port,		"TCP port to listen"			);
		print__(OPT_timeout,		opt.timeout,		"Connection timeout in seconds"		);

		print__(OPT_max_clients,	opt.max_clients,	"Max Clients"				);
		print__(OPT_max_memlist_size,	opt.max_memlist_size,	"Max size of memlist in MB"		);
	}

private:
	template<class T>
	static void do_(const StringRef &name, const StringRef &value, const char *opt_name, T &opt_value){
		if (name == opt_name)
			assign_(value, opt_value);
	}

private:
	template<class T>
	static void assign_(const StringRef &value, T &opt_value){
		static_assert(std::is_unsigned<T>::value, "T must be unsigned type");
		auto const default_value = opt_value;
		opt_value = stou<T>(value, default_value);
	}

	static void assign_(const StringRef &value, std::string &opt_value){
		opt_value = value;
	}

	static void assign_(const StringRef &, nullptr_t){
		/* nada */
	}

private:
	template<class T>
	static void print__(const char *name, const T &value, const char *description){
		std::cout
			<< '\t'
			<< std::setw(20) << std::left << name
			<< std::setw(6) << std::left << value
			<< description
			<< '\n'
		;
	}

	static void print__(const char *name, uint8_t const value, const char *description){
		print__(name, (int) value, description);
	}

	static void print__(const char *name, nullptr_t, const char *description){
		print__(name, "n/a", description);
	}
};

