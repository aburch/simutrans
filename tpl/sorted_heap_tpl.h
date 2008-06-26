/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef tpl_sorted_heap_tpl_h
#define tpl_sorted_heap_tpl_h

#include <stdlib.h>

#include "debug_helper.h"


/**
 * my try on a sorted heap template
 * the insert point is by binary search and then the array is just shifted with memmove
 *
 * @date March 2006
 * @author prissi
 */

#include "../simtypes.h"

template <class T>
class sorted_heap_tpl
{
private:
	T *nodes;

public:
	uint32 node_count;
	uint32 node_size;

	sorted_heap_tpl()
	{
		DBG_MESSAGE("sorted_heap_tpl()","initialized");
		nodes = MALLOCN(T, 4096);
		node_size = 4096;
		node_count = 0;
	}


	~sorted_heap_tpl()
	{
		free( nodes );
	}


	/**
	* Inserts an element into the queue.
	*
	* @author Hj. Malthaner
	*/
	void insert(const T item)
	{
		uint32 index = node_count;
		// insert with binary search, ignore doublettes
		if(node_count>0  &&  *(nodes[node_count-1])<=*item  &&  !(*(nodes[node_count-1])==*item)) {
			sint32 high = node_count, low = -1, probe;
			while(high - low>1) {
				probe = ((uint32) (low + high)) >> 1;
				if(*(nodes[probe])==*item) {
					low = probe;
					break;
				}
				else if(*item<=*(nodes[probe])) {
					low = probe;
				}
				else {
					high = probe;
				}
			}
			// we want to insert before, so we may add 1
			index = 0;
			if(low>=0) {
				index = low;
				if(*item<=*(nodes[index])) {
					index ++;
				}
			}
//DBG_MESSAGE("bsort","current=%i f=%i  f+1=%i",new_f,nodes[index]->f,nodes[index+1]->f);
		}

		// need to enlarge?
		if(node_count==node_size) {
			T *tmp=nodes;
			node_size += 4096;
			nodes = MALLOCN(T, node_size);
			memcpy( nodes, tmp, sizeof(T)*(node_size-4096) );
			free( tmp );
		}

		if(index<node_count) {
			// was not best f so far => insert
			memmove( nodes+index+1ul, nodes+index, sizeof(T)*(node_count-index) );
		}
		nodes[index] = item;
		node_count ++;
	}

	/**
	* Checks if the given element is already contained in the queue.
	*
	* @author Hj. Malthaner
	*/
	bool contains(const T data) const
	{
		// the fact that we are sorted does not help here ...
		assert(0);
	}


	/**
	* Retrieves the first element from the list. This element is
	* deleted from the list. Useful for some queueing tasks.
	* @author Hj. Malthaner
	*/
	T pop() {
		assert(node_count>0);
		node_count--;
		return nodes[node_count];
	}


	/**
	* Recycles all nodes. Doesn't delete the objects.
	* Leaves the list empty.
	* @author Hj. Malthaner
	*/
	void clear()
	{
		node_count = 0;
	}



	int count() const
	{
		return node_count;
	}


	bool empty() const { return node_count == 0; }
};

#endif
