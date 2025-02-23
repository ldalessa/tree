#pragma once

#include "tree/error.hpp"
#include "exit_scope.hpp"
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
	struct Glob
	{
		u64 _capacity = options::default_glob_capacity;
		std::vector<u128> _data = _default_data();

		Glob() = default;

		Glob(stdr::range auto range) {
			_data.insert(_data.begin(), stdr::begin(range), stdr::end(range));
		}
		
		auto size() const {
			return _data.size();
		}

		auto take_all() -> std::vector<u128> {
			return std::move(_data);
		}
		
		auto extract(stdr::range auto range) -> Glob
		{
			auto const _ = exit_scope([&] {
				erase(range);
			});
			return Glob(range);
		}
		
		auto erase(stdr::range auto range) {
			return _data.erase(stdr::begin(range), stdr::end(range));
		}

		auto contains(u128 key) const -> bool {
			return stdr::find(_data, key) != _data.end();
		}

		auto insert(u128 key) -> bool
		{
			if (_data.size() < _capacity) {
				_data.push_back(key);
				return true;
			}
			return false;
		}

		auto split_point(Fit fit, Key const& key)
		{
			switch (fit) {
			  case FIRST: return radix_split::first(_data, key);
			  case  BEST: return radix_split::best(_data, key);
			  default: throw error("median split unimplemented");
			}
		}
		
		auto factor(Key& key)
		{
			auto const shift = Key::max_size() - options::factor;
			assert(shift <= key.size());
			
			// Only really need to sort this to options::factor bits
			stdr::sort(_data);

			// Determine the range to extract, and extract it.
			u128 const v = key.data() >> shift;
			auto const range = stdr::equal_range(_data, v, std::less{}, [&](u128 const& x) -> u128 {
				return x >> shift;
			});
			
			assert(stdr::size(range) != 0);

			auto const a = Key(*stdr::begin(range), options::factor);
			auto const b = Key(*stdr::rbegin(range), options::factor);
			key = a ^ b;
			return range;
		}
		
	  private:
		static auto _default_data() -> std::vector<u128> {
			std::vector<u128> data;
			data.reserve(options::default_glob_capacity);
			return data;
		}
	};
}
