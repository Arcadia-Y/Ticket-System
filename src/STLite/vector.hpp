#ifndef SJTU_VECTOR_HPP
#define SJTU_VECTOR_HPP

#include "exceptions.hpp"
#include <climits>
#include <cstddef>

namespace sjtu 
{
/**
 * a data container like std::vector
 * store data in a successive memory and support random access.
 */
template<typename T>
class vector 
{
	T* head;
	size_t max;
	size_t now;
	void double_space()
	{
		T* tmp = (T*)malloc(max*2 * sizeof(T));
		for (size_t i = 0; i < now; ++i) new(tmp+i) T(head[i]);
		for (size_t i = 0; i < now; ++i) (head+i)->~T();
		free(head);
		head = tmp;
		max *= 2;
	}

public:
	/**
	 * a type for actions of the elements of a vector, and you should write
	 *   a class named const_iterator with same interfaces.
	 */
	/**
	 * you can see RandomAccessIterator at CppReference for help.
	 */
	class const_iterator;
	class iterator 
	{
	// The following code is written for the C++ type_traits library.
	// Type traits is a C++ feature for describing certain properties of a type.
	// For instance, for an iterator, iterator::value_type is the type that the 
	// iterator points to. 
	// STL algorithms and containers may use these type_traits (e.g. the following 
	// typedef) to work properly. In particular, without the following code, 
	// @code{std::sort(iter, iter1);} would not compile.
	// See these websites for more information:
	// https://en.cppreference.com/w/cpp/header/type_traits
	// About value_type: https://blog.csdn.net/u014299153/article/details/72419713
	// About iterator_category: https://en.cppreference.com/w/cpp/iterator
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using iterator_category = std::output_iterator_tag;

	private:
		size_t id;
		T* head;

	public:
		iterator(T* HEAD, size_t ID) : head(HEAD), id(ID) {}
		/**
		 * return a new iterator which pointer n-next elements
		 * as well as operator-
		 */
		iterator operator+(const int &n) const 
		{
			return iterator(head, id+n);
		}
		iterator operator-(const int &n) const 
		{
			return iterator(head, id-n);
		}
		// return the distance between two iterators,
		// if these two iterators point to different vectors, throw invaild_iterator.
		int operator-(const iterator &rhs) const 
		{
			if (head != rhs.head) throw invalid_iterator();
			return id - rhs.id;
		}
		iterator& operator+=(const int &n) 
		{
			id += n;
			return *this;
		}
		iterator& operator-=(const int &n) 
		{
			id -= n;
			return *this;
		}
		/**
		 * iter++
		 */
		iterator operator++(int) 
		{
			id++;
			return iterator(head, id-1);
		}
		/**
		 * ++iter
		 */
		iterator& operator++() 
		{
			++id;
			return *this;
		}
		/**
		 * iter--
		 */
		iterator operator--(int) 
		{
			id--;
			return iterator(head, id+1);
		}
		/**
		 * --iter
		 */
		iterator& operator--() 
		{
			--id;
			return *this;
		}
		/**
		 * *it
		 */
		T& operator*() const
		{
			try
			{
				return head[id];
			}
			catch(...)
			{
				throw runtime_error();
			}
		}
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory address).
		 */
		bool operator==(const iterator &rhs) const 
		{
			if (head == rhs.head && id == rhs.id) return true;
			return false;
		}
		bool operator==(const const_iterator &rhs) const 
		{
			if (head == rhs.head && id == rhs.id) return true;
			return false;
		}
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const 
		{
			return !((*this) == rhs);
		}
		bool operator!=(const const_iterator &rhs) const 
		{
			return !((*this) == rhs);
		}
		// show head and id of the iterator
		T* showhead() const
		{
			return head;
		}
		size_t showid() const
		{
			return id;
		}
	};
	/**
	 * has same function as iterator, just for a const object.
	 */
	class const_iterator 
	{
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using iterator_category = std::output_iterator_tag;
	
	private:
		size_t id;
		T* head;

