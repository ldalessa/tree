module;

#include <cassert>

export module tree:TreeNode;

import :exit_scope;
import :options;
import :testing;
import :types;
import :Key;
import std;

export
namespace tree
{
	template <class>
	struct tree_value_type;
	
	template <class ValueNode>
	struct TreeNode
	{
		using Value = typename tree_value_type<ValueNode>::type;
		
		Key _key;
		TreeNode* _parent{};
		std::unique_ptr<TreeNode> _children[2]{};

		constexpr virtual ~TreeNode() = default;
		
		constexpr TreeNode(Key key)
			: _key(key)
		{
		}

		constexpr TreeNode(std::unique_ptr<TreeNode> a, std::unique_ptr<TreeNode> b)
			: _key(a->_key ^ b->_key)
			, _children{std::move(a), std::move(b)}
		{
			assert(_children[0] and _children[1]);
			_children[0]->_parent = this;
			_children[1]->_parent = this;
			_canonicalize();
			_validate();
		}	
		
		constexpr auto key() const -> Key {
			return _key;
		}
		
		constexpr auto name() const -> std::string {
			return std::format("{}", _key);
		}

		struct find_result {
			TreeNode* node;
			TreeNode* best;
		};

		constexpr auto find(Key key, TreeNode* best) -> find_result
		{
			assert(_key <= key or best != nullptr);

			if (auto p = this->_as_value_node()) {
				best = this;
			}

			if (_children[1] <= key) {
				assert(_children[0] and key < _children[0]);
				return _children[1]->find(key, best);
			}

			if (_children[0] <= key) {
				return _children[0]->find(key, best);
			}
            
			return find_result {
				.node = best,
				.best = this
			};
		}

		constexpr auto insert(Key key, Value value) -> ValueNode&
		{
			assert(_key <= key);
			
			if (_key == key) {
				return set_value(std::move(value));
			}

			if (_children[0] <= key) {
				assert(not (_children[1] <= key));
				return _children[0]->insert(key, std::move(value));
			}

			if (_children[1] <= key) {
				return _children[1]->insert(key, std::move(value));
			}

			auto const _ = exit_scope([&] {
				_canonicalize();
				_validate();
			});
			
			auto node = std::make_unique<ValueNode>(key, std::move(value));
			auto out = node.get();
			if (key < _children[0]) {
				node->_set_child(0, _take_child(0));
			}

			if (key < _children[1]) {
				node->_set_child(1, _take_child(1));
			}

			if (_children[0] == nullptr) {
				_set_child(0, std::move(node));
				return *out;
			}

			if (_children[1] == nullptr) {
				_set_child(1, std::move(node));
				return *out;
			}

			_set_child(0, std::make_unique<TreeNode>(_take_child(0), _take_child(1)));
			_set_child(1, std::move(node));
			return *out;
		}

		constexpr virtual auto set_value(Value value) -> ValueNode&
		{
			assert(_parent);
			
			auto vnode = std::make_unique<ValueNode>(_key, std::move(value));
			vnode->_set_child(0, _take_child(0));
			vnode->_set_child(1, _take_child(1));
			auto out = vnode.get();
			
			if (_parent->_children[0].get() == this) {
				_parent->_set_child(0, std::move(vnode));
			}
			else {
				assert(_parent->_children[1].get() == this);
				_parent->_set_child(0, std::move(vnode));
			}

			return *out;
		}

	protected:
		constexpr virtual auto _as_value_node() -> ValueNode* {
			return nullptr;
		}
		
	private:
		constexpr auto _validate() const -> void
		{
			if (_parent) {
				assert(_parent->_key < _key);
				assert(_parent->_children[0].get() == this or _parent->_children[1].get() == this);
			}

			if (_children[1]) {
				assert(_children[0]);
				assert(_key < _children[1]->_key);
				assert(_children[1]->_parent == this);
			}

			if (_children[0]) {
				assert(_key < _children[0]->_key);
				assert(_children[0]->_parent == this);
			}

			if (_children[0] and _children[1]) {
				assert(_children[0]->_key <=> _children[1]->_key == std::partial_ordering::unordered);
			}
		}
		
		constexpr auto _canonicalize() -> void
		{
			if (_children[1] == nullptr) {
				return;
			}

			if (_children[0] == nullptr) {
				_children[0] = std::move(_children[1]);
			}

			auto const c = _children[0]->_key ^ _children[1]->_key;
			auto const b = c.size();
			assert(b < 128u);
			
			if (_children[0]->_key[b] == (u64)0) {
				assert(_children[1]->_key[b] == (u64)1);
				return;
			}

			std::swap(_children[0], _children[1]);
		}
		
		constexpr auto _set_child(int n, std::unique_ptr<TreeNode> c) -> TreeNode*
		{
			assert(0 <= n and n < 2);
			if (std::is_constant_evaluated() and _children[n]) {
				_children[n]->_parent = nullptr;
			}

			if (not std::is_constant_evaluated() and options::debug and _children[n]) {
				_children[n]->_parent = nullptr;
			}

			if ((_children[n] = std::move(c))) {
				_children[n]->_parent = this;
			}
			
			return _children[n].get();
		}
		
		constexpr auto _take_child(int n) -> std::unique_ptr<TreeNode>
		{
			assert(0 <= n and n < 2);
			if (std::is_constant_evaluated() and _children[n]) {
				_children[n]->_parent = nullptr;
			}

			if (not std::is_constant_evaluated() and options::debug and _children[n]) {
				_children[n]->_parent = nullptr;
			}

			return std::move(_children[n]);
		}

		constexpr friend auto operator<(Key a, std::unique_ptr<TreeNode> const& b) -> bool {
			return not b or a < b->_key;
		}

		constexpr friend auto operator<(std::unique_ptr<TreeNode> const& a, Key b) -> bool {
			return a and a->_key < b;
		}

		constexpr friend auto operator<=(Key a, std::unique_ptr<TreeNode> const& b) -> bool {
			return not b or a <= b->_key;
		}

		constexpr friend auto operator<=(std::unique_ptr<TreeNode> const& a, Key b) -> bool {
			return a and a->_key <= b;
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
		Value _value;

		constexpr ValueNode(Key key, Value value)
			: ValueNode::TreeNode::TreeNode(key)
			, _value(std::move(value))
		{
		}

		constexpr auto set_value(Value value) -> ValueNode& override {
			_value = std::move(value);
			return *this;
		}

	private:
		constexpr virtual auto _as_value_node() -> ValueNode* override final {
			return this;
		}
	};
}

#undef DNDEBUG

static testing::test<[] {
	TreeNode<ValueNode<int>> root("0/0");
	assert(root._parent == nullptr);
	assert(root._children[0] == nullptr);
	assert(root._children[1] == nullptr);
		
	auto& a = root.insert("1/128", 0);
	assert(root._children[0].get() == &a);
	assert(root._children[0]->_children[0] == nullptr);
	assert(root._children[0]->_children[1] == nullptr);
	assert(root._children[1] == nullptr);
	{
		auto const [s, t] = root.find("1/128", nullptr);
		assert(s == &a);
		assert(t == &a);
	}
		
	auto& b = root.insert("0/128", 1);
	assert(root._children[0].get() == &b);
	assert(root._children[0]->_children[0] == nullptr);
	assert(root._children[0]->_children[1] == nullptr);
	assert(root._children[1].get() == &a);
	assert(root._children[1]->_children[0] == nullptr);
	assert(root._children[1]->_children[1] == nullptr);
		
	return true;
 }> test_insert;
