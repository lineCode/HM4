#ifndef DBADAPTER_H_
#define DBADAPTER_H_

#include <type_traits>

#include <sstream>


template<class LIST, class LOADER>
class DBAdapter{
public:
	constexpr static std::false_type IS_MUTABLE{};

public:
	constexpr static size_t DEFAULT_MAX_RESULTS = 50;

public:
	DBAdapter(LIST &list, /* optional */ LOADER *loader, size_t const maxResults = DEFAULT_MAX_RESULTS) :
				list_(list),
				loader_(loader),
				maxResults_(maxResults){}

	std::string get(const StringRef &key) const{
		assert(! key.empty());

		const auto &p = list_[key];

		if (p.valid(/* tomb */ true))
			return p.getVal();
		else
			return {};
	}

	std::vector<std::string> getall(const StringRef &key) const{
		std::vector<std::string> result;

		// reserve x2 because of hgetall
		result.reserve(maxResults_ * 2);

		const auto bit = key.empty() ? list_.begin() : list_.lowerBound(key);
		const auto eit = list_.end();

		size_t c = 0;
		for(auto it = bit; it != eit; ++it){
			result.push_back(it->getKey());

			if (it->valid(/* tomb */ true))
				result.push_back(it->getVal());
			else
				result.emplace_back();

			if (++c >= maxResults_)
				break;
		}

		return result;
	}

	std::string info() const{
		std::stringstream ss;

		ss	<< "Keys (estimated): "	<< list_.size(true)	<< '\n'
			<< "Size: "		<< list_.bytes()	<< '\n'
		;

		return ss.str();
	}

	bool refresh(){
		if (loader_)
			return loader_->refresh();
		else
			return true;
	}

private:
	LIST	&list_;
	LOADER	*loader_;
	size_t	maxResults_;
};


#endif
