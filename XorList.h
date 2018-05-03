#pragma once
#include <utility>
#include <memory>
#include <cassert>
template < typename T >
class Node {
private:
	T data_;
	uintptr_t XOR_;
	Node *get_neighbor(Node *other_neighbor) {
		uintptr_t value = (reinterpret_cast <uintptr_t> (other_neighbor) ^ XOR_);
		if (value == 0)
			return nullptr;
		return reinterpret_cast <Node*>(value);
	}
public:
	template < typename U >
	explicit Node(U&& data, Node* left = nullptr, Node* right = nullptr) :
		XOR_(reinterpret_cast <uintptr_t> (left) ^ reinterpret_cast <uintptr_t> (right)),
		data_(std::forward < U >(data))
	{}
	explicit Node(const Node& v) :
		data_(v.data_),
		XOR_(v.XOR_)
	{}

	void upd_XOR(Node* prev, Node* now) {
		XOR_ ^= reinterpret_cast <uintptr_t> (prev) ^ reinterpret_cast <uintptr_t> (now);
	}

	uintptr_t get_XOR() {
		return XOR_;
	}

	T& data() {
		return data_;
	}

	Node* get_right(Node *left) {
		return get_neighbor(left);
	}
	Node* get_left(Node *right) {
		return get_neighbor(right);
	}

	~Node() {}
};

template < typename T, class Allocator = std::allocator < T > >
class XorList {
private:
	Node < T > *first_;
	Node < T > *last_;
	using node_alloc_t = typename std::allocator_traits < Allocator > :: template rebind_alloc < Node < T > >;
	node_alloc_t node_alloc_;
	size_t cnt_;

	void construct_from_lvalue(const XorList& other) {
		Node < T > *now = other.first_, *left = nullptr;
		while (now != nullptr) {
			push_back(now->data());
			Node < T > *tmp = now;
			now = now->get_right(left);
			left = tmp;
		}
	}

	void constructor_from_rvalue(XorList&& other) {
		std::swap(first_, other.first_);
		std::swap(last_, other.last_);
		std::swap(cnt_, other.cnt_);
	}

	void defaults() {
		first_ = nullptr;
		last_ = nullptr;
		cnt_ = 0;
	}

	template < typename U >
	void push(U&& value, bool is_back) {
		if (cnt_ == 0) {
			first_ = last_ = std::allocator_traits < node_alloc_t >::allocate(node_alloc_, 1);
			std::allocator_traits < node_alloc_t >::construct(node_alloc_, first_, std::forward < U >(value));
		}
		else {
			Node < T > *v = std::allocator_traits < node_alloc_t >::allocate(node_alloc_, 1);
			Node < T > **ref_to_pointer;
			if (is_back) {
				std::allocator_traits <node_alloc_t>::construct(node_alloc_, v, std::forward < U >(value), last_);
				ref_to_pointer = &last_;
			}
			else {
				std::allocator_traits <node_alloc_t>::construct(node_alloc_, v, std::forward < U >(value), nullptr, first_);
				ref_to_pointer = &first_;
			}
			(*ref_to_pointer)->upd_XOR(nullptr, v);
			(*ref_to_pointer) = v;
		}
		++cnt_;
	}

	void pop(bool is_back) {
		assert(last_ != nullptr);
		Node < T > *& tmp = (is_back ? last_ : first_);
		if (cnt_ == 1) {
			std::allocator_traits <node_alloc_t>::deallocate(node_alloc_, tmp, 1);
			first_ = last_ = nullptr;
		}
		else {
			Node < T > *new_tmp = reinterpret_cast <Node < T >*> (tmp->get_XOR());
			new_tmp->upd_XOR(tmp, nullptr);
			std::allocator_traits <node_alloc_t>::deallocate(node_alloc_, tmp, 1);
			tmp = new_tmp;
		}
		--cnt_;
	}
public:
	class iterator : public std::iterator <std::bidirectional_iterator_tag, T> {
	protected:
		Node < T > *left_ = nullptr;
		Node < T > *now_ = nullptr;
	public:
		explicit iterator(Node < T > *v, Node < T > *left = nullptr) :
			left_(left),
			now_(v)
		{}
		explicit iterator() :
			left_(nullptr),
			now_(nullptr)
		{}
		iterator(const iterator &it) :
			iterator(it.now_, it.left_)
		{}

		iterator& operator = (const iterator& it) {
			now_ = it.now_;
			left_ = it.left_;
			return (*this);
		}

		bool operator == (const iterator& it) const {
			return now_ == it.now_;
		}
		bool operator != (const iterator& it) const {
			return !((*this) == it);
		}

		iterator& operator ++ () {
			Node < T > *prev_now = now_;
			now_ = now_->get_right(left_);
			left_ = prev_now;
			return *this;
		}
		const iterator operator ++(int) {
			iterator ret = (*this);
			++(*this);
			return ret;
		}
		iterator& operator -- () {
			Node < T > *prev_left = left_;
			if (left_)
				left_ = left_->get_left(now_);
			now_ = prev_left;
			return *this;
		}
		const iterator operator --(int) {
			iterator ret;
			ret.left_ = left_;
			ret.now_ = now_;
			--(*this);
			return ret;
		}

