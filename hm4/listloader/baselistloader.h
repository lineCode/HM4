#ifndef BASE_TABLE_LOADER_H_
#define BASE_TABLE_LOADER_H_

#include "disk/disklist.h"

namespace hm4{
namespace listloader{
	using DiskList = hm4::disk::DiskList;

	namespace impl_{

		template<class Container>
		struct ContainerHelper{
			using DiskList = hm4::disk::DiskList;

			static_assert( std::is_same<typename Container::value_type, DiskList>::value, "Container values are not DiskList");

			ContainerHelper(Container &container, MMAPFile::Advice const advice, DiskList::OpenMode const mode) :
							container(container),
							advice(advice),
							mode(mode){}

			void clear(){
				container.clear();
			}

			void reserve(size_t const size){
				container.reserve(size);
			}

			void push_back(const StringRef &filename){
				container.emplace_back();
				container.back().open(filename, advice, mode);
			}

		private:
			Container		&container;
			MMAPFile::Advice	advice;
			DiskList::OpenMode	mode;
		};

	} // namespace impl_

} // namespace listloader
} // namespace


#endif

