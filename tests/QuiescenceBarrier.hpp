#pragma once

#include "tree/types.hpp"

#include <atomic>
#include <barrier>

namespace tree::tests
{
	struct QuiescenceBarrier
	{
		std::barrier<std::function<void()>> _barrier;
		std::atomic_flag _flag{};
		bool _state = true;

		constexpr QuiescenceBarrier(u32 n_threads)
			: _barrier(n_threads, [&] { _reset_barrier(); })
		{
		}

		constexpr auto operator()(auto&& f) -> void
			requires requires { {f()} -> std::same_as<bool>; }
		{
			while (join(f()));
		}
		
		constexpr auto join(bool active) -> bool
		{
			if (active) {
				_flag.test_and_set();
			}
			
			_barrier.arrive_and_wait();
			
			return _state;
		}

	private:
		constexpr auto _reset_barrier() -> void {
			_state = _flag.test();
			_flag.clear();
		}
	};
}
