#pragma once

#include "tree/types.hpp"

#include <atomic>
#include <barrier>

namespace tree::tests
{
	struct QuiescenceBarrier
	{
		std::barrier<std::function<void()>> _barrier;
		int _quiescent_rounds{};
		std::atomic<int> _active{};

		constexpr QuiescenceBarrier(u32 n_threads)
			: _barrier(n_threads, [&] { _reset_barrier(); })
		{
		}

		constexpr auto operator()(auto str, auto&& f) -> void
		{
			while (join(str, f()));
		}
		
		constexpr auto join(auto str, int active) -> bool
		{
			std::print("{} arriving at barrier active = {}\n", str, active);
			_active += active;
			_barrier.arrive_and_wait();
			return _quiescent_rounds < 2;
		}

	private:
		constexpr auto _reset_barrier() -> void {
			if (_active.exchange(0)) {
				_quiescent_rounds = 0;
			}
			else {
				_quiescent_rounds += 1;
			}
		}
	};
}
