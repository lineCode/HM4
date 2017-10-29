#ifndef MY_STRING_REF_H_
#define MY_STRING_REF_H_

#include <cstring>
#include <string>
#include <ostream>

#include <initializer_list>

class StringRef{
private:
	constexpr static bool COMPARE_MICRO_OPTIMIZATIONS = true;

public:
	// CONSTRUCTORS

	constexpr StringRef() = default;

	constexpr StringRef(const char *data, size_t const size);

	constexpr StringRef(const char *data);

	StringRef(const std::string &s);

	explicit StringRef(std::string &&){
		throw std::logic_error("std::string will decay and you do not want this");
	}

	// DATA MEMBERS

	constexpr const char *data() const noexcept{
		return data_;
	}

	constexpr size_t size() const noexcept{
		return size_;
	}

	constexpr size_t length() const noexcept{
		return size_;
	}

	constexpr bool empty() const noexcept{
		return size_ == 0;
	}

	// ITERATORS

	constexpr const char &operator [] (size_t index) const noexcept{
		return data_[index];
	}

	constexpr const char *begin() const noexcept{
		return data_;
	}

	constexpr const char *end() const noexcept{
		return data_ + size_;
	}

	// COMPARES

	int compare(const char *data, size_t size) const noexcept{
		return compare(data_, size_, data, size);
	}

	int compare(const char *data) const noexcept{
		return compare(data, strlen__(data) );
	}

	int compare(const std::string &s) const noexcept{
		return compare(s.data(), s.size() );
	}

	int compare(const StringRef &sr) const noexcept{
		return compare(sr.data(), sr.size() );
	}

	// EQUALS

	constexpr bool equals(const char *data, size_t size) const noexcept{
		return equals(data_, size_, data, size);
	}

	constexpr bool operator ==(const char *data) const noexcept{
		return equals(data, strlen__(data) );
	}

	// constexpr - CentOS gcc / clang complains
	bool operator ==(const std::string &data) const noexcept{
		return equals(data.data(), data.size() );
	}

	constexpr bool operator ==(const StringRef &data) const noexcept{
		return equals(data.data(), data.size() );
	}

	constexpr bool operator ==(char const c) const noexcept{
		return size_ == 1 && data_[0] == c;
	}

	// NOT EQUALS

	constexpr bool operator !=(const char *data) const noexcept{
		return ! (*this == data);
	}

	// constexpr - CentOS gcc / clang complains
	bool operator !=(const std::string &data) const noexcept{
		return ! (*this == data);
	}

	constexpr bool operator !=(const StringRef &data) const noexcept{
		return ! (*this == data);
	}

	constexpr bool operator !=(char data) const noexcept{
		return ! (*this == data);
	}

	// CASTS

	operator std::string() const{
		return std::string(data_, size_);
	}

	constexpr const char *c_str() const noexcept{
		return data();
	}

public:
	// HASH CHAR * HELPERS

	constexpr static uint32_t hash(const char* data, size_t const size) noexcept;

	constexpr static auto hash(const StringRef &data) noexcept{
		return hash(data.data(), data.size() );
	}

	constexpr static auto hash(const char* data) noexcept{
		return hash(data, strlen__(data) );
	}

	// HASH

	constexpr auto hash() const noexcept{
		return hash(*this);
	}

public:
	// CHAR * HELPERS

	static int compare(const char *s1, size_t const size1, const char *s2, size_t const size2) noexcept;

	constexpr
	static bool equals(const char *s1, size_t const size1, const char *s2, size_t const size2) noexcept{
		return equals__(s1, size1, s2, size2);
	}

	constexpr static bool fastEmptyChar(const char* s){
		return s == nullptr ? true : s[0] == 0;
	}

	constexpr static bool fastEmptyChar(const char* s, size_t const size){
		return s == nullptr ? true : size == 0;
	}

	static std::string concatenate(const std::initializer_list<StringRef> &args);
	static const std::string &concatenate(std::string &buffer, const std::initializer_list<StringRef> &args);

private:
	size_t		size_	= 0;
	const char	*data_	= "";

private:
	static int compare__(const char *s1, size_t size1, const char *s2, size_t size2) noexcept;
	constexpr
	static bool equals__(const char *s1, size_t size1, const char *s2, size_t size2) noexcept;

	static int memcmp__( const void *s1, const void *s2, size_t const n) noexcept;
	constexpr static size_t strlen___(const char *s) noexcept;
	constexpr static size_t strlen__(const char *s) noexcept;
	constexpr static const char *strptr__(const char *s) noexcept;

