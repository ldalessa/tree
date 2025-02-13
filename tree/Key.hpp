#pragma once

#include "tree/testing.hpp"
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
		u128 _data{};
		u32  _size{};

		constexpr Key() = default;

		constexpr Key(std::same_as<u128> auto x, u32 size = 128)
				: _data(_mask(x, size))
				, _size(size)
		{
		}

		constexpr Key(u64 u, u64 v, u32 size = 128) 
				: Key(((u128)v << 64) | u, size)
		{
		}

		[[gnu::nonnull]]
		constexpr Key(char const* i, char const* e, int base)
		{
			auto [p, ec] = std::from_chars(i, e, _data, 16);
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
				_size = 128u;
			}

			_data = _shift_left(_data, 128u - _size);
			assert(_data == _mask(_data, _size));
		}
	
		template <std::size_t N>
		constexpr Key(char const (&str)[N], int base = 16)
				: Key(std::ranges::begin(str), std::ranges::end(str), base)
		{
		}

		constexpr auto size() const -> u32 {
			return _size;
		}

		constexpr auto operator[](u32 i) const -> u64 {
			assert(i < 128u);
			return _shift_right(_data, 127u - i) & (u64)1;
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
			u128 const c = a._data ^ b._data;
			u32 const n = std::min({_clz(c), a._size, b._size});
			return Key(a._data, n);
		}

	  private:
		static constexpr auto _shift_left(u128 x, u32 bits) -> u128 {
			return bits < 128 ? x << bits : (u128)0;
		}
		
		static constexpr auto _shift_right(u128 x, u32 bits) -> u128 {
			return bits < 128 ? x >> bits : (u128)0;
		}

		static constexpr auto _clz(u128 x) -> u32 {
			return (u32)__builtin_clzg(x, 128);
		}

		static constexpr auto _mask(u128 x, u32 size) -> u128 {
			static constexpr u128 ones = (u128)0 - 1;
			return x & _shift_left(ones, 128u - size);
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

inline auto test_equivalent = tree::testing::test<[]{
	tree::Key a{}, b{};
	assert(a <=> b == std::partial_ordering::equivalent);
	assert(a == b);
	assert(a <= b);
	assert(a >= b);
	assert(not (a < b));
	assert(not (b < a));
	return true;
 }>{};

inline auto test_greater = tree::testing::test<[]{
	tree::Key a("1/1"), b{};
	assert(a <=> b == std::partial_ordering::greater);
	assert(a > b);
	assert(a >= b);
	assert(a != b);
	assert(not (a < b));
	assert(not (a <= b));
	return true;
 }>{};

inline auto test_unordered = tree::testing::test<[]{
	tree::Key a("1/1"), b{"0/1"};
	assert(a <=> b == std::partial_ordering::unordered);
	assert(not (a < b));
	assert(not (a <= b));
	assert(not (a == b));
	assert(not (a >= b));
	assert(not (a > b));
	assert(a != b);
	return true;
 }>{};

inline auto test_less = tree::testing::test<[] {
	tree::Key a("0/1"), b{"0/2"};
	assert(a <=> b == std::partial_ordering::less);
	assert(a < b);
	assert(a <= b);
	assert(not (a == b));
	assert(not (a >= b));
	assert(not (a > b));
	assert(a != b);
	return true;
 }>{};

inline auto test_xor = tree::testing::test<[]{
	tree::Key a("0/2"), b("1/2"), c("0/1");
	assert((a ^ a) == a);
	assert((b ^ b) == b);
	assert((c ^ c) == c);
	assert((a ^ b) == c);
	return true;
 }>{};

#endif
