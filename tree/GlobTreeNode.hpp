#pragma once

#include "tree/exit_scope.hpp"
#include "tree/types.hpp"
#include "tree/Glob.hpp"
#include "tree/Key.hpp"

#include <cassert>
#include <memory>
#include <optional>

namespace tree
{
	struct GlobTreeNode
	{
		Key const _key;
		std::optional<Glob> _glob{};
		std::unique_ptr<GlobTreeNode> _child[2]{};

		constexpr GlobTreeNode(Key const& key)
				: _key(key)
		{
		}

		constexpr GlobTreeNode(Key const& key, Glob glob)
				: _key(key)
				, _glob(glob)
		{
		}

		constexpr GlobTreeNode(std::unique_ptr<GlobTreeNode> a, std::unique_ptr<GlobTreeNode> b)
				: _key(a->_key ^ b->_key)
				, _child{std::move(a), std::move(b)}
		{
			_canonicalize();
			_validate();
		}
		
		constexpr auto find(Key const& key, Glob const* best = nullptr) const -> bool
		{
			assert(key.size() == 128_u128);
			assert(_key <= key);

			if (_glob.has_value()) {
				best = &_glob.value();
			}
			
			if (_child[1] and _child[1]->_key <= key) {
				return _child[1]->find(key, best);
			}
				
			if (_child[0] and _child[0]->_key <= key) {
				return _child[0]->find(key, best);
			}

			return best ? best->contains(key.data()) : false; 
		}

		constexpr auto insert(Key const& key, GlobTreeNode* best = nullptr) -> bool
		{
			assert(key.size() == 128_u128);
			assert(_key <= key);

			if (_glob.has_value()) {
				best = this;
			}

			if (_child[1] and _child[1]->_key <= key) {
				return _child[1]->insert(key, best);
			}
				
			if (_child[0] and _child[0]->_key <= key) {
				return _child[0]->insert(key, best);
			}

			if (best) {
				return best->_insert(key);
			}

			// failure path means there were no concrete nodes on the way here,
			// so we'll upgrade to a concrete node and then insert the key here
			return _glob.emplace().insert(key);
		}

	  private:
		constexpr auto _validate() const -> void
		{
			if (_child[1]) {
				assert(_child[0]);
				assert(_key < _child[1]->_key);
			}

			if (_child[0]) {
				assert(_key < _child[0]->_key);
			}

			if (_child[0] and _child[1]) {
				assert(_child[0]->_key <=> _child[1]->_key == std::partial_ordering::unordered);
				assert(less(_child[0]->_key, _child[1]->_key));
			}
		}
		
		constexpr auto _canonicalize() -> void
		{
			if (not _child[0]) {
				_child[0] = std::move(_child[1]);
				return;
			}

			if (not _child[1]) {
				return;
			}
			
			if (not less(_child[0]->_key, _child[1]->_key)) {
				using std::swap;
				swap(_child[0], _child[1]);
			}
		}

		constexpr auto _insert(Key const& key) -> bool
		{
			assert(key.size() == 128_u128);
			assert(_glob.has_value());
			
			if (_glob->insert(key)) {
				return true;
			}

			// For testing
			assert(not options::factor);
			assert(not options::bubble);

			{
				auto const& [range, fit] = _glob->split_point(options::local_fit, _key);
				assert(range.size() < _glob->size() && "failed to split glob");
				_insert(fit, _glob->extract(range));
			}

			// Restart the recursive insert.
			return insert(key);
		}

		constexpr auto _insert(Key const& key, Glob glob) -> void
		{
			assert(_key <= key);
			assert(glob.size() != 0);

			// case 0: key matches this and we're synthetic, just upgrade
			if (_key == key) {
				assert(not _glob.has_value());
				_glob = std::move(glob);
				return;
			}

			// case 1: key matches right child, recurse
			if (_child[1] and _child[1]->_key <= key) {
				_child[1]->_insert(key, std::move(glob));
				return;
			}
			
			// case 2: key matches left child, recurse			
			if (_child[0] and _child[0]->_key <= key) {
				_child[0]->_insert(key, std::move(glob));
				return;
			}

			auto const _ = exit_scope([&] {
				_canonicalize();
				_validate();
				if (_child[0]) {
					_child[0]->_validate();
				}
				if (_child[1]) {
					_child[1]->_validate();
				}
			});
			
			auto node = std::make_unique<GlobTreeNode>(key, std::move(glob));

			// if the node dominates either or both children, then move it
			{
				if (_child[0] and key < _child[0]->_key) {
					node->_child[0] = std::move(_child[0]);
				}

				if (_child[1] and key < _child[1]->_key) {
					node->_child[1] = std::move(_child[1]);
				}

				node->_canonicalize();
				node->_validate();
			}
			
			// case 3: _child[0] is now null
			if (not _child[0]) {
				_child[0] = std::move(node);
				return;
			}

			// case 4: _child[1] is now null
			if (not _child[1]) {
				_child[1] = std::move(node);
				return;
			}
			
			// case 5: I have two children
			auto const a = (_child[0]->_key ^ _child[1]->_key).size();
			auto const b = (_child[0]->_key ^ key).size();
			auto const c = (key ^ _child[1]->_key).size();
				
			// case 5a:
			if (a < b and b < c) {
				_child[1] = std::make_unique<GlobTreeNode>(std::move(node), std::move(_child[1]));
				return;
			}

			// case 5b:
			if (a < b) {
				_child[0] = std::make_unique<GlobTreeNode>(std::move(node), std::move(_child[0]));
				return;
			}

			// case 5c:
			if (a < c) {
				_child[1] = std::make_unique<GlobTreeNode>(std::move(node), std::move(_child[1]));
				return;
			}

			// case 5d:
			_child[0] = std::make_unique<GlobTreeNode>(std::move(_child[0]), std::move(_child[1]));
			_child[1] = std::move(node);
		}
	};
}

#ifdef TREE_TESTING

#include "tree/testing.hpp"

namespace tree::testing
{
	inline const auto test_glob_tree_upgrade = test<[]
	{
		auto const factor = std::exchange(tree::options::factor, 0);
		auto const bubble = std::exchange(tree::options::bubble, 0);
		auto const capacity = std::exchange(tree::options::default_glob_capacity, 2);

		auto node = GlobTreeNode("0/0");
		assert(node.insert("0/128"));
		assert(node.insert("1/128"));
		assert(node.insert("2/128"));
		assert(node.insert("3/128"));
		assert(node.insert("4/128"));

		tree::options::factor = factor;
		tree::options::bubble = bubble;
		tree::options::default_glob_capacity = capacity;
		
		return true;
	}>{};
}

#endif
