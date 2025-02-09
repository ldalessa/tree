#include <cassert>

import tree;
import std;

int main() {
	tree::Key k("1/1");

	std::print("{}\n", k);

	tree::TreeNode<int> node(k);
	{
		auto [a, b] = node.find(k, nullptr);
		assert(not a);
		assert(b and b->key() == k);
	}
	auto& next = node.insert("1/1", 0);
	{
		auto [a, b] = node.find(k, nullptr);
		assert(a);
		assert(a and a->key() == k);
	}
}
