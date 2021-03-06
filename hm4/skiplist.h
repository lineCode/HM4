#ifndef _SKIP_LIST_LIST_H
#define _SKIP_LIST_LIST_H

#include "ilist.h"
#include "listcounter.h"

#include <array>

#include "pmallocator.h"

namespace hm4{


class SkipList{
public:
	using Allocator		= MyAllocator::PMAllocator;

	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

	using height_size_type = uint8_t;

public:
	constexpr static height_size_type MAX_HEIGHT = sizeof(uint64_t) * 8;

	class iterator;

public:
	SkipList(Allocator &allocator);
	SkipList(SkipList &&other);
	~SkipList(){
		clear();
	}

	void swap(SkipList &other);

public:
	bool clear();

	bool erase(std::string_view const key);

	iterator insert(	std::string_view key, std::string_view val,
			uint32_t expires = 0, uint32_t created = 0);

	iterator insert(Pair const &src);

	iterator insert(typename Pair::smart_ptr::type<Allocator> &&newdata);

	auto size() const{
		return lc_.size();
	}

	auto bytes() const{
		return lc_.bytes();
	}

	Allocator &getAllocator() const{
		return *allocator_;
	}

public:
	template<bool B>
	iterator find(std::string_view const key, std::bool_constant<B> exact) const;
	iterator begin() const;
	static constexpr iterator end();

public:
	void printLanes() const;
	void printLane(height_size_type lane) const;

private:
	struct 			Node;

	template<typename T>
	using HeightArray	= std::array<T,  MAX_HEIGHT>;

	HeightArray<Node *>	heads_;

	ListCounter		lc_;

	Allocator		*allocator_;

private:
	void deallocate_(Node *node);
	void deallocate_(void *p);

	void zeroing_();

	struct NodeLocator;

	NodeLocator locate_(std::string_view const key, bool shortcut_evaluation);

	const Node *locateNode_(std::string_view const key, bool const exact) const;

	static height_size_type getRandomHeight_();
};

// ==============================

class SkipList::iterator{
public:
	constexpr iterator(const Node *node) : node_(node){}

public:
	using difference_type = SkipList::difference_type;
	using value_type = const Pair;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::forward_iterator_tag;

public:
	iterator &operator++();
	reference operator*() const;

public:
	bool operator==(const iterator &other) const{
		return node_ == other.node_;
	}

	bool operator!=(const iterator &other) const{
		return ! operator==(other);
	}

	pointer operator ->() const{
		return & operator*();
	}

private:
	const Node	*node_;
};

// ==============================

template<bool B>
inline auto SkipList::find(std::string_view const key, std::bool_constant<B> const exact) const -> iterator{
	return locateNode_(key, exact.value);
}

inline auto SkipList::begin() const -> iterator{
	return heads_[0];
}

constexpr auto SkipList::end() -> iterator{
	return nullptr;
}

inline auto SkipList::insert(
		std::string_view key, std::string_view val,
		uint32_t expires, uint32_t created) -> iterator{

	return hm4::insert(*this, key, val, expires, created);
}

inline auto SkipList::insert(Pair const &src) -> iterator{
	return hm4::insert(*this, src);
}

// ==============================

inline auto swap(SkipList &a, SkipList &b){
	return a.swap(b);
}

} // namespace

#endif
