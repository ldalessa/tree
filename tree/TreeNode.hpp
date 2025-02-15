#pragma once

#include "tree/exit_scope.hpp"
#include "tree/options.hpp"
#include "tree/types.hpp"
#include "tree/Key.hpp"

#include <compare>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <utility>

namespace tree
{
	template <class>
	struct tree_value_type;
	
	template <class ValueNode>
	struct TreeNode
	{
		using Value = typename tree_value_type<ValueNode>::type;
		
		Key const _key;
		TreeNode* _children[2]{};
		std::shared_mutex mutable _lock{};
		
		constexpr virtual ~TreeNode() noexcept {
			delete _children[0];
			delete _children[1];
		}
		
		constexpr TreeNode(Key key)
				: _key(key)
		{
		}

		constexpr TreeNode(Key key, TreeNode* a, TreeNode* b)
				: _key(key)
				, _children{ a, b }
		{
			_canonicalize();
			_validate();
		}

		/// Create a dominator for nodes a and b.
		[[gnu::nonnull]]
		constexpr TreeNode(TreeNode* a, TreeNode* b)
				: TreeNode(a->_key ^ b->_key, a, b)
		{
		}

		constexpr auto find_best(Key key) const -> TreeNode const*
		{
			auto const _ = std::shared_lock(_lock);

			if (_children[0] and _children[0]->_key <= key) {
				return _children[0]->find(key);
			}

			if (_children[1] and _children[1]->_key <= key) {
				return _children[1]->find(key);
			}

			if (_key <= key) {
				return this;
			}
			
			return nullptr;
		}

		constexpr auto find(Key key) const -> ValueNode const* {
			auto const _ = std::shared_lock(_lock);

			if (_children[0] and _children[0]->_key <= key) {
				return _children[0]->find(key) ?: _as_value_node();
			}

			if (_children[1] and _children[1]->_key <= key) {
				return _children[1]->find(key) ?: _as_value_node();
			}

			if (_key <= key) {
				return _as_value_node();
			}
			
			return nullptr;
		}

		constexpr auto insert(Key key, Value value) -> ValueNode*
		{
			auto const _ = exit_scope([&] {
				_validate();
			});
			
		  retry:
			{
				auto const _ = std::shared_lock(_lock);
			
				assert(_key <= key);

				if (key == _key) {
					goto upgrade;
				}
			
				if (_children[0] and _children[0]->_key <= key) {
					return _children[0]->insert(key, std::move(value));
				}

				if (_children[1] and _children[1]->_key <= key) {
					return _children[1]->insert(key, std::move(value));
				}
			}
			
			{
				auto const _ = std::unique_lock(_lock);
				
				if (_children[0] and _children[0]->_key <= key) {
					goto retry;
				}

				if (_children[1] and _children[1]->_key <= key) {
					goto retry;
				}
				
				return _insert(key, std::move(value));
			}

		  upgrade:
			{
				auto const _ = std::unique_lock(_lock);

				if (auto found = get_value()) {
					assert(*found == value);
					return _as_value_node();
				}
				else {
					assert(false);
				}
			}
		}

		constexpr virtual auto get_value() const -> std::optional<Value> {
			return std::nullopt;
		}

	  protected:
		constexpr virtual auto _as_value_node() const -> ValueNode const* {
			return nullptr;
		}
		
		constexpr virtual auto _as_value_node() -> ValueNode* {
			return nullptr;
		}
		
	  private:
		constexpr auto _validate() const -> void
		{
			if (_children[1]) {
				assert(_children[0]);
				assert(_key < _children[1]->_key);
			}

			if (_children[0]) {
				assert(_key < _children[0]->_key);
			}

			if (_children[0] and _children[1]) {
				assert(_children[0]->_key <=> _children[1]->_key == std::partial_ordering::unordered);
				assert(less(_children[0]->_key, _children[1]->_key));
			}
		}
		
		constexpr auto _canonicalize() -> void {
			if (not _children[1]) {
				return;
			}
			assert(_children[0]);
			if (not less(_children[0]->_key, _children[1]->_key)) {
				std::swap(_children[0], _children[1]);
			}
		}
		
		constexpr auto _insert(Key key, Value value) -> ValueNode*
		{
			assert(not _children[1] or _children[0] && "Tree is not canonical");

			auto const _ = exit_scope([&] {
				_canonicalize();
				_validate();
			});
			
			auto node = new ValueNode(key, std::move(value));

			// neither child is null
			if (_children[0] and _children[1]) {
				auto const a = _children[0]->_key ^ _children[1]->_key;
				auto const b = _children[0]->_key ^ node->_key;
				auto const c = node->_key ^ _children[1]->_key;
				
				if (a < b) {
					if (b < c) {
						_children[1] = new TreeNode(node, _children[1]);
					}
					else {
						_children[0] = new TreeNode(node, _children[0]);
					}
					return node;
				}

				if (a < c) {
					_children[1] = new TreeNode(node, _children[1]);
					return node;
				}

				_children[0] = new TreeNode(_children[0], _children[1]);
				_children[1] = node;
				return node;
			}

			// _children[1] is null and the node dominates _children[0]
			if (_children[0] and key < _children[0]->_key) {
				node->_children[0] = _children[0];
				_children[0] = node;
				return node;
			}

			// _children[1] is null and node does not dominate _children[0]
			if (_children[0]) {
				_children[1] = node;
				return node;
			}

			// _children[0] is null
			_children[0] = node;
			return node;
		}
	};

	template <class Value>
	struct ValueNode;

	template <class Value>
	struct tree_value_type<ValueNode<Value>> {
		using type = Value;
	};
	
	template <class Value>
	struct ValueNode : TreeNode<ValueNode<Value>>
	{
		using Base = TreeNode<ValueNode<Value>>;
		
		Value _value;

		constexpr ValueNode(Base* next, Value value)
				: Base(next->key, next->_children[0], next->_children[1])
				, _value(std::move(value))
		{
		}
		
		constexpr ValueNode(Key key, Value value)
				: Base(key)
				, _value(std::move(value))
		{
		}

		constexpr auto get_value() const -> std::optional<Value> override {
			return _value;
		}
		
	  private:
		constexpr virtual auto _as_value_node() const -> ValueNode const* override final {
			return this;
		}
		
		constexpr virtual auto _as_value_node() -> ValueNode* override final {
			return this;
		}
	};
}

#ifdef TREE_TESTING

#include "tree/testing.hpp"

namespace tree::testing
{
	inline const auto test_insert = test<[] {
		TreeNode<ValueNode<int>> root("0/0");
		assert(root._children[0] == nullptr);
		assert(root._children[1] == nullptr);
		
		auto a = root.insert("1/128", 0);
		assert(root._children[0] == a);
		assert(root._children[0]->_children[0] == nullptr);
		assert(root._children[0]->_children[1] == nullptr);
		assert(root._children[1] == nullptr);

		auto const n0 = root.find_best("1/128");
		assert(n0 == a);
	
		auto const n1 = root.find("1/128");
		assert(n1 == a);

		auto const n2 = root.find_best("0/128");
		assert(n2 == &root);

		auto const n3 = root.find("0/128");
		assert(n3 == nullptr);
	
		auto b = root.insert("0/128", 1);
		assert(root._children[0] == b);
		assert(root._children[0]->_children[0] == nullptr);
		assert(root._children[0]->_children[1] == nullptr);
		assert(root._children[1] == a);
		assert(root._children[1]->_children[0] == nullptr);
		assert(root._children[1]->_children[1] == nullptr);
		
		return true;
	}>{};
}

#endif
