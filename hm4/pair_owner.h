#ifndef PAIR_OWNER_H
#define PAIR_OWNER_H

#include "pair_pod.h"

namespace hm4{

class OPair{
public:
	OPair(const StringRef &key, const StringRef &val, uint32_t expires = 0, uint32_t created = 0) :
						pp( Pair::create( key, val, expires, created) ){}

	OPair(const Pair *p) : pp( Pair::create( p) ){}
	OPair(const Pair &p) : pp( Pair::create(&p) ){}

	// ==============================

	OPair(OPair &&other) = default;
	OPair &operator=(OPair &&other) = default;

	OPair(const OPair &other) : OPair( other.pp.get() ){}

	OPair &operator=(const OPair &other){
		OPair pair = other;

		swap(pair);

		return *this;
	}

	// ==============================

	static OPair tombstone(const StringRef &key){
		return OPair(key, ""_sr );
	}

	void swap(OPair &other) noexcept{
		using std::swap;

		swap(pp, other.pp);
	}

	operator bool() const noexcept{
		return static_cast<bool>(pp);
	}

	const Pair &operator *() const noexcept{
		return *pp;
	}

	const Pair *operator ->() const noexcept{
		return get();
	}

	const Pair *get() const noexcept{
		return pp.get();
	}

private:
	std::unique_ptr<const Pair>	pp;
};

// ==============================

inline void swap(OPair &p1, OPair &p2) noexcept{
	p1.swap(p2);
}

} // namespace

#endif
