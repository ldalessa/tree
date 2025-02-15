#pragma once

#include "tree/types.hpp"
#include "tree/Key.hpp"

#include <atomic>
#include <cassert>
#include <climits>
#include <concepts>
#include <ranges>

namespace tree
{
	struct radix_split
	{
		static inline std::atomic<u64> count_splits{};

		template <class It>
		struct split_result {
			stdr::subrange<It> range;
			Key key;

			constexpr auto size() const {
				return stdr::size(range);
			}
		};
	
		template <stdr::bidirectional_range V, class Compare = std::less<void>, class Project = std::identity>
		requires stdr::sized_range<V> and stdr::common_range<V>
		static constexpr auto best(V&& view, Key const& key, Compare&& cmp = {}, Project&& proj = {})
			-> split_result<stdr::iterator_t<V>>
		{
			assert(stdr::size(view) != 0);
			auto const target = std::max(stdr::size(view) / 2, 1zu);
			auto stop = [=](auto const n) {
				return n <= target;
			};
			return _split(stdr::begin(view), stdr::end(view), key, std::forward<Compare>(cmp), std::forward<Project>(proj), std::move(stop));
		}

		template <stdr::bidirectional_range V, class Compare = std::less<void>, class Project = std::identity>
		requires stdr::sized_range<V> and stdr::common_range<V>
		static constexpr auto first(V&& view, Key const& key, Compare&& cmp = {}, Project&& proj = {})
			-> split_result<stdr::iterator_t<V>>
		{
			assert(stdr::size(view) != 0);
			auto const target = stdr::size(view);
			auto stop = [=](auto const n) {
				return n < target;
			};
			return _split(stdr::begin(view), stdr::end(view), key, std::forward<Compare>(cmp), std::forward<Project>(proj), std::move(stop));
		}
	
	  private:
		template <class It>
		static constexpr auto _split(It const i, It const j, Key const& key, auto&& cmp, auto&& proj, auto&& stop)
			-> split_result<It>
		{
			static_assert(Key::max_size() <= sizeof(proj(*i)) * CHAR_BIT);
			assert(i <= j);
			
			// If the current range is less than the target, then terminate recursion
			// and return the current details.
			auto const n = stdr::distance(i, j);
			if (stop(n)) {
				return split_result(stdr::subrange(i, j), key);
			}
			
			assert(n != 0);
			
			if (not std::is_constant_evaluated()) {
				count_splits += 1;
			}

			// Figure out where the first 1 is in the sort, and recursively test the two
			// partitions we just created.
			auto const k = _split_point(i, std::prev(j), key, cmp, proj);
			auto const l = _split(i, k, key | 0, cmp, proj, stop);
			auto const r = _split(k, j, key | 1, cmp, proj, stop);

			// Return whichever partition does a better job matching the target.
			return l.size() <= r.size() ? r : l;
		}

		// Get the `b`th most significant bit of a value to 1.
		template <class T>
		static constexpr auto _get_msb(T const x, u32 const b) -> int {
			return (bool)((x >> (sizeof(x) * CHAR_BIT - 1 - b)) & T(1));
		}
	
		// Scans through the closed range `[i,j]` and returns an iterator `k` such
		// that either the `b` most-significant bit is `1`, or `k == j + 1`.
		template <std::bidirectional_iterator S>
		static constexpr auto _split_point(S i, S j, Key const& key, auto& cmp, auto& proj) -> S
		{
			u32 const b = key.size();
			
			while (cmp(i, j)) {
				if (_get_msb(proj(*i), b) == 0) { ++i; continue; }
				if (_get_msb(proj(*j), b) == 1) { --j; continue; }
				std::iter_swap(i++, j--);
			}

			return _get_msb(proj(*i), b) == 1 ? i : _get_msb(proj(*j), b) == 1 ? j : std::next(j);
		}
	};
}

#ifdef TREE_TESTING

#include "tree/testing.hpp"

namespace tree::testing
{
	inline const auto test_radix_split_one = cetest<[]
	{
		{
			auto data = std::array { 0_u128 };
		
			auto [rng, a] = radix_split::best(data, "0/0");
			assert(rng.size() == 1);
			assert(a == Key("0/0"));
		
			return true;
		}
		{
			auto data = std::array { 1_u128 };
			auto [rng, b] = radix_split::best(data, "0/0");
			assert(rng.size() == 1);
			assert(b == Key("0/0"));
		
			return true;
		}
	}>{};
}

#endif
