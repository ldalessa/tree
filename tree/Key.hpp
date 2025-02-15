#pragma once

#include "tree/types.hpp"

#include <cassert>
#include <charconv>
#include <compare>
#include <concepts>
#include <cstddef>
#include <format>
#include <ranges>
#include <system_error>

namespace tree
{  
	struct Key 
	{
		static constexpr u32 _max_size = 128u;
		
		u128 _data{};
		u32  _size{};

		constexpr Key() = default;

		constexpr Key(std::same_as<u128> auto x, u32 size = _max_size)
				: _data(_mask(x, size))
				, _size(size)
		{
		}

		constexpr Key(u64 u, u64 v, u32 size = _max_size) 
				: Key(((u128)v << 64) | u, size)
		{
		}

		[[gnu::nonnull]]
		constexpr Key(char const* i, char const* e, int base)
		{
			auto [p, ec] = std::from_chars(i, e, _data, base);
			switch (ec) {
			  default: break;
			  case std::errc::invalid_argument: assert(false);
			  case std::errc::result_out_of_range: assert(false);
			}

			if (p != e) {
				assert(*p == '/');
				i = p + 1;
				auto [_, ec] = std::from_chars(i, e, _size);
				switch (ec) {
				  default: break;
				  case std::errc::invalid_argument: assert(false);
				  case std::errc::result_out_of_range: assert(false);
				}
			}
			else {
				_size = _max_size;
			}

			_data = _shift_left(_data, _max_size - _size);
			assert(_data == _mask(_data, _size));
		}
	
		template <uz N>
		constexpr Key(char const (&str)[N], int base = 16)
				: Key(stdr::begin(str), stdr::end(str), base)
		{
		}

		static constexpr auto max_size() -> u32 {
			return _max_size;
		}
		
		constexpr auto size() const -> u32 {
			return _size;
		}

		constexpr auto data() const -> u128 {
			return _data;
		}

		constexpr auto operator[](u32 i) const -> u64 {
			assert(i < _max_size);
			return _get(_data, i);
		}
		
		constexpr friend auto operator==(Key, Key) -> bool = default;

		constexpr friend auto operator<=>(Key a, Key b) -> std::partial_ordering
		{
			if (a._size < b._size and a._matches_prefix(b._data)) {
				return std::partial_ordering::less;
			}

			if (b._size < a._size and b._matches_prefix(a._data)) {
				return std::partial_ordering::greater;
			}

			if (a == b) {
				return std::partial_ordering::equivalent;
			}

			return std::partial_ordering::unordered;
		}

		constexpr friend auto operator^(Key a, Key b) -> Key
		{
			auto const c = a._data ^ b._data;
			auto const n = std::min({_clz(c), a._size, b._size});
			return Key(a._data, n);
		}

		/// Append @p bit to the key and extend its size by one.
		//
		// @precondition size() != _max_size
		// @precondition bit == 0 or bit == 1
		//
		// @param the value (0 or 1) to append
		// @returns the extended key
		constexpr auto operator|(u32 bit) const -> Key {
			assert(_size < _max_size);
			assert(bit == 0_u32 or bit == 1_u32);
			return Key(_set(_data, _size, bit), _size + 1);
		}

		// This compares the first bit that diverges between a and b.
		constexpr friend auto less(Key const& a, Key const& b) -> bool
		{
			assert(a != b);
			auto const c = a ^ b;
			auto const n = c.size();
			return a[n] < b[n];
		}
		
	  private:
		static constexpr auto _shift_left(u128 x, u32 bits) -> u128 {
			return bits < 128_u32 ? x << bits : 0_u128;
		}
		
		static constexpr auto _shift_right(u128 x, u32 bits) -> u128 {
			return bits < 128_u32 ? x >> bits : 0_u128;
		}

		static constexpr auto _clz(u128 x) -> u32 {
			return (u32)__builtin_clzg(x, 128);
		}

		static constexpr auto _mask(u128 x, u32 size) -> u128 {
			static constexpr u128 ones = 0_u128 - 1;
			return x & _shift_left(ones, _max_size - size);
		}

