#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<
	class Key,
	class T,
	class Compare = std::less<Key>
> class map {
public:
    /**
	 * the internal type of data.
	 * it should have a default constructor, a copy constructor.
	 * You can use sjtu::map as value_type by typedef.
	 */
	typedef pair<const Key, T> value_type;
private:
	Compare cmp;
	size_t Size = 0;

	struct Node {
		value_type v;
		int h = 0;
		Node* l = nullptr;
		Node* r = nullptr;
		Node* p = nullptr;
	};
	Node* head = nullptr;

	Node* copyNode(Node* other) {
		if (other == nullptr)
			return nullptr;
		Node* ptr = newNode(other->v.first, other->v.second);
		ptr->h = other->h;
		ptr->l = copyNode(other->l);
		if (ptr->l != nullptr)
			ptr->l->p = ptr;
		ptr->r = copyNode(other->r);
		if (ptr->r != nullptr)
			ptr->r->p = ptr;
		return ptr;
	}

	void clearNode(Node* ptr) {
		if (ptr == nullptr) return;
		clearNode(ptr->l);
		clearNode(ptr->r);
		delete ptr;
	}

	Node* newNode(const Key& k, const T& v) {
		Node* ptr = (Node*)::operator new(sizeof(Node));
		new (&(ptr->v)) value_type(k, v);
		ptr->h = 0;
		ptr->l = ptr->r = ptr->p = nullptr;
		return ptr;
	}

	Node* find_Node(const Key& key) {
		Node* ptr = head;
		while (ptr != nullptr) {
			if (cmp(key, ptr->v.first))
				ptr = ptr->l;
			else if (cmp(ptr->v.first, key))
				ptr = ptr->r;
			else
				break;
		}
		return ptr;
	}

	Node* const find_Node(const Key& key) const {
		Node* ptr = head;
		while (ptr != nullptr) {
			if (cmp(key, ptr->v.first))
				ptr = ptr->l;
			else if (cmp(ptr->v.first, key))
				ptr = ptr->r;
			else
				break;
		}
		return ptr;
	}

	Node* find_Node(const Key& key, Node* &parent) {
		Node* ptr = head;
		while (ptr != nullptr) {
			parent = ptr;
			if (cmp(key, ptr->v.first))
				ptr = ptr->l;
			else if (cmp(ptr->v.first, key))
				ptr = ptr->r;
			else
				break;
		}
		return ptr;
	}

	Node* insert_Node(const Key& key, const T& value) {
		if (head == nullptr) {
			head = newNode(key, value);
			++Size;
			return head;
		}
		Node* parent;
		Node* toinsert = find_Node(key, parent);
		if (toinsert != nullptr) return toinsert;
		toinsert = newNode(key, value);
		if (parent != nullptr) {
			if (cmp(key, parent->v.first)) 
				parent->l = toinsert;
			else
				parent->r = toinsert;
		}
		toinsert->p = parent;
		rebalance(parent);
		++Size;
		return toinsert;
	}

	Node* insert_Node(const Key& key) {
		T value;
		return insert_Node(key, value);
	}

	void swap_with_pre(Node* x) {
		Node* son = x->l;
		while (son->r != nullptr) son = son->r;
		if (son == x->l) {
			if (x->p != nullptr) {
				if (x->p->l == x)
					x->p->l = son;
				else
					x->p->r = son;
			}
			else
				head = son;
			if (x->r != nullptr)
				x->r->p = son;
			if (son->l != nullptr)
				son->l->p = x;
			std::swap(x->p, son->p);
			std::swap(x->l, son->l);
			std::swap(x->r, son->r);
			son->l = x;
			x->p = son;
			return;
		}
		// adjust x->l->p & son->p->r
			x->l->p = son;
			son->p->r = x;
		// adjust x->r->p & son->l->p
		if (x->r != nullptr)
			x->r->p = son;
		if (son->l != nullptr)
			son->l->p = x;
		// adjust x->p->l/r
		if (x->p != nullptr) {
			if (x->p->l == x)
				x->p->l = son;
			else
				x->p->r = son;
		}
		else
			head = son;
		// swap other ptr
		std::swap(x->p, son->p);
		std::swap(x->l, son->l);
		std::swap(x->r, son->r);
	}

	void erase_Node(Node* ptr) {
		if (ptr == nullptr)
			throw runtime_error();
		if (ptr->l != nullptr && ptr->r != nullptr) {
			swap_with_pre(ptr);
			erase_Node(ptr);
			return;
		}
		if (ptr->l == nullptr && ptr->r == nullptr) {
			if (ptr->p != nullptr) {
				if (ptr->p->l == ptr)
					ptr->p->l = nullptr;
				else
					ptr->p->r = nullptr;
				rebalance(ptr->p);
			}
			else head = nullptr;
			delete ptr;
			--Size;
			return;
		}
		Node* son;
		if (ptr->l != nullptr) son = ptr->l;
		else son = ptr->r;
		if (ptr->p != nullptr) {
			if (ptr->p->l == ptr)
				ptr->p->l = son;
			else
				ptr->p->r = son;
		}
		else
			head = son;
		son->p = ptr->p;
		delete ptr;
		--Size;
		rebalance(son);
	}

	Node* rotate_Right(Node* root) {
		Node* newroot = root->l;
		root->l = newroot->r;
		if (root->l != nullptr)
			root->l->p = root;
		newroot->r = root;
		if (root->p != nullptr) {
			if (root->p->l == root)
				root->p->l = newroot;
			else
				root->p->r = newroot;
		}
		else
			head = newroot;
		newroot->p = root->p;
		root->p = newroot;
		update_height(root);
		update_height(newroot);
		return newroot;
	}

	Node* rotate_Left(Node* root) {
		Node* newroot = root->r;
		root->r = newroot->l;
		if (root->r != nullptr)
			root->r->p = root;
		newroot->l = root;
		if (root->p != nullptr) {
			if (root->p->l == root)
				root->p->l = newroot;
			else
				root->p->r = newroot;
		}
		else
			head = newroot;
		newroot->p = root->p;
		root->p = newroot;
		update_height(root);
		update_height(newroot);
		return newroot;
	}

	int height(Node* ptr) {
		if (ptr == nullptr) return -1;
		return ptr->h;
	}

	void update_height(Node* ptr) {		
		ptr->h = std::max(height(ptr->l), height(ptr->r)) + 1;
	}

	void rebalance(Node* root) {
		int origin_h = root->h;
		update_height(root);
		int factor = height(root->l) - height(root->r);
		if (factor > 1) {
			if (height(root->l->l) >= height(root->l->r)) //LL
				root = rotate_Right(root);
			else { //LR
				root->l = rotate_Left(root->l);
				root = rotate_Right(root);
			}
		}
		else if (factor < -1) {
			if (height(root->r->r) >= height(root->r->l)) //RR
				root = rotate_Left(root);
			else { //RL
				root->r = rotate_Right(root->r);
				root = rotate_Left(root);
			}
		}
		if (root->p == nullptr) {
			head = root;
			return;
		}
		if (root->h == origin_h) return;
 		rebalance(root->p);
	}

public:
	/**
	 * see BidirectionalIterator at CppReference for help.
	 *
	 * if there is anything wrong throw invalid_iterator.
	 *     like it = map.begin(); --it;
	 *       or it = map.end(); ++end();
	 */
	class const_iterator;
	class iterator {
	private:
		Node* root = nullptr;
		Node* ptr = nullptr;
		int state = 1; // 0 for go down, 1 for stay, 2 for go up, 3 for end
	public:
		iterator() = default;
		iterator(const iterator &other) {
			ptr = other.ptr;
			root = other.root;
			state = other.state;
		}
		iterator(Node* p, Node* h, int s=1) {
			ptr = p;
			root = h;
			state = s;
		}
		void initial(Node* head) {
			root = ptr = head;
			if (head == nullptr) {
				state = 3;
				return;
			}
			while (ptr->l != nullptr)
				ptr = ptr->l;
			state = 1;
		}
		/**
		 * iter++
		 */
		iterator operator++(int) {
			iterator tmp = *this;
			++(*this);
			return tmp;
		}
		/**
		 * ++iter
		 */
		iterator & operator++() {
			if (ptr == nullptr || state == 3)
				throw invalid_iterator();
			if (ptr->r != nullptr) {
				ptr = ptr->r;
				state = 0;
			}
			else if (ptr->p == nullptr) {
				state = 3;
				return *this;
			}
			else if (ptr->p->l == ptr) {
				ptr = ptr->p;
				state = 1;
			}
			else {
				ptr = ptr->p;
				state = 2;
			}
			while (state != 1) {
				if (state == 0) {
					if (ptr->l != nullptr)
						ptr = ptr->l;
					else
						state = 1;
				}
				else if (ptr->p == nullptr) {
					state = 3;
					break;
				}
				else if (ptr->p->l == ptr) {
					ptr = ptr->p;
					state = 1;
				}
				else {
					ptr = ptr->p;
					state = 2;
				}
			}
			return *this;
		}
		/**
		 * iter--
		 */
		iterator operator--(int) {
			iterator tmp = *this;
			--(*this);
			return tmp;
		}
		/**
		 * --iter
		 */
		iterator & operator--() {
			if (state == 3) {
				if (ptr == nullptr)
					throw invalid_iterator();
				while (ptr->r != nullptr)
					ptr = ptr->r;
				state = 1;
				return *this;
			}
			if (ptr->l != nullptr) {
				ptr = ptr->l;
				state = 0;
			}
			else if (ptr->p == nullptr) {
				state = 1;
				throw invalid_iterator();
			}
			else if (ptr->p->r == ptr) {
				ptr = ptr->p;
				state = 1;
			}
			else {
				ptr = ptr->p;
				state = 2;
			}
			while (state != 1) {
				if (state == 0) {
					if (ptr->r != nullptr)
						ptr = ptr->r;
					else
						state = 1;
				}
				else if (ptr->p == nullptr) {
					state = 1;
					throw invalid_iterator();
				}
				else if (ptr->p->r == ptr) {
					ptr = ptr->p;
					state = 1;
				}
				else {
					ptr = ptr->p;
					state = 2;
				}
			}
			return *this;
		}
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory).
		 */
		value_type & operator*() const {
			return ptr->v;
		}
		bool operator==(const iterator &rhs) const {
			return ptr == rhs.ptr && state == rhs.state;
		}
		bool operator==(const const_iterator &rhs) const {
			return ptr == rhs.ptr && state == rhs.state;
		}
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
			return !(*this == rhs);
		}
		bool operator!=(const const_iterator &rhs) const {
			return !(*this == rhs);
		}

		/**
		 * for the support of it->first. 
		 * See <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/> for help.
		 */
		value_type* operator->() const noexcept {
			return &(ptr->v);
		}

		friend void map::erase(iterator pos);
		friend class const_iterator;
	};
	class const_iterator {
		// it should has similar member method as iterator.
		//  and it should be able to construct from an iterator.
	private:
		Node* root = nullptr;
		Node* ptr = nullptr;
		int state = 0;
	public:
		const_iterator() = default;
		const_iterator(const const_iterator &other) {
			ptr = other.ptr;
			state = other.state;
		}
		const_iterator(const iterator &other) {
			ptr = other.ptr;
			state = other.state;
		}
		const_iterator(Node* p, Node* h, int s=1) {
			ptr = p;
			root = h;
			state = s;
		}
		void initial(Node* head) {
			root = ptr = head;
			if (head == nullptr) {
				state = 3;
				return;
			}
			while (ptr->l != nullptr)
				ptr = ptr->l;
			state = 1;
		}
		/**
		 * iter++
		 */
		const_iterator operator++(int) {
			const_iterator tmp = *this;
			++(*this);
			return tmp;
		}
		/**
		 * ++iter
		 */
		const_iterator & operator++() {
			if (ptr == nullptr || state == 3)
				throw invalid_iterator();
			if (ptr->r != nullptr) {
				ptr = ptr->r;
				state = 0;
			}
			else if (ptr->p == nullptr) {
				state = 3;
				return *this;
			}
			else if (ptr->p->l == ptr) {
				ptr = ptr->p;
				state = 1;
			}
			else {
				ptr = ptr->p;
				state = 2;
			}
			while (state != 1) {
				if (state == 0) {
					if (ptr->l != nullptr)
						ptr = ptr->l;
					else
						state = 1;
				}
				else if (ptr->p == nullptr) {
					state = 3;
					break;
				}
				else if (ptr->p->l == ptr) {
					ptr = ptr->p;
					state = 1;
				}
				else {
					ptr = ptr->p;
					state = 2;
				}
			}
			return *this;
		}
		/**
		 * iter--
		 */
		const_iterator operator--(int) {
			const_iterator tmp = *this;
			--(*this);
			return tmp;
		}
		/**
		 * --iter
		 */
		const_iterator & operator--() {
			if (state == 3) {
				if (ptr == nullptr) 
					throw invalid_iterator();
				while (ptr->r != nullptr)
					ptr = ptr->r;
				state = 1;
				return *this;
			}
			if (ptr->l != nullptr) {
				ptr = ptr->l;
				state = 0;
			}
			else if (ptr->p == nullptr) {
				state = 1;
				throw invalid_iterator();
			}
			else if (ptr->p->r == ptr) {
				ptr = ptr->p;
				state = 1;
			}
			else {
				ptr = ptr->p;
				state = 2;
			}
			while (state != 1) {
				if (state == 0) {
					if (ptr->r != nullptr)
						ptr = ptr->r;
					else
						state = 1;
				}
				else if (ptr->p == nullptr) {
					state = 1;
					throw invalid_iterator();
				}
				else if (ptr->p->r == ptr) {
					ptr = ptr->p;
					state = 1;
				}
				else {
					ptr = ptr->p;
					state = 2;
				}
			}
			return *this;
		}
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory).
		 */
		const value_type & operator*() const {
			return ptr->v;
		}
		bool operator==(const iterator &rhs) const {
			return ptr == rhs.ptr && state == rhs.state;
		}
		bool operator==(const const_iterator &rhs) const {
			return ptr == rhs.ptr && state == rhs.state;
		}
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
			return !(*this == rhs);
		}
		bool operator!=(const const_iterator &rhs) const {
			return !(*this == rhs);
		}
		value_type* const operator->() const noexcept {
			return &(ptr->v);
		}
		friend class iterator;
	};
	/**
	 * two constructors
	 */
	map() = default;
	map(const map &other) {
		head = copyNode(other.head);
		Size = other.Size;
	}
	/**
	 * assignment operator
	 */
	map & operator=(const map &other) {
		if (&other == this) return *this;
		clearNode(head);
		head = copyNode(other.head);
		Size = other.Size;
		return *this;
	}
	/**
	 * Destructors
	 */
	~map() {
		clearNode(head);
	}
	/**
	 * access specified element with bounds checking
	 * Returns a reference to the mapped value of the element with key equivalent to key.
	 * If no such element exists, an exception of type `index_out_of_bound'
	 */
	T & at(const Key &key) {
		Node* found = find_Node(key);
		if (found == nullptr)
			throw index_out_of_bound();
		return found->v.second;
	}
	const T & at(const Key &key) const {
		Node* found = find_Node(key);
		if (found == nullptr)
			throw index_out_of_bound();
		return found->v.second;
	}
	/**
	 * access specified element 
	 * Returns a reference to the value that is mapped to a key equivalent to key,
	 *   performing an insertion if such key does not already exist.
	 */
	T & operator[](const Key &key) {
		Node* ptr = insert_Node(key);
		return ptr->v.second;
	}
	/**
	 * behave like at() throw index_out_of_bound if such key does not exist.
	 */
	const T & operator[](const Key &key) const {
		Node* found = find_Node(key);
		if (found == nullptr)
			throw index_out_of_bound();
		return found->v.second;
	}
	/**
	 * return a iterator to the beginning
	 */
	iterator begin() {
		iterator res;
		res.initial(head);
		return res;
	}
	const_iterator cbegin() const {
		const_iterator res;
		res.initial(head);
		return res;
	}
	/**
	 * return a iterator to the end
	 * in fact, it returns past-the-end.
	 */
	iterator end() {
		return iterator(head, head, 3);
	}
	const_iterator cend() const {
		return const_iterator(head, head, 3);
	}
	/**
	 * checks whether the container is empty
	 * return true if empty, otherwise false.
	 */
	bool empty() const {
		return Size == 0;
	}
	/**
	 * returns the number of elements.
	 */
	size_t size() const {
		return Size;
	}
	/**
	 * clears the contents
	 */
	void clear() {
		clearNode(head);
		Size = 0;
		head = nullptr;
	}
	/**
	 * insert an element.
	 * return a pair, the first of the pair is
	 *   the iterator to the new element (or the element that prevented the insertion), 
	 *   the second one is true if insert successfully, or false.
	 */
	pair<iterator, bool> insert(const value_type &value) {
		size_t pre_size = Size;
		Node* ptr = insert_Node(value.first, value.second);
		if (Size == pre_size)
			return pair<iterator, bool>(iterator(ptr, head), false);
		return pair<iterator, bool>(iterator(ptr, head), true);
	}
	/**
	 * erase the element at pos.
	 *
	 * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
	 */
	void erase(iterator pos) {
		if (pos.state != 1 || pos.root != head)
			throw invalid_iterator();
		try {
			erase_Node(pos.ptr);
		} catch(...) {
			throw runtime_error();
		}
	}
	/**
	 * Returns the number of elements with key 
	 *   that compares equivalent to the specified argument,
	 *   which is either 1 or 0 
	 *     since this container does not allow duplicates.
	 * The default method of check the equivalence is !(a < b || b > a)
	 */
	size_t count(const Key &key) const {
		Node* const found = find_Node(key);
		if (found == nullptr) return 0;
		return 1;
	}
	/**
	 * Finds an element with key equivalent to key.
	 * key value of the element to search for.
	 * Iterator to an element with key equivalent to key.
	 *   If no such element is found, past-the-end (see end()) iterator is returned.
	 */
	iterator find(const Key &key) {
		Node* found = find_Node(key);
		if (found == nullptr) return end();
		return iterator(found, head);
	}
	const_iterator find(const Key &key) const {
		Node* found = find_Node(key);
		if (found == nullptr) return cend();
		return const_iterator(found, head);
	}
};

}

#endif
