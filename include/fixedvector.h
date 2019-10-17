#ifndef MY_FIXED_VECTOR_H_
#define MY_FIXED_VECTOR_H_

#include <stdexcept>		// std::bad_alloc while push_back
#include <type_traits>		// std::is_trivial, std::is_standard_layout
#include <initializer_list>

//
// Based on
//	http://codereview.stackexchange.com/questions/123402/c-vector-the-basics
//	http://lokiastari.com/blog/2016/03/19/vector-simple-optimizations/
//

template<typename T, std::size_t Size>
class FixedVector{
	static_assert(std::is_trivially_copyable<T>::value,	"T must be trivially copyable type");

public:
	// TYPES

	using value_type	= T;
	using size_type		= std::size_t;
	using difference_type	= std::ptrdiff_t;

	using reference		=       T&;
	using const_reference	= const T&;

	using pointer		=       T*;
	using const_pointer	= const T*;

	using iterator		=       T*;
	using const_iterator	= const T*;

private:
	size_type	length	= 0;
	T		buffer[Size]{};

public:
	// STANDARD C-TORS

	constexpr FixedVector() = default;

	constexpr FixedVector(size_type const count, T const &value) {
		assign(count, value);
	}

	template<class IT>
	constexpr FixedVector(IT begin, IT end){
		assign(begin, end);
	}

	constexpr FixedVector(std::initializer_list<T> const &list) :
		FixedVector(list.begin(), list.end()){}


	// MISC

	constexpr
	void reserve(size_type const) const noexcept{
		// left for compatibility
	}

	constexpr
	void clear() noexcept{
		length = 0;
	}

	// COMPARISSON

	constexpr bool operator==(const FixedVector &other) const noexcept{
		if (length != other.length)
			return false;

		auto first = other.begin();
		auto last  = other.end();
		auto me    = begin();

		for(; first != last; ++first, ++me){
			if ( ! (*first == *me) )
				return false;
		}

		return true;
	}

	template<typename CONTAINER>
	constexpr bool operator!=(const CONTAINER &other) const noexcept{
		return ! operator==(other);
	}

	// ITERATORS

	constexpr
	iterator begin() noexcept{
		return buffer;
	}

	constexpr
	iterator end() noexcept{
		return buffer + length;
	}

	// CONST ITERATORS

	constexpr const_iterator begin() const noexcept{
		return buffer;
	}

	constexpr const_iterator end() const noexcept{
		return buffer + length;
	}

	// C++11 CONST ITERATORS

	constexpr const_iterator cbegin() const noexcept{
		return begin();
	}

	constexpr const_iterator cend() const noexcept{
		return end();
	}

	// Size

	constexpr size_type size() const noexcept{
		return length;
	}

	constexpr bool empty() const noexcept{
		return size() == 0;
	}

	// MORE Size

	constexpr size_type capacity() const noexcept{
		return Size;
	}

	constexpr size_type max_size() const noexcept{
		return Size;
	}

	// DATA

	constexpr
	value_type *data() noexcept{
		return buffer;
	}

	constexpr const value_type *data() const noexcept{
		return buffer;
	}

	// ACCESS WITH RANGE CHECK

	constexpr
	value_type &at(size_type const index){
		validateIndex_(index);
		return buffer[index];
	}

	constexpr const value_type &at(size_type const index) const{
		validateIndex_(index);
		return buffer[index];
	}

	// ACCESS DIRECTLY

	constexpr
	value_type &operator[](size_type const index) noexcept{
		// see [1] behavior is undefined
		return buffer[index];
	}

	constexpr const value_type &operator[](size_type const index) const noexcept{
		// see [1] behavior is undefined
		return buffer[index];
	}

	// FRONT

	constexpr
	value_type &front() noexcept{
		// see [1] behavior is undefined
		return buffer[0];
	}

	constexpr const value_type &front() const noexcept{
		// see [1] behavior is undefined
		return buffer[0];
	}

	// BACK

	constexpr
	value_type &back() noexcept{
		// see [1] behavior is undefined
		return buffer[length - 1];
	}

	constexpr const value_type &back() const noexcept{
		// see [1] behavior is undefined
		return buffer[length - 1];
	}

	// MUTATIONS

	constexpr
	void push_back(const value_type &value){
		emplace_back(value);
	}

	constexpr
	void push_back(value_type &&value){
		emplace_back(std::move(value));
	}

	template<typename... Args>
	constexpr
	void emplace_back(Args&&... args){
		if (length == Size){
			throw std::bad_alloc{};
		}

		buffer[length++] = value_type(std::forward<Args>(args)...);
	}

	constexpr
	void pop_back() noexcept{
		// see [1]
		--length;
	}

public:
	// APPEND

	constexpr
	void assign(size_type const count, T const &value) {
		for(size_type i = 0; i < count; ++i)
			push_back(value);
	}

	template<class IT>
	constexpr
	void assign(IT begin, IT end) {
		for(auto it = begin; it != end; ++it)
			push_back(*it);
	}

	constexpr
	void assign(std::initializer_list<T> const &list){
		assign(list.begin(), list.end());
	}

private:
	constexpr
	void validateIndex_(size_type const index) const{
		if (index >= length){
			throw std::out_of_range("Out of Range");
		}
	}

	// Remark [1]
	//
	// If the container is not empty,
	// the function never throws exceptions (no-throw guarantee).
	// Otherwise, it causes undefined behavior.

};

#endif

