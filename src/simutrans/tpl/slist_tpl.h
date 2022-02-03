/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TPL_SLIST_TPL_H
#define TPL_SLIST_TPL_H


#include <iterator>
#include <typeinfo>
#include "../dataobj/freelist.h"
#include "../simdebug.h"
#include <stddef.h> // for ptrdiff_t

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif


/**
 * A template class for a single linked list. Insert() and append()
 * work in fixed time. Maintains a list of free nodes to reduce calls
 * to new and delete.
 *
 * Must NOT be used with things with copy constructor like button_t or std::string!!!
 */
template<class T>
class slist_tpl
{
private:
	struct node_t
	{
		node_t(const T& data_, node_t* next_) : next(next_), data(data_) {}
		node_t(node_t* next_) : next(next_), data() {}

		void* operator new(size_t) { return freelist_t::gimme_node(sizeof(node_t)); }
		void operator delete(void* p) { freelist_t::putback_node(sizeof(node_t), p); }

		node_t* next;
		T data;
	};

	node_t *head;
	node_t *tail;
	uint32 node_count;

public:
	class const_iterator;

	/**
	 * Iterator class: can be used to erase nodes and to modify nodes
	 * Usage: @see utils/for.h
	 */
	class iterator
	{
	public:
		typedef std::forward_iterator_tag iterator_category;
		typedef T                         value_type;
		typedef ptrdiff_t                 difference_type;
		typedef value_type*               pointer;
		typedef value_type&               reference;

		iterator() : ptr(), pred() {}

		pointer   operator ->() const { return &ptr->data; }
		reference operator *()  const { return ptr->data;  }

		iterator& operator ++()
		{
			pred = ptr;
			ptr  = ptr->next;
			return *this;
		}

		iterator operator ++(int)
		{
			iterator const old = *this;
			++*this;
			return old;
		}

		bool operator ==(iterator const& o) const { return ptr == o.ptr; }
		bool operator !=(iterator const& o) const { return ptr != o.ptr; }

	private:
		iterator(node_t* ptr_, node_t* pred_) : ptr(ptr_), pred(pred_) {}

		node_t* ptr;
		node_t* pred;

	private:
		friend class slist_tpl;
		friend class const_iterator;
	};

	/**
	 * Iterator class: neither nodes nor the list can be modified
	 * Usage: @see utils/for.h
	 */
	class const_iterator
	{
	public:
		typedef std::forward_iterator_tag iterator_category;
		typedef T                         value_type;
		typedef ptrdiff_t                 difference_type;
		typedef T const*                  pointer;
		typedef T const&                  reference;

		const_iterator() : ptr() {}

		const_iterator(const iterator& o) : ptr(o.ptr) {}

		pointer   operator ->() const { return &ptr->data; }
		reference operator *()  const { return ptr->data;  }

		const_iterator& operator ++() { ptr = ptr->next; return *this; }

		const_iterator operator ++(int)
		{
			const_iterator const old = *this;
			++*this;
			return old;
		}

		bool operator ==(const_iterator const& o) const { return ptr == o.ptr; }
		bool operator !=(const_iterator const& o) const { return ptr != o.ptr; }

	private:
		explicit const_iterator(node_t* ptr_) : ptr(ptr_) {}

		const node_t* ptr;

	private:
		friend class slist_tpl;
	};

	/**
	 * Creates a new empty list.
	 */
	slist_tpl()
	{
		head = 0;             // empty list
		tail = 0;
		node_count = 0;
	}

	~slist_tpl()
	{
		clear();
	}

	/**
	 * Inserts an element at the beginning of the list.
	 */
	void insert(const T& data)
	{
		node_t* tmp = new node_t(data, head);
		head = tmp;
		if(  tail == NULL  ) {
			tail = tmp;
		}
		node_count++;
	}

	/**
	 * Inserts an element initialized by standard constructor
	 * at the beginning of the lists
	 * (avoid the use of copy constructor)
	 */
	void insert()
	{
		node_t* tmp = new node_t(head);
		head = tmp;
		if(  tail == NULL  ) {
			tail = tmp;
		}
		node_count++;
	}

