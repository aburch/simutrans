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
	bool   need_resort;

	HOT_queue_tpl() :
		node_count(0),
		node_top(node_size),
		need_resort(false)
	{
		DBG_MESSAGE("HOT_queue_tpl()","initialized");
	}


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

			// first: close the old one ...
			node_count += heap.get_count();
			while (!heap.empty()) {
				nodes[node_top].append(heap.pop());
			}

			// now the heap is empty, so we put the first in
			heap.insert(item);
			node_top = d;
			need_resort = false;
		}
		else if(node_top>d) {
			// higher pocket => append unsorted ...
			nodes[d].append(item);
			node_count ++;
		}
		else {
			// ok, touching a possibly unsorted pocket
			if(need_resort) {
				// need heap'ifying ...
				bool heap_empty = heap.empty();
				assert(heap_empty);
				node_count -= nodes[node_top].get_count();
				while (!nodes[node_top].empty()) {
					heap.insert( nodes[node_top].remove_first() );
				}
				need_resort = false;
			}
			// at the top we have the heap ...
			heap.insert(item);
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
				if(iter.get_current()->is_matching(*data)) {
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
		T tmp = heap.pop();
		// if this pocket is empty, we must go to the next
		if (heap.empty()) {
			while (nodes[node_top].empty() && node_top < node_size) {
				node_top ++;
				need_resort = true;
			}
		}
		return tmp;
	}



	// since we use this static, we do not free the list array
	void clear()
	{
		heap.clear();
		while(node_top<node_size) {
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
			full_count += nodes[i].get_count();
		}
		assert(full_count==node_count);
		return full_count+heap.get_count();
	}



	// the HOTqueue is empty, if the last pocket is empty
	// this is always the heap ...
	bool empty() const { return  node_top == node_size; }
};

#endif
