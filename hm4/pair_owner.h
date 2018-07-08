#ifndef PAIR_OWNER_H
#define PAIR_OWNER_H

#include "pair_pod.h"

#include "smallstring.h"

#include <type_traits>

namespace hm4{

class OPair{
public:
	OPair(const StringRef &key, const StringRef &val, uint32_t expires = 0, uint32_t created = 0) :
				hkey(key),
				pp( Pair::create(key, val, expires, created) ){}

	OPair(const Pair *p) :
				hkey(p ? p->getKey() : StringRef{}),
				pp( Pair::create(p) ){}

	OPair(const Pair &p) :
				hkey(p.getKey()),
				pp( Pair::create(p) ){}

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

		swap(hkey,	other.hkey	);
		swap(pp,	other.pp	);
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

public:
	int cmp(const StringRef &key) const noexcept{
		if (key.empty())
			return Pair::CMP_NULLKEY;

		int const r = hkey.compare(key);

		if (r || key.size() <= hkey.capacity())
			return r;

		return pp->cmp(key);
	}

private:
	SmallString<PairConf::HLINE_SIZE>	hkey;
	std::unique_ptr<const Pair>		pp;
};

//static_assert( std::is_trivially_copyable<OPair>::value, "OPair is not trivially copyable" );

// ==============================

inline void swap(OPair &p1, OPair &p2) noexcept{
	p1.swap(p2);
}

inline void print(const OPair &pair){
	print(*pair);
}

} // namespace

#endif