		static constexpr auto _msb_to_lsb(u32 i) -> u32
		{
			// i = 0 -> 127
			// i = 1 -> 126
			// i = 127 -> 0
			// i = 128 == error
			return  _max_size - i - 1;
		}
		
		static constexpr auto _get(u128 data, u32 i) -> u64 {
			auto const offset = _msb_to_lsb(i);
			return _shift_right(data, offset) & 1_u64;
		}

		static constexpr auto _set(u128 data, u32 i, u128 value) -> u128 {			
			auto const offset = _msb_to_lsb(i);
			auto const mask = _shift_left(value, offset);
			return (data & ~mask) | mask;
		}
		
		constexpr auto _matches_prefix(u128 x) const -> bool {
			return _data == _mask(x, _size);
		}
	};
} // namespace tree

template <>
struct std::formatter<tree::Key>
{
	tree::u32 _n{};

	constexpr auto parse(auto& ctx) -> decltype(ctx.begin()) const {
		if (auto i = ctx.begin(); i != ctx.end() and *i != '}') {
			assert(*i != ':');
			auto e = std::ranges::find(i, ctx.end(), '}');
			assert(e != ctx.end());
			auto [_, ec] = std::from_chars(i, e, _n);
			switch (ec) {
			  case std::errc::invalid_argument: assert(false);
			  case std::errc::result_out_of_range: assert(false);
			  default: return e;
			}
		}
		else {
			return i;
		}
	}

	constexpr auto format(tree::Key a, auto& ctx) const {
		if (a._size == 0) {
			return std::format_to(ctx.out(), "0x0/0");
		}

		tree::u32 n = (a._size/ 4) + !!(a._size % 4);
		
		if (_n > n) {
			n = _n;
		}

		tree::u32 const m = n * 4;

		return std::format_to(ctx.out(), "0x{:0{}x}/{}", a._data >> (128u - m), n, a._size);
	}
};

#ifdef TREE_TESTING

#include "tree/testing.hpp"

namespace tree::testing
{

	inline auto test_equivalent = cetest<[]{
		Key a{}, b{};
		assert(a <=> b == std::partial_ordering::equivalent);
		assert(a == b);
		assert(a <= b);
		assert(a >= b);
		assert(not (a < b));
		assert(not (b < a));

		Key c{0, 0x8000000000000000, 1}, d{"1/1"};
		assert(c == d);
		return true;
	}>{};

	inline auto test_greater = cetest<[]{
		Key a("1/1"), b{};
		assert(a <=> b == std::partial_ordering::greater);
		assert(a > b);
		assert(a >= b);
		assert(a != b);
		assert(not (a < b));
		assert(not (a <= b));
		return true;
	}>{};

	inline auto test_unordered = cetest<[]{
		Key a("1/1"), b{"0/1"};
		assert(a <=> b == std::partial_ordering::unordered);
		assert(not (a < b));
		assert(not (a <= b));
		assert(not (a == b));
		assert(not (a >= b));
		assert(not (a > b));
		assert(a != b);
		return true;
	}>{};

	inline auto test_less = cetest<[]{
		Key a("0/1"), b{"0/2"};
		assert(a <=> b == std::partial_ordering::less);
		assert(a < b);
		assert(a <= b);
		assert(not (a == b));
		assert(not (a >= b));
		assert(not (a > b));
		assert(a != b);
		return true;
	}>{};

	inline auto test_xor = cetest<[]{
		Key a("0/2"), b("1/2"), c("0/1");
		assert((a ^ a) == a);
		assert((b ^ b) == b);
		assert((c ^ c) == c);
		assert((a ^ b) == c);
		return true;
	}>{};

	inline const auto test_extend = cetest<[]{
		Key a("0/0");

		auto const b = a | 0;
		assert(b == Key("0/1"));

		auto const c = b | 1;
		assert(c == Key("01/2", 2));

		auto const d = c | 0;
		assert(d == Key("010/3", 2));

		auto const e = d | 1;
		assert(e == Key("0101/4", 2));
		
		return true;
	}>{};
}

#endif
