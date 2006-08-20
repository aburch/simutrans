/*
 * vector_tpl.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef tpl_vector_h
#define tpl_vector_h

/**
 * A template class for a simple vector type.
 * random access is allowed ...
 *
 * @date November 2000
 * @author Hj. Malthaner
 */

#include <stdlib.h>

#include "../simtypes.h"
#include "../simdebug.h"

/**
 * A template class for a simple vector type.
 * @date November 2000
 * @author Hj. Malthaner
 *
 * extended with function store_at() for random acess storage by prissi
 * BEWARE: using this function will create default objects, depending of the type of the vector
 */

template<class T>class vector_tpl
{
protected:

	T * data;

	/**
	 * Capacity.
	 * @author Hj. Malthaner
	 */
	uint32 size;


	/**
	 * Number of elements in vector.
	 * @author Hj. Malthaner
	 */
	uint32 count;

public:

	/**
	 * Construct a vector for size elements.
	 * @param size The capacity of the new vector
	 * @author Hj. Malthaner
	 */
	vector_tpl(uint32 size)
	{
		this->size  = size;
		if(size>0) {
			data = new T[size];
		} else {
			data = 0;
		}
		count = 0;
	}


	/**
	 * Destructor.
	 * @author Hj. Malthaner
	 */
	~vector_tpl()
	{
		if(data) {
			delete [] data;
		}
	}

	/**
	 * sets the vector to empty
	 * @author Hj. Malthaner
	 */
	void clear()
	{
		count = 0;
	}

	/**
	 * Resizes the maximum data that can be hold by this vector.
	 * Existing entries are preserved, new_size must be big enough
	 * to hold them.
	 *
	 * @author prissi
	 */
	bool resize(uint32 new_size)
	{
		if(new_size<=size) {
			return true;	// do nothing
		}
		//DBG_DEBUG("<vector_tpl>::resize()","old size %i, new size %i",size,new_size);
		if(count > new_size) {
			dbg->fatal("vector_tpl<T>::resize()", "cannot preserve elements.");
			return false;
		}
		T *new_data = new T[new_size];

		if(size>0  &&  data) {
			for(uint32 i = 0; i < count; i++) {
				new_data[i] = data[i];
			}
			delete [] data;
		}
		size  = new_size;
		data = new_data;
		return true;
	}

	/**
	 * Checks if element elem is contained in vector.
	 * Uses the == operator for comparison.
	 * @author Hj. Malthaner
	 */
	bool is_contained(T elem) const
	{
		for(uint32 i=0; i<count; i++) {
			if(data[i] == elem) {
				return true;
			}
		}
		return false;
	}


	/**
	 * Appends the element at the end of the vector.
	 * @author Hj. Malthaner
	 */
	bool append(T elem)
	{
		if(count >= size) {
			dbg->fatal("vector_tpl<T>::append()","capacity %i exceeded.",size);
			return false;
		}
		data[count ++] = elem;
		return true;
	}

	/**
	 * Appends the element at the end of the vector.
	 * of out of spce, extend with this factor
	 * @author prissi
	 */
	bool append(T elem,uint32 extend)
	{
		if(count>=size) {
			if(!resize(count+extend)) {
				dbg->fatal("vector_tpl<T>::append(,)","could not extend capacity %i.",size);
				return false;
			}
		}
		data[count ++] = elem;
		return true;
	}

	/**
	 * Checks if element is contained. Appends only new elements.
	 *
	 * @author Hj. Malthaner
	 */
	bool append_unique(T elem)
	{
		if(!is_contained(elem)) {
			return append(elem);
		}
		return false;
	}

	/**
	 * Checks if element is contained. Appends only new elements.
	 * extend vector if nessesary
	 * @author Hj. Malthaner
	 */
	bool append_unique(T elem,uint32 extend)
	{
		if(!is_contained(elem)) {
			return append(elem,extend);
		}
		return false;
	}


	/**
	* removes element, if contained
	* @author prisse
	*/
	bool remove(T elem)
	{
		uint32 i,j;
		for(i=j=0;  i<count;  i++,j++  ) {
			if(data[i] == elem) {
				// skip this one
				j++;
				count --;
			}
			// maybe we copy too often ...
			if(j<size) {
				data[i] = data[j];
			}
		}
		return true;
	}


	/**
	* insets data at a certain pos
	* @author prissi
	*/
	bool insert_at(uint32 pos,T elem)
	{
		if(  pos<count  ) {
			if(  count<size  ||  resize(count+1)) {
				// ok, a valid position, make space
				const long num_elements = (count-pos)*sizeof(T);
				memmove( data+pos+1, data+pos, num_elements );
				data[pos] = elem;
				count ++;
				return true;
			}
			else {
				dbg->fatal("vector_tpl<T>::insert_at()","cannot insert at %i! Only %i elements.", pos, count);
				return false;
			}
		}
		else {
			return append(elem,1);
		}
	}

	/**
	* put the data at a certain position
	* use with care, may lead to unitialized entries
	* @author prissi
	*/
	bool store_at(uint32 pos,T elem)
	{
		if(pos>=size) {
			resize((pos&0xFFFFFFF7)+8);
		}
		// ok, a valid position, make space
		data[pos] = elem;
		if(pos>count) {
			count = pos+1;
		}
		return true;
	}

	/**
	* removes element at position
	* @author prissi
	*/
	bool remove_at(uint32 pos)
	{
		if(pos<size  &&  pos<count) {
			uint32 i,j;
			for(i=pos, j=pos+1;  j<count;  i++,j++  ) {
				data[i] = data[j];
			}
			count --;
			return true;
		}
		return false;
	}


	/**
	 * Gets the element at position i
	 * @author Hj. Malthaner
	 */
	const T& get(uint32 i) const
	{
		if(i>=count) {
			dbg->fatal("vector_tpl<T>::get()","index out of bounds: %i not in 0..%d", i, count-1);
			// will never go here
		}
		return data[i];
	}

	/**
	 * Accesses the element at position i
	 * @author Hj. Malthaner
	 */
	T& at(uint32 i) const
	{
		if(i>=count) {
			dbg->fatal("vector_tpl<T>::at()","index out of bounds: %i not in 0..%d\n", i, count-1);
			// will never go here
		}
		return data[i];
	}


	/**
	* Gets the number of elements in the vector.
	* @author Hj. Malthaner
	*/
	uint32 get_count() const {return count;}


	/**
	* Gets the capacity.
	* @author Hj. Malthaner
	*/
	uint32 get_size() const {return size;}

} GCC_PACKED;

#endif
