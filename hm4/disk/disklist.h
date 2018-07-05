#ifndef DISK_LIST_H_
#define DISK_LIST_H_

#include "mmapfileplus.h"
#include "binarysearch.h"

#include "ilist.h"
#include "filemeta.h"

#include "mynarrow.h"

#include <type_traits>

namespace hm4{
namespace disk{


class DiskList : public IList<DiskList, false>{
public:
	class Iterator;
	class BTreeSearchHelper;

	enum class OpenMode : char {
		NORMAL,
		MINIMAL
	};

	static constexpr MMAPFile::Advice	DEFAULT_ADVICE	= MMAPFile::Advice::RANDOM;
	static constexpr OpenMode		DEFAULT_MODE	= OpenMode::NORMAL;

private:
	static constexpr size_type	BIN_SEARCH_MINIMUM_DISTANCE	= 3;
	static constexpr int		CMP_ZERO			= 1;

public:
	DiskList() = default;

	DiskList(DiskList &&other) = default;

	// no need d-tor,
	// MMAPFile-s will be closed automatically
	// ~DiskList() = default;

	void close();

	bool open(const StringRef &filename, MMAPFile::Advice const advice = DEFAULT_ADVICE, OpenMode const mode = DEFAULT_MODE){
		switch(mode){
		default:
		case OpenMode::NORMAL	: return openNormal_ (filename, advice);
		case OpenMode::MINIMAL	: return openMinimal_(filename, advice);
		}
	}

	operator bool(){
		return metadata_;
	}

	void printMetadata() const{
		metadata_.print();
	}

public:
	int cmpAt(size_type index, const StringRef &key) const{
		assert(!key.empty());

		const Pair *p = operator[](index);

		return p ? p->cmp(key) : CMP_ZERO;
	}


public:
	const Pair *operator[](const StringRef &key) const{
		assert(!key.empty());

		const auto x = search_(key);

		return x.found ? operator[](x.pos) : nullptr;
	}

	const Pair *operator[](size_type const index) const{
		assert( index < size() );

		return fdGetAt_(index);
	}

	size_type size(bool const = false) const{
		return narrow<size_type>( metadata_.size() );
	}

	size_t bytes() const{
		return mData_.size();
	}

	bool sorted() const{
		return metadata_.sorted();
	}

	bool aligned() const{
		return metadata_.aligned();
	}

public:
	Iterator lowerBound(const StringRef &key) const;
	Iterator begin() const;
	Iterator end() const;

private:
	bool openNormal_ (const StringRef &filename, MMAPFile::Advice advice);
	bool openMinimal_(const StringRef &filename, MMAPFile::Advice advice);

private:
	const Pair *fdSafeAccess_(const Pair *blob) const;

	const Pair *fdGetAt_(size_type index) const;
	const Pair *fdGetNext_(const Pair *blob) const;

private:
	static size_t alignedSize__(const Pair *blob, bool aligned);

private:
	BinarySearchResult<size_type> binarySearch_(const StringRef &key, size_type start, size_type end) const;

	BinarySearchResult<size_type> binarySearch_(const StringRef &key) const{
		return binarySearch_(key, 0, size());
	}

	BinarySearchResult<size_type> btreeSearch_(const StringRef &key) const;

	BinarySearchResult<size_type> hotLineSearch_(const StringRef &key) const;

	BinarySearchResult<size_type> search_(const StringRef &key) const;

private:
	MMAPFilePlus		mIndx_;
	MMAPFilePlus		mLine_;
	MMAPFilePlus		mData_;

	MMAPFilePlus		mTree_;
	MMAPFilePlus		mKeys_;

	FileMeta		metadata_;
};

// ===================================

class DiskList::Iterator{
private:
	friend class DiskList;
	Iterator(const DiskList &list, size_type pos,
				bool sorted_);

public:
	Iterator &operator++(){
		++pos_;
		return *this;
	}

	Iterator &operator--(){
		--pos_;
		return *this;
	}

	bool operator==(const Iterator &other) const{
		return &list_ == &other.list_ && pos_ == other.pos_;
	}

	const Pair &operator*() const;

public:
	bool operator!=(const Iterator &other) const{
		return ! operator==(other);
	}

	const Pair *operator ->() const{
		return & operator*();
	}

private:
	const DiskList	&list_;
	size_type	pos_;
	bool		sorted_;

private:
	mutable
	size_type	tmp_pos		= 0;

	mutable
	const Pair	*tmp_blob	= nullptr;
};

// ==============================

inline auto DiskList::begin() const -> Iterator{
	return Iterator(*this,      0, sorted());
}

inline auto DiskList::end() const -> Iterator{
	return Iterator(*this, size(), sorted());
}

inline auto DiskList::lowerBound(const StringRef &key) const -> Iterator{
	const auto x = search_(key);

	return Iterator(*this, x.pos, sorted());
}


} // namespace disk
} // namespace

#endif

