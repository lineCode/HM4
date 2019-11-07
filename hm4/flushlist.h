#ifndef _Flusher_List_H
#define _Flusher_List_H

#include "decoratorlist.h"

#include "logger.h"

namespace hm4{


template <class List, class Flusher, class ListLoader = std::nullptr_t, size_t MAX_SIZE = 128 * 1024 * 1024>
class FlushList : public DecoratorList<List>{
private:
	template <class UFlusher>
	FlushList(List &list, UFlusher &&flusher, ListLoader *loader, size_t const maxSize = MAX_SIZE) :
					DecoratorList<List>(list),
						list_(&list),
						flusher_(std::forward<UFlusher>(flusher)),
						loader_(loader),
						maxSize_(maxSize > MAX_SIZE ? maxSize : MAX_SIZE){}

public:
	template <class UFlusher>
	FlushList(List &list, UFlusher &&flusher, ListLoader &loader, size_t const maxSize = MAX_SIZE) :
					FlushList(list, std::forward<UFlusher>(flusher), &loader, maxSize){}

	template <class UFlusher>
	FlushList(List &list, UFlusher &&flusher, size_t const maxSize = MAX_SIZE) :
					FlushList(list, std::forward<UFlusher>(flusher), nullptr, maxSize){}

	~FlushList(){
		flush_();
	}

	bool insert(	std::string_view const key, std::string_view const val,
			uint32_t const expires = 0, uint32_t const created = 0){

		bool const result = list_->insert(key, val, expires, created );

		if (list_->bytes() > maxSize_){
			flush();
		}

		return result;
	}

public:
	bool flush(){
		bool const r = flush_();
		list_->clear();
		notifyLoader_();

		return r;
	}

	// Command pattern
	int command(bool const completeFlush){
		if (completeFlush)
			return flush();
		else
			return notifyLoader_();
	}

private:
	template<typename T>
	bool notifyLoader_(const T *){
		log__("Reloading data...");
		return loader_ && loader_->refresh();
	}

	static bool notifyLoader_(const std::nullptr_t *){
		return true;
	}

	bool notifyLoader_(){
		return notifyLoader_(loader_);
	}

	bool flush_(){
		log__("Flushing data...", "List size: ", list_->bytes(), "Max permited size: ", maxSize_);
		return flusher_ << *list_;
	}

private:
	List		*list_;
	Flusher		flusher_;
	ListLoader	*loader_;
	size_t		maxSize_;
};


} // namespace


#endif

