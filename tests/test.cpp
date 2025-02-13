#define TREE_TESTING
#undef DNDEBUG

#include "tree/tree.hpp"

#include <CLI/App.hpp>
#include <omp.h>

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
	 omp_set_num_threads(2);
	 auto queue = SPSCQueue<int, 1>();

// #pragma omp parallel shared(queue)
// 	 {
// 		 int const i = omp_get_thread_num();
// 		 if (i == 0) {
// 			 auto const b = queue.push(1);
// 			 assert(b);
// 		 }
// 		 else {
// 			 while (true) {
// 				 if (auto const i = queue.pop()) {
// 					 assert(*i == 1);
// 					 break;
// 				 }
// 			 }
// 		 }
// 	 }
	 
#pragma omp parallel shared(queue)
	 {
		 int const i = omp_get_thread_num();
		 if (i == 0) {
			 for (int n = 0; n < 10; ++n) {
				 while (queue.push(n)) {
					 std::print("{} waiting to push {}\n", i, n);
				 }
				 std::print("{} pushed {}\n", i, n);
			 }
		 }
		 else {
			 for (int n = 0; n < 10; ++n) {
				 while (true) {
					 if (auto const m = queue.pop()) {
						 std::print("{} popped {}, expected {}\n", i, *m, n);
						 std::fflush(stdout);
						 assert(*m == n);
						 break;
					 }
					 std::print("{} waiting to pop {}\n", i, n);
				 }
			 }
		 }
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
