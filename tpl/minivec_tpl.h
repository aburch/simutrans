/*
 * minivec_tpl.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef tpl_minivec_h
#define tpl_minivec_h

#include <typeinfo>

#include "no_such_element_exception.h"

/**
 * A template class for a simple vector type.
 * It can only store pointer types.
 * It is optimized to use as little memory as possible. It can only
 * hold up to 255 elements. Beware, this vector never shrinks.
 *
 * @date 24-Nov 2001
 * @author Hansjörg Malthaner
 */
template<class T> class minivec_tpl
{
private:

    /**
     * Pointer to our data
     * @author Hj. Malthaner
     */
    T* data;



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
     * Constructs an empty minivec with initial_capacity elements.
     *
     * @param initial_capacity initial capacity
     * @author Hj. Malthaner
     */
    minivec_tpl(int initial_capacity) {
	data = new T[initial_capacity];
	size = 0;
	capacity = initial_capacity;
    };


    ~minivec_tpl() {
      delete [] data;
      data = 0;            // paranoia - usually not needed
      size = 0;            // paranoia - usually not needed
      capacity = 0;        // paranoia - usually not needed
    }

    /**
     * Access element i. Throw no_such_element_exception if the
     * element does not exist.
     * @author Hj. Malthaner
     */
    T & at(unsigned int i) {
	if(i<size) {
	    return data[i];
	} else {
	    throw new no_such_element_exception("minivec_tpl<T>::at()",
						typeid(T).name(),
						i,
						"Out of bounds");
	}
    };


    /**
     * Read element i. Throw no_such_element_exception if the
     * element does not exist.
     * @author Hj. Malthaner
     */
    const T & get(unsigned int i) const {
	if(i<size) {
	    return data[i];
	} else {
	    throw new no_such_element_exception("minivec_tpl<T>::get()",
						typeid(T).name(),
						i,
						"Out of bounds");
	}
    };


    /**
     * Appends an element
     * @author Hj. Malthaner
     */
    void append(T v) {
	if(size < capacity) {
	    // printf("minivec_tpl::append(): Using unused element\n");
	    data[size++] = v;
	} else if(capacity < 255) {
	    // printf("minivec_tpl::append(): Growing, old cap %d, old size %d\n", capacity, size);
	    T* old = data;
	    data = new T[capacity+1];

	    for(int i=0; i<capacity; i++) {
		data[i] = old[i];
	    }
	    data[capacity++] = v;
	    size++;
	    delete [] old;
	    // printf("minivec_tpl::append(): Growing, new cap %d, new size %d\n", capacity, size);
	} else {
	  throw no_such_element_exception("minivec_tpl<T>::append()",
					  typeid(T).name(),
					  256,
					  "Capacity exceeded");
	}
    };


    /**
     * insert an element at a given position
     * @author V. Meyer
     */
    void insert(int pos, T v) {
	if(pos > size) {	// emergency
	    pos = size;
	}
	// first increase array
	if(size < capacity) {
	    for(int i=pos; i<size; i++) {
		data[i+1] = data[i];
	    }
	    data[pos] = v;
	    size++;
	} else {
	    T* old = data;
	    data = new T[capacity+1];
	    int i;

	    for(i=0; i<pos; i++) {
		data[i] = old[i];
	    }
	    data[i] = v;
	    for(; i<capacity; i++) {
		data[i+1] = old[i];
	    }
	    capacity++;
	    size++;
	    delete [] old;
	}
    };


    /**
     * Removes an element - preserves sort order
     * @author Hj. Malthaner
     */
    bool remove(T v) {
	for(int i=0; i<size; i++) {
	    if(data[i] == v) {
                for(int j=i; j<size-1; j++) {
		    data[j] = data[j+1];
		}
		size --;
		data[size] = 0;
		return true;
	    }
	}
	return false;
    };


    void clear() {
      size = 0;
    }


    /**
     * Returns the number of elements in this vector
     * @author Hj. Malthaner
     */
    unsigned char count() const {
	return size;
    };
};

#endif