	static size_t concatenateSize_(const std::initializer_list<StringRef> &args);

	template<typename T>
	static int sgn__(const T &a, const T &b) noexcept;
};

inline std::ostream& operator << (std::ostream& os, const StringRef &sr){
	// cast because of clang
	//return os.write(sr.data(), static_cast<std::streamsize>( sr.size() ));
	// almost the same, but std::setw() works
	return __ostream_insert(os, sr.data(), static_cast<std::streamsize>( sr.size() ));
}

// need for standard algoritms

inline bool operator ==(const std::string &s, const StringRef &sr){
	return sr == s;
}

inline bool operator <(const std::string &s, const StringRef &sr){
	return sr.compare(s) >= 0;
}

// user defined literals

#ifdef __cpp_user_defined_literals

constexpr StringRef operator "" _sr(const char *name, size_t const size){
	return StringRef{ name, size };
}

#endif

// ==================================
// ==================================
// ==================================

constexpr inline StringRef::StringRef(const char *data, size_t const size) :
		size_(size),
		data_(strptr__(data)){}

constexpr inline StringRef::StringRef(const char *data) :
		StringRef(data, strlen__(data)){}

inline StringRef::StringRef(const std::string &s) :
		size_(s.size()),
		data_(s.data()){}

// ==================================
// HASH HELPERS
// ==================================

constexpr uint32_t StringRef::hash(const char* data, size_t const size) noexcept{
	uint32_t hash = 5381;

	for(const char *c = data; c < data + size; ++c)
		hash = ((hash << 5) + hash) + *c;

	return hash;
}

// ==================================
// CHAR * HELPERS
// ==================================

inline int StringRef::compare(const char *s1, size_t const size1, const char *s2, size_t const size2) noexcept{
	if (COMPARE_MICRO_OPTIMIZATIONS){
		if (s1 == s2 && size1 == size2)
			return 0;
	}

	return compare__(s1, size1, s2, size2);
}


inline size_t concatenateSize__(const std::initializer_list<StringRef> &args){
	size_t size = 0;

	for(const auto &sr : args)
		size += sr.size();

	return size;
}

inline std::string StringRef::concatenate(const std::initializer_list<StringRef> &args){
	// super cheap concatenation,
	// with single allocation

	std::string s;

	concatenate(s, args);

	return s;
}

inline const std::string &StringRef::concatenate(std::string &s, const std::initializer_list<StringRef> &args){
	// super cheap concatenation,
	// sometimes without allocation

	// seems no need to save space for NULL terminator.
	size_t const reserve_size = concatenateSize__(args);

	s.clear();

	// reserve() will shrink capacity
	if (reserve_size > s.capacity())
		s.reserve(reserve_size);

	for(const auto &sr : args)
		s.append(sr.data(), sr.size());

	return s;
}

// ==================================
// PRIVATE
// ==================================

inline int StringRef::compare__(const char *s1, size_t const size1, const char *s2, size_t const size2) noexcept{
	// First idea was lazy based on LLVM::StringRef
	// http://llvm.org/docs/doxygen/html/StringRef_8h_source.html

	if ( int const res = memcmp__(s1, s2, std::min(size1, size2) ) )
		return res; // most likely exit

	// sgn helps convert size_t to int, without a branch
	return sgn__(size1, size2);
}

constexpr inline bool StringRef::equals__(const char *s1, size_t const size1, const char *s2, size_t const size2) noexcept{
	// Here clang do constexpr as follows -
	// it checks the sizes and short cut memcmp__().
	// There is *NO* constexpr, if you supply same sized strings:
	// StringRef::equals__("Hello", 5, "Bello", 5);

	// Idea based on LLVM::StringRef
	// http://llvm.org/docs/doxygen/html/StringRef_8h_source.html
	return size1 == size2 && memcmp__(s1, s2, size1) == 0;
}

// ==================================

inline int StringRef::memcmp__(const void *s1, const void *s2, size_t const n) noexcept{
//	return __builtin_memcmp(s1, s2, n);
	return memcmp(s1, s2, n);
}

constexpr inline size_t StringRef::strlen___(const char *s) noexcept{
	// __builtin_strlen is constexpr in clang
	return __builtin_strlen(s);
}

constexpr inline size_t StringRef::strlen__(const char *s) noexcept{
	return s ? strlen___(s) : 0;
}

constexpr inline const char *StringRef::strptr__(const char *s) noexcept{
	return s ? s : "";
}

// ==================================

template<typename T>
int StringRef::sgn__(const T &a, const T &b) noexcept{
	return (a > b) - (a < b);
}

#endif

