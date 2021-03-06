#ifndef FILE_READER_H_
#define FILE_READER_H_

#include <fstream>

#include <string_view>
#include "trim.h"

namespace filereader_impl_{

	class BasicFileReader{
	public:
		using Options = unsigned char;

	public:
		constexpr static Options OPTION_TRIM		= 1 << 0;
		constexpr static Options OPTION_SKIP_EMPRY	= 1 << 1;

		constexpr static Options DEFAULT_OPTIONS	= OPTION_TRIM | OPTION_SKIP_EMPRY;

	protected:
		static bool applyOptions__(const Options &options, char *buffer, size_t size){
			if (options & OPTION_TRIM)
				size = trim_size(buffer, size);

			if ((options & OPTION_SKIP_EMPRY) && size == 0)
				return false;

			return true;
		}

	protected:
		BasicFileReader() = default;
	};

} // namespace filereader_impl_

class FileReader : public filereader_impl_::BasicFileReader{
private:
	constexpr static const char	*NAME		= "File Reader using standard streams";

public:
	FileReader(std::string_view const filename, char *buffer, size_t const buffer_size, Options const options = DEFAULT_OPTIONS) :
					file_(filename.data()),
					buffer_(buffer),
					buffer_size_(buffer_size),
					options_(options){}

	std::string_view getLine() {
		while( file_.getline(buffer_, static_cast<std::streamsize>(buffer_size_)) ){
			// if we are here, this must be true
			//assert( file_.gcount() > 0 );

			size_t const size = size_t(file_.gcount() - 1);

			if ( applyOptions__(options_, buffer_, size) == false )
				continue;

			return buffer_;
		}

		// return empty line...
		return "";
	}

	operator bool() const{
		return file_.good();
	}

public:
	static const char *name(){
		return NAME;
	}

private:
	std::ifstream	file_;

	char		*buffer_;
	size_t		buffer_size_;

	Options		options_;
};

#endif

