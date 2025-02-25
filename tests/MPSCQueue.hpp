#pragma once

#include "tree/options.hpp"
#include "tree/types.hpp"

#include <blockingconcurrentqueue.h>
#include <concurrentqueue.h>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace tree::tests
{
	template <template <class> class QueueType, class Value>
	struct MPSCQueueImpl
	{
		struct Queue : QueueType<Value> {
			using QueueType<Value>::QueueType;
		};
			
		alignas(64) Queue _queue{};
		std::mutex _token_lock{};

		constexpr MPSCQueueImpl() = default;
		
		constexpr MPSCQueueImpl(u32 n_producers, u32 size)
			: _queue(size * n_producers, n_producers, 0)
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

			constexpr auto try_enqueue(Value const& value) -> void
			{
				while (not this->_queue.try_enqueue(_token, value)) {
					this->stalls += 1;
				}
				this->total += 1;
			}
			
			constexpr auto enqueue(Value const& value) -> void
			{
				while (not this->_queue.enqueue(_token, value)) {
					this->stalls += 1;
				}
				this->total += 1;
			}
			
			constexpr auto enqueue(std::vector<Value> const& values) -> void
			{
				while (not this->_queue.enqueue_bulk(_token, values.data(), values.size())) {
					this->stalls += 1;
				}
				this->total += values.size();
			}
		};

		struct RxEndpoint : Endpoint
		{
			moodycamel::ConsumerToken _token;
			std::unique_ptr<Value[]> _values{new Value[options::default_glob_capacity]};
			std::size_t _size{};
			
			constexpr RxEndpoint(Queue& queue)
				: Endpoint(queue)
				, _token(queue)
			{
			}

			constexpr auto try_dequeue() -> std::optional<Value>
			{
				Value key;
				if (_size != 0) {
					this->total += 1;
					return _values[--_size];
				}
				
				if ((_size = this->_queue.try_dequeue_bulk(_token, _values.get(), options::default_glob_capacity))) {
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

	template <class Value>
	struct MPSCQueue : MPSCQueueImpl<moodycamel::ConcurrentQueue, Value> {
		using MPSCQueueImpl<moodycamel::ConcurrentQueue, Value>::MPSCQueueImpl;
	};

	template <class Value>
	struct MPSCBlockingQueue : MPSCQueueImpl<moodycamel::BlockingConcurrentQueue, Value> {
		using MPSCQueueImpl<moodycamel::BlockingConcurrentQueue, Value>::MPSCQueueImpl;
	};
}
