#pragma once

#include "tree/types.hpp"

#include <atomic>
#include <bit>
#include <cstddef>
#include <optional>

namespace tree
{
	template <class T, std::size_t N>
	struct alignas(CACHELINE_SIZE) SPSCQueue
	{
		static_assert(std::has_single_bit(N), "Must provide a power of two.");
		static constexpr u64 _size = N;

		static constexpr auto acquire = std::memory_order::acquire;
		static constexpr auto release = std::memory_order::release;
		
		alignas(CACHELINE_SIZE) // producer stuff
		std::atomic<u64> _head{};
		u64 _tail_cache{};

		alignas(CACHELINE_SIZE) // consumer stuff
		std::atomic<u64> _tail{};
		u64 _head_cache{};

		alignas(CACHELINE_SIZE) // circular buffer data
		T _data[N]{};

		constexpr auto full() const -> bool
		{
			assert(_head - _tail <= _size);
			return _head - _tail == _size;
		}

		constexpr auto empty() const -> bool {
			assert(_head - _tail <= _size);
			return _head - _tail == 0;
		}
		
		constexpr auto push(T const& t) -> bool
		{			
			u64 const head = _head.load(acquire); // read head counter
        
			if (head - _tail_cache == _size) {
				_tail_cache = _tail.load(acquire); // refresh producer's tail
			}

			if (head - _tail_cache == _size) {
				return false;					// queue appears full
			}
			
			_data[head % _size] = std::move(t);	// write slot
			_head.store(head + 1, release);		// publish slot
			return true;
		}

		constexpr auto pop() -> std::optional<T>
		{
			u64 const tail = _tail.load(acquire); // read tail counter
        
			if (_head_cache - tail == 0) {
				_head_cache = _head.load(acquire); // refresh consumer's head
			}

			if (_head_cache - tail == 0) {
				return std::nullopt;			// queue appears empty
			}

			T t = std::move(_data[tail % _size]); // copy-out head data
			_tail.store(tail + 1, release);		  // consume slot
			return t;
		}
	};
}
