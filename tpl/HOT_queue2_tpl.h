/*
 * Copyright (c) 2006 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef tpl_HOT_queue_tpl_h
#define tpl_HOT_queue_tpl_h


/* HOT queue
 * we use a pocket for each distance
 * thus we need the function get_distance
 * otherwise we can forget about sorting ...
 *
 * Only the best pocket is kept within a heap (I suggest using a binary heap)
 * Unfourtunately, most of the literature on binary heaps is really dry paper stuff.
 * However, in praxis they are not too difficult, this one is written following
 *    http://theory.stanford.edu/~amitp/GameProgramming/ImplementationNotes.html
 *    (scroll down a little)
 *
 * This version tries to minimize the heapify operations, which was partially successful.
 * Unfourtunately, binary heaps perform nearly as good as the HOT queue.
 */

#include "../simtypes.h"
#include "slist_tpl.h"
#include "binary_heap_tpl.h"

// we know the maximum manhattan distance, i cannot be larger than x_size+y_size
// since the largest map is 4096*4096 ...
template <typename T, uint32 node_size = 8192> class HOT_queue_tpl
{
private:
	slist_tpl<T> nodes[node_size];
	binary_heap_tpl<T> heap;	// only needed for one pocket;

public:
	uint32 node_count;
	uint32 node_top;
	uint32 heap_index;
	uint32 need_resort;

	HOT_queue_tpl() :
		node_count(0),
		node_top(node_size),
		heap_index(node_size),
		need_resort(0xFFFFFFFFUL)
	{
		DBG_MESSAGE("HOT_queue_tpl()","initialized");
	}


private:
	void heapify(uint32 pocket) {
		bool nodes_not_empty = !nodes[pocket].empty();
		assert(nodes_not_empty);

		// needs resort: NULL pointer in the first pocket
		if (nodes[pocket].front() == NULL) {
			nodes[pocket].remove_first();	// remove NULL
		}
		// first: close the old one ...
		node_count += heap.get_count();
		while (!heap.empty()) {
			nodes[heap_index].append(heap.pop());
		}
		// now insert everything into the heap
		node_count -= nodes[pocket].get_count();
		while (!nodes[pocket].empty()) {
			heap.insert( nodes[pocket].remove_first() );
		}
		heap_index = pocket;
	}


public:
	/**
	* Inserts an element into the queue.
	* since all pockets have the same f, we do not need top sort it
	*/
	void insert(const T item)
	{
		uint32 d=item->get_distance();
		assert(d<8192);

		if(d<node_top) {
			// open a new pocket (which is of course still empty)

			// first: close the old one (first is still the lowest!)
			node_count += heap.get_count();
			while (!heap.empty()) {
				nodes[heap_index].append(heap.pop());
			}
			heap_index = node_size;

			nodes[d].append(item);
			node_count ++;
			node_top = d;
		}
		else if(node_top>d) {
			// higher pocket => append unsorted ...
			// will also keep the NULL in the first pocket (resort mark) untouched
			nodes[d].append(item);
			node_count ++;
		}
		else {	// d==node_top
			// ok, touching a possibly unsorted pocket
			if (nodes[d].get_count() > 2 && nodes[d].front() == NULL) {
				heapify(d);
			}
			// now either heap or just normal insertion1
			if(heap_index==d) {
				// at the top we have the heap ...
				heap.insert(item);
			}
			else {
				// the first is always sorted
				node_count ++;
				if (!nodes[d].empty() && *nodes[d].front() <= *item) {
					nodes[d].append(item);
				}
				else {
					nodes[d].insert(item);
				}
			}
		}
	}



	// brute force search (not used in simutrans)
	bool contains(const T data) const
	{
		assert(0);
		uint32 d=data->get_distance();
		if(d<node_top) {
			return false;
		}
		else if(d==node_top) {
			heap.constains(data);
		}
		else {
			slist_iterator_tpl<T *>iter(nodes[d]);
			while(iter.next()) {
				if(iter.get_current()  &&  iter.get_current()->is_matching(*data)) {
					return true;
				}
			}
		}
		return false;
	}



	/**
	* Retrieves the first element from the list. This element is
	* deleted from the list.
	* If the list became emtpy, we must advance to the next no-empty list
	*/
	T pop() {
		T tmp;
		if(node_top==heap_index) {
			tmp = heap.pop();
			// if this pocket is empty, we must go to the next
			if (heap.empty()) {
				while (nodes[node_top].empty() && node_top < node_size) {
					node_top ++;
				}
			}
		}
		else {
			// first is always the lowest
			tmp = nodes[node_top].remove_first();
			// might need a resort ...
			if(tmp==NULL) {
				// yws resort
				heapify(node_top);
				return pop();
			}
			else {
				node_count --;
				// now there are some cases to handle
				switch(nodes[node_top].get_count()) {
					case 0:
						while (nodes[node_top].empty() && node_top < node_size) {
							node_top ++;
						}
						break;
					case 1:	// only one left: will be still the lowest
						break;
					case 2:
						// swap the two and be still sorted
						if (*nodes[node_top].front() <= *nodes[node_top].at(1)) {
							nodes[node_top].append( nodes[node_top].remove_first() );
						}
						break;
					default:
						// unsort marker
						nodes[node_top].insert(NULL);
						break;
				}
			}
		}
		return tmp;
	}



	// since we use this static, we do not free the list array
	void clear()
	{
		heap.clear();
		while(node_top<node_size) {
			if (!nodes[node_top].empty() && nodes[node_top].front() == NULL) {
				node_count ++;
			}
			node_count -= nodes[node_top].get_count();
			nodes[node_top].clear();
			node_top ++;
		}
		assert(node_count==0);
	}



	// we have to count all pockets
	// so only call this, if you really need it ...
	int get_count() const
	{
		uint32 full_count = 0;
		for(  uint32 i=node_top;  i<node_size;  i++  ) {
			if (!nodes[i].empty()) {
				full_count += nodes[i].get_count();
				if (nodes[i].front() == NULL) {
					full_count --;
				}
			}
		}
DBG_MESSAGE("HOT_queue_tpl::get_count()","expected %i found %i (%i in heap)",node_count,full_count,heap.get_count());
		assert(full_count==node_count);
		return full_count+heap.get_count();
	}



	// the HOTqueue is empty, if the last pocket is empty
	// this is always the heap ...
	bool empty() const
	{
		return node_top==node_size;
//		return nodes[node_top].empty();
//		return heap.empty();
	}
};

#endif
