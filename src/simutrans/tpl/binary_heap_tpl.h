/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TPL_BINARY_HEAP_TPL_H
#define TPL_BINARY_HEAP_TPL_H


#include "../simmem.h"
#include "../simtypes.h"


/**
 * my try on a binary heap template,
 * inspired by the pathfinder of OTTD written by kuDr
 *
 * For information about Binary Heap algorithm,
 *   see: https://en.wikipedia.org/wiki/Binary_heap
 */
template <class T>
class binary_heap_tpl
{
private:
	T *nodes;

public:
	uint32 node_count;
	uint32 node_size;

	binary_heap_tpl(uint32 size = 4096)
	{
		nodes = MALLOCN(T, size);
		node_size = size;
		node_count = 0;
	}


	~binary_heap_tpl()
	{
		free( nodes );
	}


	/**
	 * Inserts an element into the queue.
	 * in such a way that the lowest is at the top of this tree in an array
	 * parts inspired from OTTD
	 */
	void insert(const T item)
	{
		node_count ++;

		// need to enlarge? (must be 2^x)
		if(node_count==node_size) {
			T *tmp=nodes;
			node_size *= 2;
			nodes = MALLOCN(T, node_size);
			memcpy( nodes, tmp, sizeof(T)*(node_size/2) );
			free( tmp );
		}

		// now we have to move it to the right position
		int gap = node_count;
		for (int parent = gap/2; (parent>0) && (*item <= *nodes[parent]);  ) {
			nodes[gap] = nodes[parent];
			gap = parent;
			parent /= 2;
		}
		nodes[gap] = item;
	}


	/**
	 * unfortunately, the removing is as much effort as the insertion ...
	 */
	T pop() {
		assert(!empty());

		T result = nodes[1];

		// at index 1 is the lowest one
		uint32 gap = 1;

		// this is the last one
		T item = nodes[node_count--];

		// now we must maintain relation between parent and its children:
		//   parent <= any child
		// from head down to the tail
		uint32 child = 2; // first child is at [parent * 2]

		// while children are valid
		while(child<=node_count) {

			// choose the smaller child
			if(  child<node_count && *nodes[child+1] <= *nodes[child]) {
				child++;
			}

			if(!(*nodes[child] <= *item)) {
				// the smaller child is still bigger or same as parent => we are done
				break;
			}

			// if smaller child is smaller than parent, it will become new parent
			nodes[gap] = nodes[child];
			gap = child;
			// where do we have our new children?
			child = gap * 2;
		}
		// move last item to the proper place
		if (node_count>0) {
			nodes[gap] = item;
		}

		return result;
	}

	/**
	 * Recycles all nodes. Doesn't delete the objects.
	 * Leaves the list empty.
	 */
	void clear()
	{
		node_count = 0;
	}


	uint32 get_count() const
	{
		return node_count;
	}


	bool empty() const { return node_count == 0; }


	const T& front() {
		assert(!empty());

		return nodes[1];
	}

private:
	binary_heap_tpl(const binary_heap_tpl& other);
	binary_heap_tpl& operator=( binary_heap_tpl const& other );
};

#endif
