/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef tpl_prioqueue_tpl_h
#define tpl_prioqueue_tpl_h

#include "../dataobj/freelist.h"


// collect statistics
// #define PRIQ_STATS


template<class T> class slist_iterator_tpl;

/**
 * <p>A template class for a priority queue. The queue is built upon a
 * sorted linked list.
 * Maintains a list of free nodes to reduce calls to new and delete.</p>
 *
 * <p>This template only works with pointer types, because it compares
 * data by using *p1 <= *p2. The operator <= must be defined for the type.</p>
 *
 * <p>The insert() operation works in O(n), in average n/2 steps</p>
 * <p>The pop() operation works in O(1) steps</p>
 * <p>The contains() operation works in O(n), average n/2 steps</p>
 * <p>The remove() operation works in O(n), average n steps</p>
 * <p>The get_count() and empty() operation work in O(1) steps</p>
 *
 * @date November 2001
 * @author Hj. Malthaner
 */

template <class T>
class prioqueue_tpl
{
	private:
		struct node_t
		{
			void* operator new(size_t) { return freelist_t::gimme_node(sizeof(node_t)); }
			void operator delete(void* p) { freelist_t::putback_node(sizeof(node_t), p); }

			node_t *next;
			T data;
		};

		node_t * head;
		node_t * tail;

		int node_count;

#ifdef PRIQ_STATS
		int insert_hops;
		int insert_calls;
		int insert_count;
#endif

	public:

		prioqueue_tpl()
		{
			head = 0;             // empty queue
			tail = 0;
			node_count = 0;

#ifdef PRIQ_STATS
			insert_hops = 0;
			insert_calls = 0;
			insert_count = 0;
#endif

		}

		~prioqueue_tpl()
		{
			clear();

#ifdef PRIQ_STATS
			printf("%d insert calls, total %d hops, total count %d\n",
					insert_calls, insert_hops, insert_count);
#endif
		}


		/**
		 * Inserts an element into the queue.
		 *
		 * @author Hj. Malthaner
		 */
		void insert(const T data)
		{
			node_t* tmp = new node_t();
			node_t *prev = 0;
			node_t *curr = head;

#ifdef PRIQ_STATS
			insert_calls ++;
			insert_count += node_count;
#endif

			tmp->data = data;
			tmp->next = 0;

			// empty list ?

			if (head == 0) {

				head = tail = tmp;

				// check if we may append or do we need to search?
			} else if (tail && *tail->data <= *data) {

				// we may append
				tail->next = tmp;
				tail = tmp;

				// printf("Shortcut at %d elements\n", node_count);

			} else {
				// search position

				do {
					if (curr == 0) {

						// end of list
						prev->next = tmp;
						tail = tmp;

					} else if (*data <= *curr->data) {

						tmp->next = curr;

						if (prev == 0) {
							// smallest element case
							head = tmp;
						} else {
							// middle element
							prev->next = tmp;
						}

						break;
					}

					prev = curr;
					curr = curr->next;

#ifdef PRIQ_STATS
					insert_hops ++;
#endif

				} while (true);
			}
			node_count ++;
		}


		/**
		 * Checks if the given element is already contained in the queue.
		 *
		 * @author Hj. Malthaner
		 */
		bool contains(const T data) const
		{
			node_t *p = head;

			while (p != 0 && p->data != data) {
				p = p->next;
			}

			return (p != 0);         // ist 0 wenn nicht gefunden
		}


		/**
		 * Retrieves the first element from the list. This element is
		 * deleted from the list. Useful for some queueing tasks.
		 * @author Hj. Malthaner
		 */
		T& pop() {
			if (head) {
				T& tmp = head->data;
				node_t *p = head;

				head = head->next;
				delete p;

				node_count --;

				// list got empty?
				if (head == 0) {
					tail = 0;
				}

				return tmp;
			} else {
				dbg->fatal("prioqueue_tpl<T>::pop()","called on empty queue!");
			}
		}


		/**
		 * Recycles all nodes. Doesn't delete the objects.
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

		int count() const
		{
			return node_count;
		}


		bool empty() const { return head == 0; }
};

#endif
