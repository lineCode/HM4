#ifndef DISK_FILE_BUILDER_H_
#define DISK_FILE_BUILDER_H_

#include "disk/filenames.h"

namespace hm4{
namespace disk{


class DiskFileBuilder{
public:
	template <class LIST>
	static bool buildFromList(const StringRef &filename,
				const LIST &list, bool keepTombstones){
		return build(filename, list.begin(), list.end(), keepTombstones);
	}

	template <class ITERATOR>
	static bool build(const StringRef &filename,
				const ITERATOR &begin, const ITERATOR &end,
				bool keepTombstones);

private:
	template <class ITERATOR>
	static bool writeToFile__(const ITERATOR &begin, const ITERATOR &end,
				std::ofstream &file_meta, std::ofstream &file_index, std::ofstream &file_data,
				bool keepTombstones);


};


} // namespace
} // namespace hm4


#include "filebuilder_impl.h"

#endif