	public:
		const_iterator(T* HEAD, size_t ID) : head(HEAD), id(ID) {}
		const_iterator operator+(const int &n) const 
		{
			return const_iterator(head, id+n);
		}
		const_iterator operator-(const int &n) const 
		{
			return const_iterator(head, id-n);
		}
		int operator-(const const_iterator &rhs) const 
		{
			if (head != rhs.head) throw invalid_iterator();
			return id - rhs.id;
		}
		const_iterator& operator+=(const int &n) 
		{
			id += n;
			return *this;
		}
		const_iterator& operator-=(const int &n) 
		{
			id -= n;
			return *this;
		}
		const_iterator operator++(int) 
		{
			id++;
			return iterator(head, id-1);
		}
		const_iterator& operator++() 
		{
			++id;
			return *this;
		}
		const_iterator operator--(int) 
		{
			id--;
			return iterator(head, id+1);
		}
		const_iterator& operator--() 
		{
			--id;
			return *this;
		}
		const T& operator*() const
		{
			try
			{
				return head[id];
			}
			catch(...)
			{
				throw runtime_error();
			}
		}
		bool operator==(const iterator &rhs) const 
		{
			if (head == rhs.head && id == rhs.id) return true;
			return false;
		}
		bool operator==(const const_iterator &rhs) const 
		{
			if (head == rhs.head && id == rhs.id) return true;
			return false;
		}
		bool operator!=(const iterator &rhs) const 
		{
			return !((*this) == rhs);
		}
		bool operator!=(const const_iterator &rhs) const 
		{
			return !((*this) == rhs);
		}
	};
	/**
	 * Constructs
	 * At least two: default constructor, copy constructor
	 */
	vector() 
	{
		head = (T*)malloc(8*sizeof(T));
		max = 8;
		now = 0;
	}
	vector(const vector &other) 
	{
		max = other.max;
		now = other.now;
		head = (T*)malloc(max * sizeof(T));
		for (size_t i = 0; i < now; ++i) new(head+i) T(other.head[i]);
	}
	/**
	 * Destructor
	 */
	~vector() 
	{
		for (size_t i = 0; i < now; ++i) (head+i)->~T();
		free(head);
	}
	/**
	 * Assignment operator
	 */
	vector &operator=(const vector &other) 
	{
		if (this == &other) return *this;
		for (size_t i = 0; i < now; ++i) (head+i)->~T();
		free(head);
		max = other.max;
		now = other.now;
		head = (T*)malloc(max * sizeof(T));
		for (size_t i = 0; i < now; ++i) new(head+i) T(other.head[i]);
		return *this;
	}
	/**
	 * assigns specified element with bounds checking
	 * throw index_out_of_bound if pos is not in [0, size)
	 */
	T & at(const size_t &pos) 
	{
		if (pos < 0 || pos >= now)
			throw index_out_of_bound();
		return head[pos];
	}
	const T & at(const size_t &pos) const 
	{
		if (pos < 0 || pos >= now)
			throw index_out_of_bound();
		return head[pos];
	}
	/**
	 * assigns specified element with bounds checking
	 * throw index_out_of_bound if pos is not in [0, size)
	 * !!! Pay attentions
	 *   In STL this operator does not check the boundary but I want you to do.
	 */
	T & operator[](const size_t &pos) 
	{
		if (pos < 0 || pos >= max)
			throw index_out_of_bound();
		return head[pos];
	}
	const T & operator[](const size_t &pos) const 
	{
		if (pos < 0 || pos >= max)
			throw index_out_of_bound();
		return head[pos];
	}
	/**
	 * access the first element.
	 * throw container_is_empty if size == 0
	 */
	const T & front() const 
	{
		if (now == 0) throw container_is_empty();
		return head[0];
	}
	/**
	 * access the last element.
	 * throw container_is_empty if size == 0
	 */
	const T & back() const 
	{
		if (now == 0) throw container_is_empty();
		return head[now-1];
	}
	/**
	 * returns an iterator to the beginning.
	 */
	iterator begin() 
	{
		return iterator(head, 0);
	}
	const_iterator cbegin() const 
	{
		return const_iterator(head, 0);
	}
	/**
	 * returns an iterator to the end.
	 */
	iterator end() 
	{
		return iterator(head, now);
	}
	const_iterator cend() const 
	{
		return const_iterator(head, now);
	}
	/**
	 * checks whether the container is empty
	 */
	bool empty() const 
	{
		return now == 0;
	}
	/**
	 * returns the number of elements
	 */
	size_t size() const 
	{
		return now;
	}
	/**
	 * clears the contents
	 */
	void clear() 
	{
		for (size_t i = 0; i < now; ++i) (head+i)->~T();
		free(head);
		head = (T*) malloc(8*sizeof(T));
		max = 8;
		now = 0;
	}
	/**
	 * inserts value before pos
	 * returns an iterator pointing to the inserted value.
	 */
	iterator insert(iterator pos, const T &value) 
	{
		if (pos.showhead() != head) throw invalid_iterator();
		return insert(pos.showid(), value);
	}
	/**
	 * inserts value at index ind.
	 * after inserting, this->at(ind) == value
	 * returns an iterator pointing to the inserted value.
	 * throw index_out_of_bound if ind > size (in this situation ind can be size because after inserting the size will increase 1.)
	 */
	iterator insert(const size_t &ind, const T &value) 
	{
		if (ind > now) throw index_out_of_bound();
		for (size_t i = now; i > ind; --i)
			new(head+i) T(head[i-1]);
		new(head+ind) T(value);
		++now;
		if (now >= max) double_space();
		return iterator(head, ind);
	}
	/**
	 * removes the element at pos.
	 * return an iterator pointing to the following element.
	 * If the iterator pos refers the last element, the end() iterator is returned.
	 */
	iterator erase(iterator pos) 
	{
		if (pos.showhead() != head) throw invalid_iterator();
		return erase(pos.showid());
	}
	/**
	 * removes the element with index ind.
	 * return an iterator pointing to the following element.
	 * throw index_out_of_bound if ind >= size
	 */
	iterator erase(const size_t &ind) 
	{
		if (ind >= now) throw index_out_of_bound();
		for (size_t i = ind; i < now-1; ++i)
			head[i] = head[i+1];
		--now;
		(head+now)->~T();
		return iterator(head, ind); 
	}
	/**
	 * adds an element to the end.
	 */
	void push_back(const T &value) 
	{
		new(head+now) T(value);
		++now;
		if (now >= max) double_space();
	}
	/**
	 * remove the last element from the end.
	 * throw container_is_empty if size() == 0
	 */
	void pop_back() 
	{
		if (now == 0) throw container_is_empty();
		--now;
		(head+now)->~T();
	}
	// reserve x*sizeof(T) space for UNUSED vector
	void reserve(size_t x)
	{
		if (x <= 8) return;
		free(head);
		head = (T*) malloc(x*sizeof(T));
		max = x;
		now = 0;
	}
};

}

#endif
