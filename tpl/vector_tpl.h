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

#include <stdlib.h>

#include "debug_helper.h"

/**
 * A template class for a simple vector type.
 *
 * @date November 2000
 * @author Hj. Malthaner
 */

template<class T>
class vector_tpl
{
private:

    T * data;

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

public:

    /**
     * Construct a vector for size elements.
     * @param size The capacity of the new vector
     * @author Hj. Malthaner
     */
    vector_tpl(unsigned int size)
    {
        this->size  = size;
        data = new T[size];
        count = 0;
    }


    /**
     * Destructor.
     * @author Hj. Malthaner
     */
    ~vector_tpl()
    {
        delete [] data;
    }

    /**
     * sets the vector to empty
     * @author Hj. Malthaner
     */
    void clear()
    {
	count = 0;
    }
#ifdef not_in_use   /* leave this here - maybe used later! */
    /**
     * Resizes the maximum data that can be hold by this vector.
     * Existing entries are preserved, new_size must be big enough
     * to hold them.
     *
     * @author Volker Meyer
     */
    bool resize(unsigned int new_size)
    {
	if(count > new_size) {
	    ERROR("vector_tpl<T>::resize()", "cannot preserve elements.");
	    return false;
	}
        T *new_data = new T[new_size];

	for(int i = 0; i < count; i++) {
	    new_data[i] = data[i];
	}
	size  = new_size;
	delete data;
	data = new_data;
	reutrn true;
    }
#endif
    /**
     * Checks if element elem is contained in vector.
     * Uses the == operator for comparison.
     * @author Hj. Malthaner
     */
    bool is_contained(T elem) const
    {
	for(unsigned int i=0; i<count; i++) {
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
	if(count < size) {
	    data[count ++] = elem;
	    return true;
	} else {
	    ERROR("vector_tpl<T>::append()",
	          "capacity exceeded.");
	    return false;
	}
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
	} else {
	    return true;
	}
    }


    /**
     * Gets the element at position i
     * @author Hj. Malthaner
     */
    const T& get(unsigned int i) const
    {
	if(i<count) {
	    return data[i];
	} else {
	    ERROR("vector_tpl<T>::get()",
	          "index out of bounds: %i not in 0..%d", i, count-1);
	    // return data[0];
	    abort();
	}
    }

    /**
     * Accesses the element at position i
     * @author Hj. Malthaner
     */
    T& at(unsigned int i) const
    {
	if(i<count) {
	    return data[i];
	} else {
	    ERROR("vector_tpl<T>::at()",
	          "index out of bounds: %i not in 0..%d\n", i, count-1);
	    // return data[0];
	    abort();
	}
    }


    /**
     * Gets the number of elements in the vector.
     * @author Hj. Malthaner
     */
    unsigned int get_count() const {return count;};


    /**
     * Gets the capacity.
     * @author Hj. Malthaner
     */
    unsigned int get_size() const {return size;};
};

#endif
