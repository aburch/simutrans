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
#include "../simdebug.h"
#include "../simtypes.h"

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
	T* data;
	uint16 size;

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

	uint16 get_size() const {
		return size;
	}

	void resize_size(uint16 resize) const {
		if(size<resize) {
			T *new_data = new T[size];
			for( uint16 i=0;  i<size;  i++ ) {
				new_data[i] = data[i];
			}
			delete [] data;
			data = new_data;
			size = resize;
		}
	}

	T& at(const uint16 i) {
		if(i>=size) {
			dbg->fatal("array_tpl<T>::at()", "index out of bounds: %d not in 0..%d, T=%s ", i, size-1, typeid(T).name());
		}
		return data[i];
	}

	const T& get(const uint16 i) {
		if(i>=size) {
			dbg->fatal("array_tpl<T>::get()", "index out of bounds: %d not in 0..%d, T=%s ", i, size-1, typeid(T).name());
		}
		return data[i];
	}
};

#endif
