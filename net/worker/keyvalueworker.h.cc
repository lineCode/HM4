
#include "protocol/protocoldefs.h"

#include "keyvaluecommands.h"

#include "mystring.h"
#include "myinvokeclassmember.h"
#include "fixedvector.h"

#include <type_traits>
#include <limits>
#include <algorithm>

namespace net{
namespace worker{

template<class PROTOCOL, class DB_ADAPTER, class CONNECTION>
class KeyValueWorkerProcessor{
public:
	using Command = RedisCommands::Command;

	struct RESULTS{
		constexpr static uint16_t MIN		= 10;
		constexpr static uint16_t MAX		= 1000;
		constexpr static uint16_t ACCUMULATE	= 10000;
	};

	template<size_t Size>
	using VectorGETX = FixedVector<std::string_view,Size>;

public:
	KeyValueWorkerProcessor(PROTOCOL &protocol, DB_ADAPTER &db, CONNECTION &buffer) :
					protocol_(protocol),
					db_(db),
					buffer_(buffer){}

public:
	WorkerStatus operator()(){
		using Status  = protocol::ProtocolStatus;

		// PASS

		if (buffer_.size() == 0)
			return WorkerStatus::PASS;

		const Status status = protocol_( std::string_view{ buffer_.data(), buffer_.size() } );

		if (status == Status::BUFFER_NOT_READ)
			return WorkerStatus::PASS;

		// ERROR

		buffer_.clear();

		if (status == Status::ERROR)
			return err_InternalError_();

		// fetch command

		const auto &str = protocol_.getParams().front();

		const auto &cmd = RedisCommands::get(str);

		// EXEC
		return executeCommand_(cmd);
	}

private:
	WorkerStatus executeCommand_(const Command cmd){
		using mutable_type = std::integral_constant<bool, DB_ADAPTER::MUTABLE>;

		return executeCommand_(cmd, mutable_type{});
	}

	WorkerStatus executeCommand_(const Command cmd, std::false_type){
		switch(cmd){

		case Command::EXIT	: return WorkerStatus::DISCONNECT;
		case Command::SHUTDOWN	: return WorkerStatus::SHUTDOWN;

		case Command::INFO	: return do_info();
		case Command::SAVE	: return do_save();
		case Command::RELOAD	: return do_reload();

		case Command::GET	: return do_get();
		case Command::GETX	: return do_getx();

		case Command::COUNT	: return do_count();
		case Command::SUM	: return do_sum();
		case Command::MIN	: return do_min();
		case Command::MAX	: return do_max();

		default			: return err_NotImplemented_();
		}
	}

	WorkerStatus executeCommand_(const Command cmd, std::true_type){
		switch(cmd){

		case Command::SET	: return do_set();
		case Command::SETEX	: return do_setex();
		case Command::DEL	: return do_del();

		case Command::INCR	: return do_incr();
		case Command::DECR	: return do_decr();
		case Command::GETSET	: return do_getset();

		default			: return executeCommand_(cmd, std::false_type{});
		}
	}

private:
	WorkerStatus err_String_(std::string_view const error){
		protocol_.response_error(buffer_, error);
		return WorkerStatus::WRITE;
	}

	WorkerStatus err_BadRequest_(){
		return err_String_("Bad request");
	}

	WorkerStatus err_NotImplemented_(){
		return err_String_("Not Implemented");
	}

	WorkerStatus err_InternalError_(){
		return err_String_("Internal Error");
	}

private:
	// SYSTEM

	WorkerStatus do_info(){
		const auto &p = protocol_.getParams();

		if (p.size() != 1)
			return err_BadRequest_();

		protocol_.response_string(buffer_, db_.info());

		return WorkerStatus::WRITE;
	}

	template<typename F>
	WorkerStatus do_save_(F func){
		const auto &p = protocol_.getParams();

		if (p.size() != 1)
			return err_BadRequest_();

		invoke_class_member(db_, func);

		protocol_.response_ok(buffer_);

		return WorkerStatus::WRITE;
	}

	WorkerStatus do_save(){
		return do_save_(&DB_ADAPTER::save);
	}

	WorkerStatus do_reload(){
		return do_save_(&DB_ADAPTER::reload);
	}

	// IMMUTABLE

	WorkerStatus do_get(){
		const auto &p = protocol_.getParams();

		if (p.size() != 2)
			return err_BadRequest_();

		const auto &key = p[1];

		if (key.empty())
			return err_BadRequest_();

		protocol_.response_string(buffer_, db_.get(key));

		return WorkerStatus::WRITE;
	}

	// HIGHER LEVEL ACCUMULATORS

	template<typename Acummulator>
	WorkerStatus do_accumulate_(Acummulator &accumulator){
		const auto &p = protocol_.getParams();

		if (p.size() != 4)
			return err_BadRequest_();

		const auto &key    = p[1];
		uint16_t const count = from_string<uint16_t>(p[2]);
		const auto &prefix = p[3];

		if constexpr(Acummulator::container()){
			auto const count2 = std::clamp(count, RESULTS::MIN, RESULTS::MAX);

			auto const container = db_.foreach(key, count2, prefix, accumulator);

			protocol_.response_strings(buffer_, container );
		}else{
			auto const count2 = std::clamp(count, RESULTS::MIN, RESULTS::ACCUMULATE);

			auto const [ data, lastKey ] = db_.foreach(key, count2, prefix, accumulator);

			to_string_buffer_t buffer;

			protocol_.response_strings(buffer_, to_string(data, buffer), lastKey);
		}

		return WorkerStatus::WRITE;
	}

