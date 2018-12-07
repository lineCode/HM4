#include "vectorlist.h"

namespace hm4{

namespace{
	int comp(OPair const &p, StringRef const &key){
		return p.cmp(key);
	};

	template<class T>
	auto binarySearch(T const &v, StringRef const &key){
		return binarySearch(std::begin(v), std::end(v), key, comp);
	}
} // anonymous namespace

#if 0
const Pair *VectorList::operator[](StringRef const &key) const{
	assert(!key.empty());

	auto const x = binarySearch(vector_, key);

	return x.found ? x.it->get() : nullptr;
}
#endif

auto VectorList::find(const StringRef &key, bool const exact) const noexcept -> iterator{
	assert(!key.empty());

	auto const x = binarySearch(vector_, key);

	if (exact == false)
		return x.it;
	else
		return x.found ? x.it : end();
}

bool VectorList::insert(OPair&& newdata){
	assert(newdata);

	const StringRef &key = newdata->getKey();

	auto const x = binarySearch(vector_, key);

	if (x.found){
		// key exists, overwrite, do not shift

		OPair &olddata = vector_[ static_cast<size_type>(x.pos) ];

		// check if the data in database is valid
		if (! newdata->isValid(*olddata) ){
			// newdata will be magically destroyed.
			return false;
		}

		dataSize_ = dataSize_
					- olddata->bytes()
					+ newdata->bytes();

		// copy assignment
		olddata = std::move(newdata);

		return true;
	}

	// key not exists, shift, then add

	try{
		auto const newit = vector_.insert(x.it, std::move(newdata));
		dataSize_ += newit->get()->bytes();
	}catch(...){
	}

	return true;
}

bool VectorList::erase(const StringRef &key){
	assert(!key.empty());

	auto const x = binarySearch(vector_, key);

	if (! x.found){
		// the key does not exists in the vector.
		return true;
	}

	dataSize_ -= x.it->get()->bytes();

	vector_.erase(x.it);

	return true;
}

} // namespace

