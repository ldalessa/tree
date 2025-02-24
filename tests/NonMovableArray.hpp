#pragma once

#include <concepts>
#include <cstddef>
#include <memory>

namespace tree::tests
{
	template <class T>
	struct NonMovableArray
	{
		using allocator = std::allocator<T>;
		using traits = std::allocator_traits<allocator>;

		T* _data{};						  // want the data first for performance
		std::size_t _size{};
		allocator _allocator{};
		
		constexpr ~NonMovableArray()
		{
			if (not _data) {
				return;
			}
			
			for (auto i = 0zu, j = _size - 1; i < _size; ++i, --j) {
				traits::destroy(_allocator, _data + j);
			}
			
			traits::deallocate(_allocator, _data, _size);
		}

		constexpr NonMovableArray() = default;

		template <class... Ts> requires std::constructible_from<T, Ts...>
		constexpr NonMovableArray(std::size_t size, Ts&&... ts)
				: _size(size)
		{
			_data = traits::allocate(_allocator, size);
			assert(_data != nullptr);
			
			for (auto i = 0zu; i < _size; ++i) {
				traits::construct(_allocator, _data + i, ts...);
			}
		}

		template <class... Ts> requires std::constructible_from<T, std::size_t, std::size_t, Ts...>
		constexpr NonMovableArray(std::size_t size, Ts&&... ts)
				: _size(size)
		{
			_data = traits::allocate(_allocator, size);
			assert(_data != nullptr);
			
			for (auto i = 0zu; i < _size; ++i) {
				traits::construct(_allocator, _data + i, i, _size, ts...);
			}
		}

		constexpr NonMovableArray(NonMovableArray const&) = delete;
		constexpr NonMovableArray(NonMovableArray&&) = delete;
		
		constexpr auto size() const -> std::size_t {
			return _size;
		}

		constexpr auto begin(this auto& self) -> auto* {
			return self._data;
		}

		constexpr auto end(this auto& self) -> auto* {
			return self._data + self._size;
		}

		constexpr auto operator[](this auto& self, std::size_t i) -> auto& {
			assert(i < self._size);
			return self._data[i];
		}
	};
}
