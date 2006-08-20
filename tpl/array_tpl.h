/*
 * array_tpl.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef tpl_array_tpl_h
#define tpl_array_tpl_h

#include <typeinfo>
#include "debug_helper.h"

/**
 * A template class for bounds checked 1-dimesnional arrays.
 * This is kept as simple as possible. Does not use exceptions
 * for error handling.
 *
 * @author Hj. Malthaner
 */

template <class T>
class array_tpl
{
private:

    /**
     * if someone tries to make an out of bounds access to this
     * array, he'll get a reference to dummy. This is save, because
     * no one can expect that values read from or written to an
     * a out of bounds index are stored in the array
     *
     * @author Hj. Malthaner
     */
    T dummy;

    T* data;
    unsigned int size;

public:

    array_tpl(unsigned int size) {
	this->size = size;
	data = new T[size];
    }


    array_tpl(unsigned int size, T * arr) {
	this->size = size;
	data = new T[size];

	for(int i=0; i<size; i++) {
	    data[i] = arr[i];
	}
    }


    ~array_tpl() {
	delete [] data;
    }


    unsigned int get_size() const {
	return size;
    }


    T& at(const unsigned int i) {
	if(i<size) {
	    return data[i];
	} else {
	    ERROR("array_tpl<T>::at()",
	          "index out of bounds: %d not in 0..%d, T=%s ",
		  i, size-1, typeid(T).name());

	    return dummy;
	}
    }


    const T& get(const unsigned int i) const {
	if(i<size) {
	    return data[i];
	} else {
	    ERROR("array_tpl<T>::get()",
	          "index out of bounds: %d not in 0..%d, T=%s ",
		  i, size-1, typeid(T).name());

	    return dummy;
	}
    }
};


#endif