		T& operator * () {
			return now_->data();
		}

		Node < T > *get_Node() {
			return now_;
		}
		Node < T > *get_left_Node() {
			return left_;
		}

		explicit operator bool() const {
			return (now_ ? true : false);
		}

		~iterator() {}
	};

	class reverse_iterator : public iterator {
	public:
		explicit reverse_iterator(Node < T > *v, Node < T > *left = nullptr) :
			iterator(v, left)
		{}
		explicit reverse_iterator() : iterator()
		{}
		reverse_iterator(const reverse_iterator &it) :
			reverse_iterator(it.now_, it.left_)
		{}

		reverse_iterator& operator -- () {
			iterator::operator++();
			return (*this);
		}
		reverse_iterator operator -- (int) {
			reverse_iterator ret = (*this);
			--(*this);
			return ret;
		}
		reverse_iterator& operator ++ () {
			iterator::operator--();
			return (*this);
		}
		reverse_iterator operator ++ (int) {
			reverse_iterator ret = (*this);
			++(*this);
			return ret;
		}
	};

	typedef T value_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef Allocator allocator_type;
	typedef std::size_t size_type;
	typedef const iterator const_iterator;
	typedef const reverse_iterator const_reverse_iterator;

	explicit XorList(const Allocator& alloc = Allocator()) :
		first_(nullptr),
		last_(nullptr),
		cnt_(0),
		node_alloc_(alloc)
	{}

	template < typename U >
	void push_back(U&& value) {
		push(std::forward < U > (value), true);
	}
	template < typename U >
	void push_front(U&& value) {
		push(std::forward < U > (value), false);
	}

	T& front() {
		return first_->data();
	}
	const T& front() const {
		return first_->data();
	}
	T& back() {
		return last_->data();
	}
	const T& back() const {
		return const last_->data();
	}

	void pop_back() {
		pop(true);
	}
	void pop_front() {
		pop(false);
	}

	size_t size() const {
		return cnt_;
	}

	void clear() {
		while (cnt_ > 0)
			pop_back();
	}

	bool empty() const {
		return cnt_ == 0;
	}

	XorList(std::size_t count, const T& value = T(), const Allocator& alloc = Allocator()) : XorList(alloc)
	{
		while (count--) {
			push_back(value);
		}
	}
	XorList(const XorList& other) : XorList(other.node_alloc_)
	{
		construct_from_lvalue(other);
	}
	XorList(XorList&& other) : XorList(other.node_alloc_)
	{
		constructor_from_rvalue(std::move(other));
	}
	
	~XorList() {
		while (cnt_ > 0)
			pop_back();
	}

	XorList &operator= (const XorList& other) {
		clear();
		node_alloc_ = other.node_alloc_;
		construct_from_lvalue(other);
		return *this;
	}
	XorList &operator= (XorList&& other) {
		clear();
		node_alloc_ = other.node_alloc_;
		constructor_from_rvalue(std::move(other));
		return *this;
	}

	iterator begin() {
		return iterator(first_, nullptr);
	}
	iterator end() {
		return iterator(nullptr, last_);
	}

	reverse_iterator rend() {
		return reverse_iterator(nullptr, nullptr);
	}
	reverse_iterator rbegin() {
		return reverse_iterator(last_, (last_ == nullptr ? nullptr : last_->get_left(nullptr)));
	}

	template < typename U >
	iterator insert_before(iterator it, U&& value) {
		if (it == begin()) {
			push_front(std::forward < U >(value));
			return begin();
		}
		if (it == end()) {
			push_back(std::forward < U >(value));
			return rbegin();
		}
		Node < T > *v = std::allocator_traits <node_alloc_t>::allocate(node_alloc_, 1);
		std::allocator_traits <node_alloc_t>::construct(node_alloc_, v, std::forward < U >(value));
		Node < T > *now = it.get_Node(), *left = it.get_left_Node();
		left->upd_XOR(now, v);
		now->upd_XOR(left, v);
		v->upd_XOR(nullptr, now);
		v->upd_XOR(nullptr, left);
		++cnt_;
		return iterator(v, left);
	}

	template < typename U >
	iterator insert_after(iterator it, U&& value) {
		assert(it != this->end());
		++it;
		return insert_before(it, std::forward < U >(value));
	}

	void erase(iterator it) {
		assert(it);
		if (it == begin()) {
			pop_front();
			return;
		}
		iterator right = it, left = it;
		--left;
		++right;
		if (!right) {
			pop_back();
			return;
		}
		cnt_--;
		left.get_Node()->upd_XOR(it.get_Node(), right.get_Node());
		right.get_Node()->upd_XOR(it.get_Node(), left.get_Node());
		std::allocator_traits <node_alloc_t>::deallocate(node_alloc_, it.get_Node(), 1);
	}

};
//