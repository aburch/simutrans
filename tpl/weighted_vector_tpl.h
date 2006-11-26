/*
 * vector_tpl.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef tpl_weighted_vector_h
#define tpl_weighted_vector_h

#include <stdlib.h>

#include "../simdebug.h"

/**
 * A template class for a simple vector type.
 *
 * @date November 2000
 * @author Hj. Malthaner
 */

template<class T>
class weighted_vector_tpl
{
protected:

	// each elements has a size and a weight

	typedef struct {
		T data;
		unsigned long weight;
	} nodestruct;

	// all together
	nodestruct *nodes;

    /**
     * Capacity.
     * @author Hj. Malthaner
     */
    unsigned int size;


    /**
     * Number of elements in vector.
     * @author Hj. Malthaner
     */
    unsigned int count;

    /**
     * sum of all weights
     * @author prissi
     */
    unsigned long total_weight;

public:

    /**
     * Construct a vector for size elements.
     * @param size The capacity of the new vector
     * @author Hj. Malthaner
     */
	weighted_vector_tpl(unsigned int size)
	{
		this->size  = size;
		if(size>0) {
			nodes = new nodestruct[size];
			for( unsigned i=0; i<size; i++ ) {
				nodes[i].weight = 0;
			}
		} else {
			nodes = 0;
		}
		count = 0;
		total_weight = 0;
	}


    /**
     * Destructor.
     * @author Hj. Malthaner
     */
	~weighted_vector_tpl()
	{
		if(nodes) {
			delete [] nodes;
		}
	}

    /**
     * sets the vector to empty
     * @author Hj. Malthaner
     */
	void clear()
	{
		count = 0;
		total_weight = 0;
	}

    /**
     * Resizes the maximum data that can be hold by this vector.
     * Existing entries are preserved, new_size must be big enough
     * to hold them.
     *
     * @author prissi
     */
	bool resize(unsigned int new_size)
	{
		if(new_size<=size) {
			return true;	// do nothing
		}
		if(count > new_size) {
			dbg->fatal("weighted_vector_tpl<T>::resize()", "cannot preserve elements.");
			return false;
		}
		nodestruct *new_nodes = new nodestruct[new_size];

		if(size>0  &&  nodes) {
			for(unsigned int i = 0; i < count; i++) {
				new_nodes[i].data = nodes[i].data;
				new_nodes[i].weight =  nodes[i].weight;
			}
			delete [] nodes;
		}
		size  = new_size;
		nodes = new_nodes;
		return true;
	}

    /**
     * Checks if element elem is contained in vector.
     * Uses the == operator for comparison.
     * @author Hj. Malthaner
     */
    bool is_contained(T elem) const
    {
		for(unsigned int i=0; i<count; i++) {
			if(nodes[i].data == elem) {
				return true;
			}
		}
		return false;
    }


    /**
     * Appends the element at the end of the vector.
     * @author Hj. Malthaner
     */
	bool append(T elem,unsigned long weight)
	{
#ifdef IGNORE_ZERO_WEIGHT
		if(weight==0) {
			// ignore unused entries ...
			return false;
		}
#endif
		if(count >= size) {
			dbg->fatal("weighted_vector_tpl<T>::append()","capacity %i exceeded.",size);
			return false;
		}
		nodes[count].data = elem;
		nodes[count].weight = total_weight;
		count ++;
		total_weight += weight;
		return true;
	}

    /**
     * Appends the element at the end of the vector.
     * of out of spce, extend with this factor
     * @author prissi
     */
	bool append(T elem,unsigned long weight,unsigned extend)
	{
#ifdef IGNORE_ZERO_WEIGHT
		if(weight==0) {
			// ignore unused entries ...
			return false;
		}
#endif
		if(count>=size) {
			if(  !resize( count+extend )  ) {
				dbg->fatal("weighted_vector_tpl<T>::append(,)","could not extend capacity %i.",size);
				return false;
			}
		}
		nodes[count].data = elem;
		nodes[count].weight = total_weight;
		count ++;
		total_weight += weight;
		return true;
	}

    /**
     * Checks if element is contained. Appends only new elements.
     *
     * @author Hj. Malthaner
     */
	bool append_unique(T elem,unsigned long weight)
	{
		if(!is_contained(elem)) {
			return append(elem,weight);
		}
		return true;
	}

    /**
     * Checks if element is contained. Appends only new elements.
     * extend vector if nessesary
     * @author Hj. Malthaner
     */
	bool append_unique(T elem,unsigned long weight,unsigned extend)
	{
		if(!is_contained(elem)) {
			return append(elem,weight,extend);
		}
		return true;
	}


