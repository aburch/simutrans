/*
 * array2d_tpl.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef tpl_array2d_tpl_h
#define tpl_array2d_tpl_h

#include "../dataobj/koord.h"
#include "debug_helper.h"

/**
 * A template class for bounds checked 2-dimesnional arrays.
 * This is kept as simple as possible. Does not use exceptions
 * for error handling.
 *
 * @author Hj. Malthaner
 * @see array_tpl
 */

template <class T>
class array2d_tpl
{
private:

    /**
     * if someone tries to make an out of bounds access to this
     * array, he'll get a reference to dummy. This is save, because
     * no one can expect that values read from or written to an
     * a out of bounds index are stored in the array
     */
    T dummy;

    T* data;
    unsigned int w, h;

public:

    array2d_tpl(unsigned int w, unsigned int h) {
	this->w = w;
	this->h = h;

	data = new T[w*h];
    }

    ~array2d_tpl() {
	delete [] data;
    }

    unsigned int get_width() const {
	return w;
    }

    unsigned int get_height() const {
	return h;
    }

    T& at(unsigned int x, unsigned int y) {
	if(x<w && y<h) {
	    return data[y*w + x];
	} else {
	    ERROR("array2d_tpl<T>::at()",
	          "index out of bounds: (%d,%d) not in (0..%d, 0..%d)", x, y, w-1, h-1);
		trap();
	    return dummy;
	}
    }

    T& at(koord k) {
	return at((unsigned int)k.x, (unsigned int)k.y);
    }

    /*
     * use this with care, you'll lose all checks!
     */
    const T* to_array() const {
	return data;
    }


    void copy_from(const array2d_tpl <T> &other) {
	if(h == other.h && w == other.w) {
	    memcpy(data, other.data, sizeof(T)*w*h);
	} else {
	    ERROR("array2d_tpl<T>::copy_from()",
	          "source has different size!");
	}
    }

};

#endif
