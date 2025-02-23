#pragma once

#include "tree/tree.hpp"
#include <blockingconcurrentqueue.h>
#include <concurrentqueue.h>
#include <mutex>
#include <utility>

namespace tree::tests
{
	template <template <class> class QueueType>
	struct MPSCQueueImpl
	{
		struct Queue : QueueType<u128> {
			using QueueType<u128>::QueueType;
		};
			
		alignas(64) Queue _queue{};
		std::mutex _token_lock{};

		constexpr MPSCQueueImpl() = default;
		
		constexpr MPSCQueueImpl(u32 n_producers, u32 size)
			: _queue(size, n_producers, 0)
		{
		}

		constexpr auto size() const {
			return _queue.size_approx();
		}

		struct Endpoint
		{
			Queue& _queue;
			u64 stalls{};
			u64 total{};

			constexpr Endpoint(Queue& queue) : _queue(queue) {}

			constexpr auto size() const -> u64 {
				return _queue.size_approx();
			}
		};

		struct TxEndpoint : Endpoint
		{
			moodycamel::ProducerToken _token;
			
			constexpr TxEndpoint(Queue& queue)
				: Endpoint(queue)
				, _token(queue)
			{
			}

			constexpr auto enqueue(u128 key) -> void
			{
				while (not this->_queue.enqueue(_token, key)) {
					this->stalls += 1;
				}
				this->total += 1;
			}
			
			constexpr auto enqueue(Glob const& glob) -> void
			{
				while (not this->_queue.enqueue_bulk(_token, glob.begin(), glob.size())) {
					this->stalls += 1;
				}
				this->total += 1;
			}
		};

		struct RxEndpoint : Endpoint
		{
			moodycamel::ConsumerToken _token;
			std::unique_ptr<u128[]> _keys{new u128[options::default_glob_capacity]};
			std::size_t _size{};
			
			constexpr RxEndpoint(Queue& queue)
				: Endpoint(queue)
				, _token(queue)
			{
			}

			constexpr auto try_dequeue() -> std::optional<u128>
			{
				if (_size != 0) {
					this->total += 1;
					return _keys[--_size];
				}
				
				if ((_size = this->_queue.try_dequeue_bulk(_token, _keys.get(), options::default_glob_capacity))) {
					return try_dequeue();
				}

				this->stalls += 1;
				return std::nullopt;
			}
		};

		constexpr auto get_tx_endpoint() -> TxEndpoint {
			auto const _ = std::scoped_lock(_token_lock);
			return TxEndpoint(_queue);
		}

		constexpr auto get_rx_endpoint() -> RxEndpoint {
			auto const _ = std::scoped_lock(_token_lock);
			return RxEndpoint(_queue);
		}
	};

	struct MPSCQueue : MPSCQueueImpl<moodycamel::ConcurrentQueue> {
		using MPSCQueueImpl::MPSCQueueImpl;
	};

	struct BlockingMPSCQueue : MPSCQueueImpl<moodycamel::BlockingConcurrentQueue> {
		using MPSCQueueImpl::MPSCQueueImpl;
	};
}
