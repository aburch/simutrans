/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef tpl_slist_tpl_h
#define tpl_slist_tpl_h

#include <iterator>
#include <typeinfo>
#include "../dataobj/freelist.h"
#include "../simdebug.h"

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif


template<class T> class slist_iterator_tpl;


/**
 * A template class for a single linked list. Insert() and append()
 * work in fixed time. Maintains a list of free nodes to reduce calls
 * to new and delete.
 *
 * Must NOT be used with things with copy contructor like button_t or cstring_t!!!
 *
 * @date November 2000
 * @author Hj. Malthaner
 */

template<class T>
class slist_tpl
{
private:
	struct node_t
	{
		node_t(const T& data_, node_t* next_) : next(next_), data(data_) {}

		void* operator new(size_t) { return freelist_t::gimme_node(sizeof(node_t)); }
		void operator delete(void* p) { freelist_t::putback_node(sizeof(node_t), p); }

		node_t* next;
		T data;
	};

	node_t * head;
	node_t * tail;
	unsigned int node_count;

	friend class slist_iterator_tpl<T>;

public:
		class const_iterator;

		class iterator
		{
			public:
				typedef std::forward_iterator_tag iterator_category;
				typedef T                         value_type;
				typedef ptrdiff_t                 difference_type;
				typedef value_type*               pointer;
				typedef value_type&               reference;

				pointer   operator ->() const { return &ptr->data; }
				reference operator *()  const { return ptr->data;  }

				iterator& operator ++()
				{
					pred = ptr;
					ptr  = ptr->next;
					return *this;
				}

				bool operator ==(const iterator& o) { return ptr == o.ptr; }
				bool operator !=(const iterator& o) { return ptr != o.ptr; }

			private:
				iterator(node_t* ptr_, node_t* pred_) : ptr(ptr_), pred(pred_) {}

				node_t* ptr;
				node_t* pred;

			friend class slist_tpl;
			friend class const_iterator;
		};

		class const_iterator
		{
			public:
				typedef std::forward_iterator_tag iterator_category;
				typedef const T                   value_type;
				typedef ptrdiff_t                 difference_type;
				typedef value_type*               pointer;
				typedef value_type&               reference;

				const_iterator(const iterator& o) : ptr(o.ptr) {}

				pointer   operator ->() const { return &ptr->data; }
				reference operator *()  const { return ptr->data;  }

				const_iterator& operator ++() { ptr = ptr->next; return *this; }

				bool operator ==(const const_iterator& o) { return ptr == o.ptr; }
				bool operator !=(const const_iterator& o) { return ptr != o.ptr; }

			private:
				explicit const_iterator(node_t* ptr_) : ptr(ptr_) {}

				const node_t* ptr;

			friend class slist_tpl;
		};

	int node_size() // debuging
	{
		return sizeof(node_t);
	}

	/**
	 * Creates a new empty list.
	 *
	 * @author Hj. Malthaner
	 */
	slist_tpl()
	{
		head = 0;             // leere liste
		tail = 0;
		node_count = 0;
	}

	~slist_tpl()
	{
		clear();
	}

	/**
	 * Inserts an element at the beginning of the list.
	 *
	 * @author Hj. Malthaner
	 */
	void insert(const T& data)
	{
		node_t* tmp = new node_t(data, head);
		head = tmp;
		if (tail == NULL) tail = tmp;
		node_count++;
	}

	/**
	 * Appends an element to the end of the list.
	 *
	 * @author Hj. Malthaner
	 */
	void append(const T& data)
	{
		if (tail == 0) {
			insert(data);
		} else {
			node_t* tmp = new node_t(data, 0);
			tail->next = tmp;
			tail = tmp;
			node_count++;
		}
	}

	/**
	 * Checks if the given element is already contained in the list.
	 *
	 * @author Hj. Malthaner
	 */
	bool contains(const T data) const
	{
		node_t *p = head;

		while(p != 0 && p->data != data) {
			p = p->next;
		}
		return p != 0;         // ist NULL wenn nicht gefunden
	}

	/**
	 * Removes an element from the list
	 *
	 * @author Hj. Malthaner
	 */
	bool remove(const T data)
	{
		if (empty()) {
			//MESSAGE("slist_tpl<T>::remove()", "data not in list!");
			return false;
		}

		if(head->data == data) {
			node_t *tmp = head->next;
			delete head;
			head = tmp;

			if(head == NULL) {
				tail = NULL;
			}
		}
		else {
			node_t *p = head;

			while(p->next != 0 && !(p->next->data == data)) {
				p = p->next;
			}
			if(p->next == 0) {
				//MESSAGE("slist_tpl<T>::remove()", "data not in list!");
				return false;
			}
			node_t *tmp = p->next->next;
			delete p->next;
			p->next = tmp;

			if(tmp == 0) {
				tail = p;
			}
		}
		node_count--;
		return true;
	}