	/**
	* insets data at a certain pos
	* @author prissi
	*/
	bool insert_at(unsigned int pos,T elem,unsigned long weight)
	{
#ifdef IGNORE_ZERO_WEIGHT
		if(weight==0) {
			// ignore unused entries ...
			return false;
		}
#endif
		if(  pos<count  ) {
			if(  count<size  ||  resize(count+1)) {
				// ok, a valid position, make space
				const long num_elements = (count-pos)*sizeof(nodestruct);
				memmove( nodes+pos+1, nodes+pos, num_elements );
				nodes[pos].data = elem;
				for(unsigned i=pos+1;  i<=count;  i++ ) {
					nodes[i].weight += weight;
				}
				total_weight += weight;
				count ++;
				return true;
			}
			else {
				dbg->fatal("weighted_vector_tpl<T>::insert_at()","cannot insert at %i! Only %i elements.", pos, count);
				return false;
			}
		}
		else {
			return append(elem,weight,1);
		}
	}


	/**
	* removes element, if contained
	* @author prissi
	*/
	bool remove(T elem)
	{
		unsigned int i,j;
		unsigned long diff_weight=0;

		for(i=j=0;  i<count;  i++,j++  ) {
			if(nodes[i].data == elem) {
				// skip this one
				j++;
				// get the change in the weight; must check, if it isn't the last element
				diff_weight = (i+1<count) ? nodes[i+1].weight-nodes[i].weight : total_weight-nodes[i].weight;
				count --;
			}
			// maybe we copy too often ...
			if(j<size) {
				nodes[i].data = nodes[j].data;
				nodes[i].weight = nodes[j].weight-diff_weight;
			}
		}
		total_weight -= diff_weight;
		return true;
	}


	/**
	* removes element at position
	* @author prissi
	*/
	bool remove_at(unsigned int pos)
	{
		if(  pos<count  ) {
			unsigned int i,j;
			// get the change in the weight; must check, if it isn't the last element
			const unsigned long diff_weight = (pos+1<count) ? nodes[pos+1].weight-nodes[pos].weight : total_weight-nodes[pos].weight;
			for(i=pos, j=pos+1;  j<count;  i++,j++  ) {
				nodes[i].data = nodes[j].data;
				nodes[i].weight = nodes[j].weight - diff_weight;
			}
			count --;
			total_weight -= diff_weight;
			return true;
		}
		return false;
	}


    /**
     * Gets the element at position i
     * @author Hj. Malthaner
     */
	const T& get(unsigned int i) const
	{
		if(i<count) {
			return nodes[i].data;
		} else {
			dbg->fatal("weighted_vector_tpl<T>::get()","index out of bounds: %i not in 0..%d", i, count-1);
			return nodes[0].data;	// dummy
		}
	}

	/**
	* Accesses the element at position i
	* @author Hj. Malthaner
	*/
	T& at(unsigned int i) const
	{
		if(i<count) {
			return nodes[i].data;
		} else {
			dbg->fatal("weighted_vector_tpl<T>::at()","index out of bounds: %i not in 0..%d", i, count-1);
			return nodes[0].data;	// dummy
		}
	}

	/**
	* returns the weight at a position
	* @author prissi
	*/
	unsigned long weight_at(unsigned pos) const
	{
		return (pos<count) ? nodes[pos].weight : total_weight+1;
	}

	/**
	* Accesses the element at position i by weight
	* @author prissi
	*/
	T& at_weight(const unsigned long target_weight) const
	{
		if(target_weight<=total_weight) {
#if 0
			// that is the main idea (but slower, the more entries are in the list)
			unsigned pos;
			unsigned long current_weight=0;
			for(  pos=0;  pos<count-1  &&  current_weight+nodes[pos+1].weight<target_weight;  pos++) {
				current_weight += nodes[pos+1].weight;
			}
//DBG_DEBUG("at_weight()","target_weight=%i found_weight=%i at pos %i",target_weight,nodes[pos].weight,pos);
			return nodes[pos].data;
#else
			// ... and that the much faster binary search
			unsigned diff = 1;
			sint8 counter=0;
			// now make sure, diff is 2^n
			while(diff <= count) {
				diff <<= 1;
				counter ++;
			}
			diff >>= 1;

			unsigned pos = diff;

			// now search
			while(counter-->0) {
				diff = (diff+1)/2;
				if(  pos<count  &&  weight_at(pos)<=target_weight  ) {
					if(  weight_at(pos+1)>target_weight) {
						break;
					}
					pos += diff;
				}
				else {
					pos -= diff;
				}
			}
//DBG_DEBUG("at_weight()","target_weight=%i found_weight=%i at pos %i",target_weight,weight_at(pos),pos);
			return nodes[pos].data;
#endif
		} else {
			dbg->fatal("weighted_vector_tpl<T>::at_weight()","weight out of bounds: %i not in 0..%d", target_weight, total_weight);
			return nodes[0].data;	// dummy
		}
	}


    /**
     * Gets the number of elements in the vector.
     * @author Hj. Malthaner
     */
    unsigned int get_count() const { return count; }

    /**
     * Gets the capacity.
     * @author Hj. Malthaner
     */
    unsigned int get_size() const { return size; }

    /**
     * Gets the total weight
     * @author prissi
     */
    unsigned int get_sum_weight() const { return total_weight; }

		bool empty() const { return count == 0; }
};

#endif
