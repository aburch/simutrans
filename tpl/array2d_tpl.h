/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef tpl_array2d_tpl_h
#define tpl_array2d_tpl_h

#include "../dataobj/koord.h"
#include "../simdebug.h"

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
	T* data;
	unsigned w, h;

public:

	array2d_tpl(unsigned _w, unsigned _h) : w(_w), h(_h) {
		data = new T[w*h];
	}

	array2d_tpl(const array2d_tpl <T> &other) {
		w = other.w;
		h = other.h;
		data = new T[w*h];
		memcpy(data, other.data, sizeof(T)*w*h);
	}

	~array2d_tpl() {
		delete [] data;
	}

	unsigned get_width() const {
		return w;
	}

	unsigned get_height() const {
		return h;
	}

	void init( T value ) {
		if(sizeof(T)==1) {
			memset( data, w*h, value );
		}
		else {
			unsigned i=(w*h)+1;
			while(  i>0  ) {
				data[--i] = value;
			}
		}
	}

	T& at(unsigned x, unsigned y) {
		if(x<w  &&  y<h) {
			return data[y*w + x];
		}
		else {
			dbg->fatal("array2d_tpl<T>::at()","index out of bounds: (%d,%d) not in (0..%d, 0..%d)", x, y, w-1, h-1);
			return data[0];//dummy
		}
	}

	T& at(koord k) {
		return at((unsigned)k.x, (unsigned)k.y);
	}

	/*
	 * use this with care, you'll lose all checks!
	 */
	const T* to_array() const {
		return data;
	}

	array2d_tpl<T> & operator = (const array2d_tpl <T> &other) {
		if(  h != other.h  &&  w != other.w  ) {
			if(  h*w!=0  ) {
				dbg->error("array2d_tpl<T>::=()","source has different size!");
			}
		}
		delete [] data;
		w = other.w;
		h = other.h;
		data = new T[w*h];
		memcpy(data, other.data, sizeof(T)*w*h);
	}
};


#endif
