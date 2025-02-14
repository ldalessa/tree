#pragma once

#include "tree/error.hpp"
#include "tree/options.hpp"
#include "tree/radix_split.hpp"
#include "tree/types.hpp"
#include "tree/Key.hpp"

#include <cassert>
#include <algorithm>
#include <ranges>
#include <vector>
#include <utility>

namespace tree
{
	template <class T>
	struct to_key_trait;
	
	struct to_key_fn
	{
		template <class T>
		requires requires (T&& v) {
			typename to_key_trait<std::remove_cvref_t<T>>;
			{ to_key_trait<std::remove_cvref_t<T>>::to_key(v) } -> std::convertible_to<u128>;
		}
		static constexpr auto operator()(T const& v) -> u128 {
			return to_key_trait<std::remove_cvref_t<T>>::to_key(v);
		}
		
		static constexpr auto operator()(Key const& key) -> u128 {
			return key.data();
		}
		
		static constexpr auto operator()(u128 const key) -> u128 {
			return key;
		}
	};

	inline constexpr to_key_fn to_key{};

	template <class T>
	struct GlobBase
	{
		using value_type = T;
		
		u64 _capacity = options::default_glob_capacity;
		std::vector<value_type> _data = _default_data();

		GlobBase() = default;
		GlobBase(stdr::range auto range) {
			_data.insert(_data.begin(), stdr::begin(range), stdr::end(range));
		}

		auto capacity() const {
			return _capacity;
		}
		
		auto size() const {
			return _data.size();
		}
		
		auto begin(this auto& self) {
			return self._data.begin();
		}
		
		auto end(this auto& self) {
			return self._data.end();
		}

		// Create a new glob from the extracted range, erase the tuples
		// from this glob, and return the extracted glob.
		auto extract(this auto& self, stdr::range auto range) -> std::remove_reference_t<decltype(self)>
		{
			auto out = std::remove_reference_t<decltype(self)>(range);
			self.erase(range);
			return out;
		}
		
		// auto extract(this auto& self, u128 key, unsigned const bits)
		// {
		// 	key = mask_msb(key, bits);

		// 	auto const less = [bits](u128 const a, u128 const b) {
		// 		return mask_msb(a, bits) < mask_msb(b, bits);
		// 	};

		// 	// Sort this well enough to extract a single range.
		// 	stdr::sort(self._data, less, to_key);

		// 	// Determine the range to extract, and extract it.
		// 	auto const range = stdr::equal_range(self._data, key, less, to_key);
		// 	return self.extract(range);
		// }

		auto erase(stdr::range auto range) {
			return _data.erase(stdr::begin(range), stdr::end(range));
		}

		auto contains(u128 const key) const -> bool {
			return stdr::find(_data, key, to_key) != _data.end();
		}

		// auto contains(die::Tuple const& tuple) const -> bool {
		// 	return contains(to_key(tuple));
		// }

		auto insert(auto const& tuple) -> bool
		{
			if (_data.size() < _capacity) {
				if constexpr (requires { _data.push_back(tuple); }) {
					_data.push_back(tuple);
				}
				else {
					_data.push_back(to_key(tuple));
				}
				return true;
			}
			return false;
		}

		// auto common_prefix(bool const sorted = false) const -> std::tuple<u128, unsigned>
		// {
		// 	using gmap::common::utils::mask_msb;

		// 	assert(_data.size() != 0);
		// 	if (sorted and options::debug) {
		// 		_verify_sorted();
		// 	}

		// 	auto const [min, max] = _min_max(sorted);
		// 	auto const bits = __builtin_clzg(min ^ max);
		// 	auto const key = mask_msb(min, bits);
		// 	return std::make_tuple(key, bits);
		// }

		enum FIT {
			FIRST,
			BEST,
			MEDIAN
		};

		auto split_point(FIT fit, Key const& key)
		{
			switch (fit) {
			  case FIRST: return radix_split::first(_data, key, std::less<>{}, to_key);
			  case BEST:  return radix_split::best(_data, key, std::less<>{}, to_key);
			  case MEDIAN: throw error("median split unimplemented");
			}
			std::unreachable();
		}
		
	  private:
		static auto _default_data() -> std::vector<value_type> {
			std::vector<value_type> data;
			data.reserve(options::default_glob_capacity);
			return data;
		}

		// auto _min_max(bool const sorted) const -> std::tuple<u128, u128> {
		// 	if (sorted) {
		// 		return std::make_tuple(to_key(_data.front()), to_key(_data.back()));
		// 	}

		// 	u128 min = -1;
		// 	u128 max = 0;
		// 	for (auto const& key : _data | stdv::transform(to_key)) {
		// 		min = std::min(min, key);
		// 		max = std::max(max, key);
		// 	}
		// 	return std::make_tuple(min, max);
		// }

		auto _verify_sorted() const -> void {
			for (auto i = 1zu; i < _data.size(); ++i) {
				assert(to_key(_data[i - 1]) < to_key(_data[i]));
			}
		}
	};

	struct Glob : GlobBase<u128> {
		using GlobBase::GlobBase;
	};
}
