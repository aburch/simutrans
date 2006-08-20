/*
 * slist_tpl.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef tpl_slist_tpl_h
#define tpl_slist_tpl_h

#include <stdlib.h>
#include <typeinfo>

#include "../dataobj/freelist.h"

#include "no_such_element_exception.h"
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
 * @date November 2000
 * @author Hj. Malthaner
 */

template<class T>
class slist_tpl
{
private:
    struct node_t
    {
public:
	node_t *next;
	T data;
    };

    node_t * head;
    node_t * tail;
    unsigned int node_count;

    friend class slist_iterator_tpl<T>;

	node_t * gimme_node()
	{
		return (node_t *)freelist_t::gimme_node(sizeof(node_t));
	}


	void putback_node(node_t *tmp)
	{
		freelist_t::putback_node(sizeof(node_t),tmp);
	}


	void putback_nodes()
	{
		freelist_t::putback_nodes(sizeof(node_t),head);
	}

public:

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


    /**
     * The destructor. Just calls clear()
     *
     * @author Hj. Malthaner
     */
    ~slist_tpl()
    {
        clear();
    }


    /**
     * Inserts an element at the beginning of the list.
     *
     * @author Hj. Malthaner
     */
	void insert(T data)
	{
		node_t *tmp = gimme_node();

		// vorne einfuegen
		tmp->next = head;
		head = tmp;
		head->data = data;

		if(tail == NULL) {
			tail = tmp;
		}
		node_count++;
	}


    /**
     * Inserts an element at a specific position
     * - pos must be in the range 0..count().
     * @author V. Meyer
     */
    void insert(T data, unsigned int pos)
    {
		if(pos > node_count) {
			dbg->fatal("slist_tpl<T>::insert()","<%s> index %d is out of bounds (<%d)",typeid(T).name(),pos,count());
		}

		if(pos == 0) { // insert at front
			insert(data);
			return;
		}
		node_t *p = head;
		while(--pos) {
			p = p->next;
		}
		// insert between p and p->next
		node_t *tmp = gimme_node();
		tmp->data = data;
		tmp->next = p->next;
		p->next = tmp;

		if(tail == p) {
			tail = tmp;
		}
		node_count++;
    }

    /**
     * Appends an element to the end of the list.
     *
     * @author Hj. Malthaner
     */
    void append(T data)
    {
		if(tail == 0) {
			insert(data);
		}
		else {
			node_t *tmp = gimme_node();
			tmp->data = data;
			tmp->next = 0;

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
		if(node_count == 0) {
			//MESSAGE("slist_tpl<T>::remove()", "data not in list!");
			return false;
		}

		if(head->data == data) {
			node_t *tmp = head->next;
			putback_node( head );
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
			putback_node( p->next );
			p->next = tmp;

			if(tmp == 0) {
				tail = p;
			}
		}
		node_count--;
		return true;
    }

    /**
     * Removes an specific element from the list
     *
     * @author Hj. Malthaner
     */
    void remove_at(unsigned int pos)
    {
		if(pos >= node_count) {
			dbg->fatal("slist_tpl<T>::remove_at()","<%s> index %d is out of bounds",typeid(T).name(),pos);
		}

		if(pos == 0) {
			// remove first element
			node_t *tmp = head->next;
			putback_node( head );
			head = tmp;

			if(head == 0) {
				tail = 0;
			}
		}
		else {
			node_t *p = head;
			while(--pos) {
				p = p->next;
			}

			// remove p->next
			node_t *tmp = p->next->next;
			putback_node( p->next );
			p->next = tmp;
			if(tmp == 0) {
				tail = p;
			}
		}
		node_count--;
    }

    /**
     * Retrieves the first element from the list. This element is
     * deleted from the list. Useful for some queueing tasks.
     * @author Hj. Malthaner
     */
	T remove_first()
	{
		if(node_count==0) {
			dbg->fatal("slist_tpl<T>::remove_first()","List of <%s> is empty",typeid(T).name());
		}

		T tmp = head->data;
		node_t *p = head;

		head = head->next;
		putback_node(p);

		node_count--;

		if(head == 0) {
			// list is empty now
			tail = 0;
		}

		return tmp;
	}


    /**
     * Recycles all nodes. Doesn't delete the objects.
     * Leaves the list empty.
     * @author Hj. Malthaner
     */
    void clear()
    {
		if(head) {
			putback_nodes();
		}
		head = 0;
		tail = 0;
		node_count = 0;
    }

    unsigned int count() const
    {
	return node_count;
    }

    bool is_empty() const
    {
	return head == 0;
    }

    T &at(unsigned int pos) const
    {
		node_t *p = head;

		while(p != 0) {
			if(pos-- == 0) {
				return p->data;
			}
			p = p->next;
		}

		dbg->fatal("slist_tpl<T>::at()","<%s> index %d is out of bounds",typeid(T).name(),pos);
		return head->data;	// to keep compiler silent
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


//template <class T> nodelist_t * slist_tpl<T>::freelist = 0;



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
	slist_iterator_tpl(const slist_tpl<T> *list)
	{
		current_node = &lead;
		lead.next = list->head;
	}

	slist_iterator_tpl(const slist_tpl<T> &list)
	{
		current_node = &lead;
		lead.next = list.head;
	}

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
			dbg->error("class slist_iterator_tpl.get_current()","Iteration: accesed lead!");
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
			dbg->error("class slist_iterator_tpl.get_current()","Iteration: accesed lead!");
		}
		return current_node->data;
	}
};

#endif
