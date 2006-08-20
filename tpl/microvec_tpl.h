/*
 * microvec_tpl.h
 *
 * Copyright (c) 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef tpl_microvec_h
#define tpl_microvec_h

#include <typeinfo>
#include <assert.h>

#include "no_such_element_exception.h"
#include "../simdebug.h"

/**
 * A template class for a simple vector type.
 * It can only store pointer types.
 * It is optimized to use as little memory as possible. It can only
 * hold up to 255 elements. Beware, this vector never shrinks.
 *
 * It uses a hack to improve efficiency of storage of one element only
 * (doesn't use heap, but data pointer itself)
 *
 * @date 16-Mar-2003
 * @author Hansjörg Malthaner
 *
 * I resolved the hack by using a union for data. Code result should
 * be the same.
 * @author: Volker Meyer
 * @date: 21.05.2003
 *
 */
template<class T> class microvec_tpl
{
private:

    /**
     * Pointer to our data or data itself is size == 1
     * @author Hj. Malthaner
     */
    union {
	T* some;    // valid if capacity > 1
	T one;      // valid if capacity == 1
    } data;


    /**
     * Current size (number of elements)
     * @author Hj. Malthaner
     */
    unsigned char size;


    /**
     * Capacity.
     * @author Hj. Malthaner
     */
    unsigned char capacity;


public:

    /**
     * Constructs an empty minivec with MAX(initial_capacity, 1) elements.
     *
     * @param initial_capacity initial capacity
     * @author Hj. Malthaner
     */
    microvec_tpl(int initial_capacity)
    {
		assert( sizeof(T)<=4 );
		size = 0;
		capacity = initial_capacity > 1 ? initial_capacity : 1;

		if(capacity > 1) {
			data.some = new T[capacity];
		}
    };


    ~microvec_tpl()
    {
		size = 0;            // paranoia - usually not needed
		if(capacity > 1) {
			delete [] data.some;
		}
		data.some = 0;            // paranoia - usually not needed
		capacity = 1;        // paranoia - usually not needed
    }


    /**
     * Access element i. Throw no_such_element_exception if the
     * element does not exist.
     * @author Hj. Malthaner
     */
    T & at(unsigned int i)
    {
		if(i<size) {
			if(capacity == 1) {
				return data.one;
			} else {
				return data.some[i];
			}
		} else {
			throw new no_such_element_exception("microvec_tpl<T>::at()",
																		typeid(T).name(),
																		i,
																		"Out of bounds");
		}
    };


    /**
     * Read element i. Returns NULL is no such element exists.
     * @author Hj. Malthaner
     */
	T get(unsigned int i) const
	{
		if(i<size) {
			if(capacity == 1) {
				return data.one;
			} else {
				return data.some[i];
			}
		} else {
//			throw new no_such_element_exception("microvec_tpl<T>::at()",typeid(T).name(),i,"Out of bounds");
			return 0;
		}
	};


    /**
     * insert an element at a given position
     * @author V. Meyer
     */
	void insert(int pos, T v) {
		if(pos > size) {
			// emergency
			dbg->fatal("microvec_tpl<T>::insert()","type=%s, size exeeded: pos=%i size=%i",typeid(T).name(), pos,size);
			pos = size;
		}

		if(capacity == 1) {
			if(size == 0) {
				// use base element
				data.one = v;
				size = 1;
			} else {
				// growing non-heap vector;
				T old = data.one;

				capacity = 2;
				data.some = new T[capacity];

				if(pos == 0) {
					data.some[0] = v;
					data.some[1] = old;
				} else {
					data.some[0] = old;
					data.some[1] = v;
				}
				size = 2;
			}
		} else {
			if(capacity < 255) {
				// first increase array
				if(size < capacity) {
					for(int i=pos; i<size; i++) {
						data.some[i+1] = data.some[i];
					}
					data.some[pos] = v;
					size++;
				} else {
					T* old = data.some;
					data.some = new T[capacity+1];
					int i;
					for(i=0; i<pos; i++) {
						data.some[i] = old[i];
					}
					data.some[i] = v;
					for(; i<capacity; i++) {
						data.some[i+1] = old[i];
					}
					capacity++;
					size++;
					delete [] old;
				}
			} else {
				throw no_such_element_exception("microvec_tpl<T>::insert()",
																		typeid(T).name(),
																		pos,
																		"Capacity exceeded");
			}
		}
	};


    /**
     * Appends an element
     * @author Hj. Malthaner
     */
	void append(T v) {
		insert(size, v);
	};


    /**
     * Removes an element - preserves sort order
     * @author Hj. Malthaner
     */
    bool remove(T v) {
      if(capacity == 1) {
	if(size > 0 && data.one == v) {
	  data.one = 0;
	  size = 0;
	  return true;
	}
	return false;

      } else {

	for(int i=0; i<size; i++) {
	    if(data.some[i] == v) {
                for(int j=i; j<size-1; j++) {
		    data.some[j] = data.some[j+1];
		}
		size --;
		data.some[size] = 0;
		return true;
	    }
	}
	return false;
      }
    };


	void clear() {
		size = 0;
	}


    /**
     * Returns the number of elements in this vector
     * @author Hj. Malthaner
     */
	unsigned char get_count() const {
		return size;
	};
};

#endif
