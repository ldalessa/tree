#define TREE_TESTING
#undef DNDEBUG

#include "tree/tree.hpp"
#include <CLI/App.hpp>
#include <thread>

using namespace tree;

static auto const test_queue = testing::test<[]
 {
	 auto queue = SPSCQueue<Key, 1>();
	 auto const key = Key(1, 1);
	 auto const r = queue.push(key);
	 assert(r);
	 auto const keyʹ = queue.pop();
	 assert(keyʹ);
	 assert(key == *keyʹ);
	 return true;
 }>{};

static auto const test_queue_concurrent = testing::test<[]
 {
	 auto queue = SPSCQueue<int, 1>();

	 {
		 auto producer = std::jthread([&] {
			 auto const b = queue.push(1);
			 assert(b);
		 });

		 auto consumer = std::jthread([&] {
			 while (true) {
				 if (auto const i = queue.pop()) {
					 assert(*i == 1);
					 break;
				 }
			 }
		 });
	 
		 consumer.join();
		 producer.join();
	 }

	 {
		 auto producer = std::jthread([&] {
			 for (int n = 0; n < 10; ++n) {
				 while (not queue.push(n)) {
				 }
			 }
		 });
		 
		 auto consumer = std::jthread([&] {
			 for (int n = 0; n < 10; ++n) {
				 while (true) {
					 if (auto const m = queue.pop()) {
						 assert(*m == n);
						 break;
					 }
				 }
			 }
		 });

		 consumer.join();
		 producer.join();
	 }

	 return true;
 }>{};

int main(int argc, char* argv[]) {
	CLI::App app;
	argv = app.ensure_utf8(argv);
	options::process_command_line(app);
	CLI11_PARSE(app, argc, argv);
	testing::run_all_tests();
}
