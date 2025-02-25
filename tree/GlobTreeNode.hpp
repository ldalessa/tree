#pragma once

#include "tree/exit_scope.hpp"
#include "tree/types.hpp"
#include "tree/Glob.hpp"
#include "tree/Key.hpp"

#include <cassert>
#include <expected>
#include <memory>
#include <optional>

namespace tree
{	
	struct GlobTreeNode
	{
		Key const _key{};
		std::optional<Glob> _glob{};
		std::unique_ptr<GlobTreeNode> _child[2]{};

		constexpr GlobTreeNode() = default;
		
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

		constexpr auto key() const -> Key const& {
			return _key;
		}

		constexpr auto has_value() const -> bool {
			return _glob.has_value();
		}
		
		constexpr auto value() const -> Glob const& {
			assert(_glob.has_value());
			return _glob.value();
		}

		constexpr auto take_value() -> Glob&& {
			assert(_glob.has_value());
			return std::move(_glob.value());
		}
		
		constexpr auto find(u128 key, Glob const* best = nullptr) const -> bool
		{
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

			return best ? best->contains(key) : false; 
		}

		constexpr auto insert(u128 key, GlobTreeNode* best = nullptr) -> std::expected<bool, GlobTreeNode>
		{
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

		// Reinsert a glob that was ejected during bubbling. 
		constexpr auto reinsert(GlobTreeNode&& node) -> void {
			_insert(node.key(), node.take_value());
		}

		constexpr auto for_each_node(auto&& f) const -> void {
			f(*this);
			if (_child[0]) _child[0]->for_each_node(f);
			if (_child[1]) _child[1]->for_each_node(f);
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

		constexpr auto _insert(u128 key) -> std::expected<bool, GlobTreeNode>
		{
			assert(_glob.has_value());
			
			if (_glob->insert(key)) {
				return true;
			}
			
			if (options::bubble <= _key.size()) {
				auto [range, bkey] = _glob->split_point(options::global_fit, _key);
				auto glob = _glob->extract(range);
				if (bkey <= key) {
					if (not glob.insert(key)) {
						throw error("Bubbled glob does not have space for key {}", key);
					}
				}
				else {
					if (not _glob->insert(key)) {
						throw error("Bubbled glob did not make space for key {}", key);
					}
				}
				return std::unexpected<GlobTreeNode>(std::in_place, bkey, std::move(glob));
			}

			auto [range, fit] = _glob->split_point(options::local_fit, _key);
			assert(range.size() < _glob->size() && "failed to split glob");

			// should we try to factor this glob
			if (_key.size() < options::factor and options::factor <= fit.size()) {
				range = _glob->factor(fit);
				if (options::debug) {
					assert(range.size() != _glob->size());
				}
			}

			_insert(fit, _glob->extract(range));

			// Restart the recursive insert.
			return insert(key);
		}

		constexpr auto _insert(Key key, Glob glob) -> void
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
		auto const bubble = std::exchange(tree::options::bubble, -1_u32);
		auto const capacity = std::exchange(tree::options::default_glob_capacity, 2);

		auto node = GlobTreeNode("0/0");
		assert(node.insert(0));
		assert(node.insert(1));
		assert(node.insert(2));
		assert(node.insert(3));
		assert(node.insert(4));

		tree::options::factor = factor;
		tree::options::bubble = bubble;
		tree::options::default_glob_capacity = capacity;
		
		return true;
	}>{};
}

#endif
