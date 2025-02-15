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
		using value_type = u128;
		
		u64 _capacity = options::default_glob_capacity;
		std::vector<value_type> _data = _default_data();

		Glob() = default;

		Glob(stdr::range auto range) {
			_data.insert(_data.begin(), stdr::begin(range), stdr::end(range));
		}
		
		auto size() const {
			return _data.size();
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

		auto contains(Key const& key) const -> bool {
			assert(key.size() == Key::max_size());
			return stdr::find(_data, key.data()) != _data.end();
		}

		auto insert(Key const& key) -> bool
		{
			assert(key.size() == Key::max_size());
			if (_data.size() < _capacity) {
				_data.push_back(key.data());
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
		static auto _default_data() -> std::vector<value_type> {
			std::vector<value_type> data;
			data.reserve(options::default_glob_capacity);
			return data;
		}
	};
}
