#pragma once

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <ranges>

namespace tree
{
	inline 
	namespace types
	{
		template <class T, char... cs>
		inline consteval auto to_literal() -> T
		{
			static constexpr char str[] = { cs..., '\0' }; 
			T x{};
			std::from_chars(str, str + sizeof(str), x);
			return x;
		}
		
#define TREE_TYPES_MAKE_TYPE(type, alias)						\
		using alias = type;										\
																\
		template <char... cs>									\
		inline consteval auto operator ""_##alias() -> alias {	\
			return to_literal<alias, cs...>();					\
		}

		TREE_TYPES_MAKE_TYPE(std::int32_t, i32)
		TREE_TYPES_MAKE_TYPE(std::int64_t, i64)
		TREE_TYPES_MAKE_TYPE(__int128, i128)
		
		TREE_TYPES_MAKE_TYPE(std::uint32_t, u32)
		TREE_TYPES_MAKE_TYPE(std::uint64_t, u64)
		TREE_TYPES_MAKE_TYPE(unsigned __int128, u128)

		TREE_TYPES_MAKE_TYPE(std::size_t, uz)
		TREE_TYPES_MAKE_TYPE(std::ptrdiff_t, z)
		
#undef TREE_TYPES_MAKE_TYPE
	}

	static constexpr std::size_t CACHELINE_SIZE = 64;

	namespace stdr = std::ranges;
	namespace stdv = std::views;
}
