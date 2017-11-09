#ifndef BASE_TABLE_LOADER_H_
#define BASE_TABLE_LOADER_H_

#include "disk/disklist.h"

#include <vector>

namespace hm4{
namespace listloader{

namespace {
namespace baselistloader_impl_{

class BaseListLoader{
public:
	using DiskList		= disk::DiskList;
	using container_type	= std::vector<DiskList>;

public:
	static constexpr MMAPFile::Advice DEFAULT_ADVICE = DiskList::DEFAULT_ADVICE;

protected:
	BaseListLoader(const MMAPFile::Advice advice) :
				advice_(advice){}

public:
	const container_type &operator*() const{
		return container_;
	}


protected:
	void insert_(const StringRef &filename){
		container_.emplace_back();
                container_.back().open(filename, advice_);
	}

protected:
	container_type		container_;

private:
	MMAPFile::Advice	advice_;
};


} // namespace baselistloader_impl_
} // namespace

} // namespace listloader
} // namespace


#endif

