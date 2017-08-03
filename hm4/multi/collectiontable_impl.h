namespace hm4{
namespace multi{


template <class CONTAINER>
ObserverPair CollectionTable<CONTAINER>::operator[](const StringRef &key) const{
	assert(container_);
	assert(!key.empty());

	// CONTAINER is responsible for ordering the tables,
	// in correct (probably reverse) order.

	// CONTAINER is responsible for providing goof const Pair &.

	for(const auto &table : *container_ ){
		if (const Pair &pair = table[key])
			return Pair::observer(pair);
	}

	return Pair::zero();
}

// ===================================

template <class CONTAINER>
size_t CollectionTable<CONTAINER>::bytes() const{
	assert(container_);

	size_t result = 0;

	for(const auto &table : *container_ )
		result += table.bytes();

	return result;
}

template <class CONTAINER>
auto CollectionTable<CONTAINER>::sizeEstimated_() const -> size_type{
	assert(container_);

	size_type result = 0;

	for(const auto &table : *container_ )
		result += table.size(true);

	return result;
}

template <class CONTAINER>
auto CollectionTable<CONTAINER>::sizeReal_() const -> size_type{
	// Slooooow....
	size_type result = 0;

	auto const eit = end();
	for(auto it = begin(); it != eit; ++it)
		++result;

	return result;
}


} // namespace multi
} // namespace

