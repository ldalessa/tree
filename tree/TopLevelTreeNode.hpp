#pragma once

#include "tree/exit_scope.hpp"
#include "tree/options.hpp"
#include "tree/types.hpp"
#include "tree/Key.hpp"

#include <cassert>
#include <compare>
#include <concepts>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace tree
{
	struct TopLevelTreeNode
	{
		using Value = u32;

		Key _key;
		std::optional<Value> _value{};
		std::unique_ptr<TopLevelTreeNode> _child[2]{};

		// Construct a node with no value.
		constexpr TopLevelTreeNode(Key key)
				: _key(key)
		{
		}

		// Construct a node with a value.
		template <class... Ts>
		requires (sizeof...(Ts) != 0) and std::constructible_from<Value, Ts...>
		constexpr TopLevelTreeNode(Key key, Ts&&... ts)
			: _key(key)
											 , _value(std::forward<Ts>(ts)...)
		{
		}

		// Construct a dominator node (with no value)
		constexpr TopLevelTreeNode(std::unique_ptr<TopLevelTreeNode> a, std::unique_ptr<TopLevelTreeNode> b)
				: _key(a->_key ^ b->_key)
				, _child{std::move(a), std::move(b)}
		{
			_canonicalize();
			_validate();
		}	

		constexpr auto key() const -> Key {
			return _key;
		}

		constexpr auto has_value() const -> bool {
			return _value.has_value();
		}
		
		constexpr auto value() const -> Value const& {
			return _value.value();
		}
		
		constexpr auto name() const -> std::string {
			return std::format("{}", _key);
		}

		// Return the node with the value that best matches the key.
		constexpr auto find(Key key, TopLevelTreeNode const* best = nullptr) const
			-> TopLevelTreeNode const*
		{
			assert(_key <= key);

			if (has_value()) {
				best = this;
			}

			if (_child[1] and _child[1]->_key <= key) {
				return _child[1]->find(key, best);
			}

			if (_child[0] and _child[0]->_key <= key) {
				return _child[0]->find(key, best);
			}

			return best;
		}

		// Insert a key value pair into the tree. If the key already exists it
		// will overwrite the value. Returns the value.
		template <class... Ts>
		requires std::constructible_from<Value, Ts...>
		constexpr auto insert_or_update(Key key, Ts&&... ts) -> Value const&
		{
			assert(_key <= key);

			// case 1: update
			if (_key == key) {
				return _value.emplace(std::forward<Ts>(ts)...);
			}

			// case 2: this key belongs below my right child
			if (_child[0] and _child[0]->_key <= key) {
				return _child[0]->insert_or_update(key, std::forward<Ts>(ts)...);
			}

			// case 3: this key belongs below my right child
			if (_child[1] and _child[1]->_key <= key) {
				return _child[1]->insert_or_update(key, std::forward<Ts>(ts)...);
			}

			// case 4: this key needs to be inserted here
			return _insert(key, std::forward<Ts>(ts)...);
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
		template <class... Ts>
		constexpr auto _insert(Key key, Ts&&... ts) -> Value const&
		{
			auto const _ = exit_scope([&] {
				_canonicalize();
				_validate();
			});
			
			auto node = std::make_unique<TopLevelTreeNode>(key, std::forward<Ts>(ts)...);
			auto const& value = node->value();
			
			// if the node dominates either or both children, reorder them
			if (_child[0] and key < _child[0]->_key) {
				node->_child[0] = std::move(_child[0]);
			}

			if (_child[1] and key < _child[1]->_key) {
				node->_child[1] = std::move(_child[1]);
			}

			node->_canonicalize();
			node->_validate();

			// case 1: _child[0] is null
			if (not _child[0]) {
				_child[0] = std::move(node);
				return value;
			}

			// case 2: _child[1] is null
			if (not _child[1]) {
				_child[1] = std::move(node);
				return value;
			}

			// case 3: I have two children
			auto const a = (_child[0]->_key ^ _child[1]->_key).size();
			auto const b = (_child[0]->_key ^ key).size();
			auto const c = (key ^ _child[1]->_key).size();
				
			// case 3a:
			if (a < b and b < c) {
				_child[1] = std::make_unique<TopLevelTreeNode>(std::move(node), std::move(_child[1]));
				return value;
			}

			// case 3b:
			if (a < b) {
				_child[0] = std::make_unique<TopLevelTreeNode>(std::move(node), std::move(_child[0]));
				return value;
			}

			// case 3c:
			if (a < c) {
				_child[1] = std::make_unique<TopLevelTreeNode>(std::move(node), std::move(_child[1]));
				return value;
			}

			// case 3d:
			_child[0] = std::make_unique<TopLevelTreeNode>(std::move(_child[0]), std::move(_child[1]));
			_child[1] = std::move(node);

			return value;
		}
	};
}

#ifdef TREE_TESTING

#include "tree/testing.hpp"

namespace tree::testing
{
	inline auto const test_top_level_tree_node_insert = test<[] {
		TopLevelTreeNode root("0/0");
		assert(root._child[0] == nullptr);
		assert(root._child[1] == nullptr);
		
		{
			auto const& a = root.insert_or_update("1/128", 1);
			auto const* p = root.find("1/128", &root);
			assert(p == root._child[0].get());
			assert(root._child[0]->_child[0] == nullptr);
			assert(root._child[0]->_child[1] == nullptr);
			assert(root._child[1] == nullptr);

			assert(a == 1);
			assert(a == p->value());
			assert(&a == &p->value());
		}

		{
			auto const& a = root.insert_or_update("0/128", 42);
			auto const* p = root.find("0/128", &root);
			auto const* q = root.find("1/128", &root);
			assert(p == root._child[0].get());
			assert(q == root._child[1].get());
			
			assert(a == 42);
			assert(a == p->value());
			assert(&a == &p->value());
			assert(q->value() == 1);
		}
		
		return true;
	}>{};
}

#endif