	/**
	 * Retrieves the first element from the list. This element is
	 * deleted from the list. Useful for some queueing tasks.
	 * @author Hj. Malthaner
	 */
	T remove_first()
	{
		if (empty()) {
			dbg->fatal("slist_tpl<T>::remove_first()","List of <%s> is empty",typeid(T).name());
		}

		T tmp = head->data;
		node_t *p = head;

		head = head->next;
		delete p;

		node_count--;

		if(head == 0) {
			// list is empty now
			tail = 0;
		}

		return tmp;
	}

	/**
	 * Recycles all nodes.
	 * Leaves the list empty.
	 * @author Hj. Malthaner
	 */
	void clear()
	{
		node_t* p = head;
		while (p != NULL) {
			node_t* tmp = p;
			p = p->next;
			delete tmp;
		}
		head = 0;
		tail = 0;
		node_count = 0;
	}

	unsigned int count() const
	{
		return node_count;
	}

	bool empty() const { return head == 0; }

	T& at(uint pos) const
	{
		if (pos >= count()) {
			dbg->fatal("slist_tpl<T>::at()", "<%s> index %d is out of bounds", typeid(T).name(), pos);
		}

		node_t* p = head;
		while (pos--) p = p->next;
		return p->data;
	}

	T& front() const { return head->data; }
	T& back()  const { return tail->data; }

	iterator begin() { return iterator(head, NULL); }
	iterator end()   { return iterator(NULL, tail); }

	const_iterator begin() const { return const_iterator(head); }
	const_iterator end()   const { return const_iterator(NULL); }

	/* Erase element at pos
	 * pos is invalid after this method
	 * An iterator pointing to the successor of the erased element is returned */
	iterator erase(iterator pos)
	{
		node_t* pred = pos.pred;
		node_t* succ = pos.ptr->next;
		if (pred == NULL) {
			head = succ;
		} else {
			pred->next = succ;
		}
		if (succ == NULL) tail = pred;
		delete pos.ptr;
		--node_count;
		return iterator(succ, pred);
	}

	/* Add x before pos
	 * pos is invalid after this method
	 * An iterator pointing to the new element is returned */
	iterator insert(iterator pos, const T& x)
	{
		node_t* tmp = new node_t(x, pos.ptr);
		if (pos.pred == NULL) {
			head = tmp;
		} else {
			pos.pred->next = tmp;
		}
		if (pos.ptr == NULL) tail = tmp;
		++node_count;
		return iterator(tmp, pos.pred);
	}

	int index_of(T data) const
	{
		node_t *t = head;
		int index = 0;

		while(t && t->data != data) {
			t = t->next;
			index++;
		}
		return t ? index : -1;
	}
};



/**
 * Iterator class for single linked lists.
 * Iterators may be invalid after any changing operation on the list!
 * @author Hj. Malthaner
 */
template<class T>
class slist_iterator_tpl
{
private:
    typename slist_tpl<T>::node_t *current_node;
    typename slist_tpl<T>::node_t lead;	// element zero

public:
	slist_iterator_tpl(const slist_tpl<T>* list) :
		current_node(&lead),
		lead(T(), list->head)
	{}

	slist_iterator_tpl(const slist_tpl<T>& list) :
		current_node(&lead),
		lead(T(), list.head)
	{}

	slist_iterator_tpl<T> &operator = (const slist_iterator_tpl<T> &iter)
	{
		lead = iter.lead;
		if(iter.current_node == &iter.lead) {
			current_node = &lead;
		} else {
			current_node = iter.current_node;
		}
		return *this;
	}

	/**
	 * start iteration
	 * @author Hj. Malthaner
	 */
	void begin()
	{
		current_node = &lead;
	}


	void begin(const slist_tpl<T> *list)
	{
		current_node = &lead;
		lead.next = list->head;
	}

	/**
	 * iterate next element
	 * @return false, if no more elements
	 * @author Hj. Malthaner
	 */
	bool next()
	{
		current_node = current_node->next;
		return (current_node!= 0);
	}

	/**
	 * @return the current element (as const reference)
	 * @author Hj. Malthaner
	 */
	const T& get_current() const
	{
		if(current_node==&lead) {
			dbg->fatal("class slist_iterator_tpl.get_current()","Iteration: accesed lead!");
		}
		return current_node->data;
	}


	/**
	 * @return the current element (as reference)
	 * @author Hj. Malthaner
	 */
	T& access_current()
	{
		if(current_node==&lead) {
			dbg->fatal("class slist_iterator_tpl.get_current()","Iteration: accesed lead!");
		}
		return current_node->data;
	}
};

#endif