	/**
	 * Appends an element to the end of the list.
	 */
	void append(const T& data)
	{
		if (tail == 0) {
			insert(data);
		}
		else {
			node_t* tmp = new node_t(data, 0);
			tail->next = tmp;
			tail = tmp;
			node_count++;
		}
	}

	/**
	 * Append an zero/empty element
	 * mostly used for T=slist_tpl<...>
	 */
	void append()
	{
		if (tail == 0) {
			insert();
		}
		else {
			node_t* tmp = new node_t(0);
			tail->next = tmp;
			tail = tmp;
			node_count++;
		}
	}

	/**
	 * Appends an element to the end of the list.
	 */
	void append_unique(const T& data)
	{
		if (tail == 0) {
			insert(data);
		}
		else if(  !is_contained(data)  ) {
			node_t* tmp = new node_t(data, 0);
			tail->next = tmp;
			tail = tmp;
			node_count++;
		}
	}

	/**
	 * Appends the nodes of another list
	 * empties other list
	 * -> no memory allocation involved
	 */
	void append_list(slist_tpl<T>& other)
	{
		if (tail) {
			tail->next = other.head;
		}
		else {
			head = other.head;
		}
		if (other.tail) {
			tail = other.tail;
		}
		node_count += other.node_count;

		// empty other list
		other.tail = NULL;
		other.head = NULL;
		other.node_count = 0;
	}

	/**
	 * Checks if the given element is already contained in the list.
	 */
	bool is_contained(const T &data) const
	{
		node_t *p = head;

		while(p != 0 && p->data != data) {
			p = p->next;
		}
		return p != 0;         // is NULL when not found
	}

	/**
	 * Removes an element from the list
	 */
	bool remove(const T &data)
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
	 * deleted from the list. Useful for some queuing tasks.
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

	uint32 get_count() const
	{
		return node_count;
	}

	bool empty() const { return head == 0; }

	T& at(uint32 pos) const
	{
		if (pos >= node_count) {
			dbg->fatal("slist_tpl<T>::at()", "<%s> index %d is out of bounds", typeid(T).name(), pos);
		}

		node_t* p = head;
		while (pos--) {
			p = p->next;
		}
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
		}
		else {
			pred->next = succ;
		}
		if (succ == NULL) {
			tail = pred;
		}
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
		}
		else {
			pos.pred->next = tmp;
		}
		if (pos.ptr == NULL) {
			tail = tmp;
		}
		++node_count;
		return iterator(tmp, pos.pred);
	}

	/* Add an empty node
	 * pos is invalid after this method
	 * An iterator pointing to the new element is returned */
	iterator insert(iterator pos)
	{
		node_t* tmp = new node_t(pos.ptr);
		if (pos.pred == NULL) {
			head = tmp;
		}
		else {
			pos.pred->next = tmp;
		}
		if (pos.ptr == NULL) {
			tail = tmp;
		}
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

	/**
	 * sorts list using specified comparator
	 */
	void sort( int (*compare)(const T &l, const T &r) ){
		if(  NULL == head  ||  head == tail  ) {
			return;
		}

		for(  uint i=1;  i < node_count;  ++i  ) {
			int changes = 0;
			if(  compare( head->data, head->next->data ) > 0  ) {
				node_t * tmp = head;
				head = head->next;
				tmp->next = head->next;
				head->next = tmp;
				++changes;
				if(  head == tail  ) {
					tail = tail->next;
					break;
				}
			}
			for(  node_t *node = head;  node != tail  &&  node->next != tail;  node = node->next  ) {
				if(  compare( node->next->data, node->next->next->data ) > 0  ) {
					node_t * tmp = node->next;
					node->next = node->next->next;
					tmp->next = node->next->next;
					node->next->next = tmp;
					++changes;
					if(  node->next == tail  ){
						tail = tail->next;
						break;
					}
				}
			}
			if(  changes == 0  ) {
				break;
			}
		}
	}

private:
	slist_tpl(const slist_tpl& slist_tpl);
	slist_tpl& operator=( slist_tpl const& other );

};

#endif
