#ifndef BIN_LOG_LIST_H_
#define BIN_LOG_LIST_H_

#include "decoratorlist.h"
#include "logger.h"


namespace hm4{


template <class List, class BinLogger>
class BinLogList : public DecoratorList<List>{
public:
	using Allocator = typename DecoratorList<List>::Allocator;

	template <class UBinLogger>
	BinLogList(List &list, UBinLogger &&binlogger) :
					DecoratorList<List>(list),
						list_		(&list	),
						binlogger_	(std::forward<UBinLogger>(binlogger)	){}

	auto insert(	std::string_view const key, std::string_view const val,
			uint32_t const expires = 0, uint32_t const created = 0
			){

		return hm4::insert(*this, key, val, expires, created);
	}

	auto insert(typename Pair::smart_ptr::type<Allocator> &&newdata){
		binlogger_(*newdata);

		return list_->insert(std::move(newdata));
	}

	bool erase(std::string_view const key){
		assert(Pair::check(key));

		return this->insert(key, Pair::TOMBSTONE) != std::end(*list_);
	}

	bool clear(){
		bool const result = list_->clear();

		binlogger_.clear();

		return result;
	}

private:
	List		*list_;
	BinLogger	binlogger_;
};


} // namespace


#endif