	auto do_getx(){
		constexpr size_t size = 2 * RESULTS::MAX + 1;

		using MyVector = VectorGETX<size>;

		struct AccumulatorVectorNew{
			MyVector data;

			AccumulatorVectorNew(typename MyVector::size_type size){
				data.reserve(size);
			}

			auto operator()(std::string_view key, std::string_view val){
				data.emplace_back(key);
				data.emplace_back(val);
				return true;
			}

			const auto &result(std::string_view key = ""){
				// emplace even empty
				data.emplace_back(key);

				return data;
			}

			constexpr static auto container(){
				return true;
			}
		};

		AccumulatorVectorNew accumulator(size);

		return do_accumulate_(accumulator);
	}

	auto do_count(){
		using T = int64_t;

		struct{
			T data = 0;

			auto operator()(std::string_view, std::string_view){
				++data;
				return true;
			}

			auto result(std::string_view key = "") const{
				return std::make_pair(data, key);
			}

			constexpr static auto container(){
				return false;
			}
		} accumulator;

		return do_accumulate_(accumulator);
	}

	auto do_sum(){
		using T = int64_t;

		struct{
			T data = 0;

			auto operator()(std::string_view, std::string_view val){
				data += from_string<T>(val);
				return true;
			}

			auto result(std::string_view key = "") const{
				return std::make_pair(data, key);
			}

			constexpr static auto container(){
				return false;
			}
		} accumulator;

		return do_accumulate_(accumulator);
	}

	auto do_min(){
		using T = int64_t;

		struct{
			T data = std::numeric_limits<T>::max();

			auto operator()(std::string_view, std::string_view val){
				auto x = from_string<T>(val);

				if (x < data)
					data = x;
				return true;
			}

			auto result(std::string_view key = "") const{
				return std::make_pair(data, key);
			}

			constexpr static auto container(){
				return false;
			}
		} accumulator;

		return do_accumulate_(accumulator);
	}

	auto do_max(){
		using T = int64_t;

		struct{
			T data = std::numeric_limits<T>::min();

			auto operator()(std::string_view, std::string_view val){
				auto x = from_string<T>(val);

				if (x > data)
					data = x;
				return true;
			}

			auto result(std::string_view key = "") const{
				return std::make_pair(data, key);
			}

			constexpr static auto container(){
				return false;
			}
		} accumulator;

		return do_accumulate_(accumulator);
	}

	// MUTABLE

	WorkerStatus do_set(){
		const auto &p = protocol_.getParams();

		if (p.size() != 3 && p.size() != 4)
			return err_BadRequest_();

		const auto &key = p[1];
		const auto &val = p[2];
		uint32_t const exp = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

		if (key.empty())
			return err_BadRequest_();

		db_.set(key, val, exp);

		protocol_.response_ok(buffer_);

		return WorkerStatus::WRITE;
	}

	WorkerStatus do_setex(){
		const auto &p = protocol_.getParams();

		if (p.size() != 4)
			return err_BadRequest_();

		const auto &key = p[1];
		const auto &val = p[3];

		uint32_t const exp = from_string<uint32_t>(p[2]);

		if (key.empty())
			return err_BadRequest_();

		db_.set(key, val, exp);

		protocol_.response_ok(buffer_);

		return WorkerStatus::WRITE;
	}

	WorkerStatus do_del(){
		const auto &p = protocol_.getParams();

		if (p.size() != 2)
			return err_BadRequest_();

		const auto &key = p[1];

		if (key.empty())
			return err_BadRequest_();

		protocol_.response_bool(buffer_, db_.del(key));

		return WorkerStatus::WRITE;
	}

	// HIGHER LEVEL ATOMIC COUNTERS

	template<int Sign>
	WorkerStatus do_incr_decr(){
		const auto &p = protocol_.getParams();

		if (p.size() != 2 && p.size() != 3)
			return err_BadRequest_();

		const auto &key = p[1];

		if (key.empty())
			return err_BadRequest_();

		int64_t n = p.size() == 3 ? Sign * from_string<int64_t>(p[2]) : Sign;

		if (n == 0)
			return err_BadRequest_();

		if (! key.empty())
			n += from_string<int64_t>( db_.get(key) );

		to_string_buffer_t buffer;

		std::string_view const val = to_string(n, buffer);

		db_.set(key, val);

		protocol_.response_string(buffer_, val);

		return WorkerStatus::WRITE;
	}

	auto do_incr(){
		return do_incr_decr<+1>();
	}

	auto do_decr(){
		return do_incr_decr<-1>();
	}

	auto do_getset(){
		const auto &p = protocol_.getParams();

		if (p.size() != 3)
			return err_BadRequest_();

		// GET

		const auto &key = p[1];

		if (key.empty())
			return err_BadRequest_();

		protocol_.response_string(buffer_, db_.get(key));
		// now old value is inserted in the buffer and
		// we do not care if pair is overwritten

		// SET

		const auto &val = p[2];

		db_.set(key, val);

		// return

		return WorkerStatus::WRITE;
	}

private:
	PROTOCOL	&protocol_;
	DB_ADAPTER	&db_;
	CONNECTION	&buffer_;
};


// ====================================


template<class PROTOCOL, class DB_ADAPTER>
template<class CONNECTION>
WorkerStatus KeyValueWorker<PROTOCOL, DB_ADAPTER>::operator()(CONNECTION &buffer){
	using MyKeyValueWorkerProcessor = KeyValueWorkerProcessor<PROTOCOL, DB_ADAPTER, CONNECTION>;

	MyKeyValueWorkerProcessor processor{ protocol_, db_, buffer };

	return processor();
}


} // namespace worker
} // namespace

