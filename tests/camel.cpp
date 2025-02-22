#include <concurrentqueue.h>
#include <atomic>
#include <barrier>
#include <cstdio>
// #include <cstdlib>
#include <mutex>
#include <thread>
#include <vector>

[[gnu::noinline, gnu::noipa]]
static void do_not_optimize(auto&&) {}

int main(int, char** argv)
{
	auto const N = std::atoi(argv[1]);
	auto const M = std::atoi(argv[2]);
	auto const S = std::atoi(argv[3]);
	auto const V = std::atoi(argv[4]);

	std::printf("%d producers\n", N);
	std::printf("%d consumers\n", M);
	std::printf("%d capacity\n", S);
	std::printf("%d values per producer\n", V);

	std::vector<std::jthread> threads;
	std::vector<moodycamel::ConcurrentQueue<int>> queues;

	for (int i = 0; i < M; ++i) {
		queues.emplace_back(S, N, 0);
	}

	std::atomic_flag done_producing{};
	std::barrier producer_barrier(N + 1);
	std::barrier consumer_barrier(M + 1);
	std::mutex token_lock{};

	for (int i = 0; i < M; ++i)
	{
		threads.emplace_back([&,id=i]
		{
			// get a token don't know if the lock is necessary
			auto token = [&] {
				auto const _ = std::unique_lock(token_lock);
				return moodycamel::ConsumerToken(queues[i]);
			}();

			consumer_barrier.arrive_and_wait();
			
			int value;
			while (not done_producing.test()) {
				while (queues[id].try_dequeue(token, value)) {
					do_not_optimize(value);
				}
			}

			while (queues[id].try_dequeue(token, value)) {
				do_not_optimize(value);
			}

			consumer_barrier.arrive_and_wait();
		});
	}

	for (int i = 0; i < N; ++i)
	{
		threads.emplace_back([&]
		{
			std::vector<moodycamel::ProducerToken> tokens;
			for (int j = 0; j < M; ++j) {
				auto const _ = std::unique_lock(token_lock);
				tokens.emplace_back(queues[j]);
			}

			producer_barrier.arrive_and_wait();
			
			for (int j = 0; j < V; ++j) {
				while (not queues[j % M].try_enqueue(tokens[j % M], j)) {
				}
			}

			producer_barrier.arrive_and_wait();
		});
	}

	consumer_barrier.arrive_and_wait();
	producer_barrier.arrive_and_wait();
	producer_barrier.arrive_and_wait();
	done_producing.test_and_set();
	consumer_barrier.arrive_and_wait();

	for (auto& thread : threads) {
		thread.join();
	}
}
