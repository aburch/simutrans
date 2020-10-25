/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TPL_ARRAY_TPL_H
#define TPL_ARRAY_TPL_H


#include <typeinfo>
#include "../simdebug.h"
#include "../simtypes.h"

/**
 * A template class for bounds checked 1-dimensional arrays.
 * This is kept as simple as possible. Does not use exceptions
 * for error handling.
 */
template<class T> class array_tpl
{
public:
	typedef const T* const_iterator;
	typedef       T* iterator;

	typedef uint32 index;

	explicit array_tpl() : data(NULL), size(0) {}

	explicit array_tpl(index s) : data(new T[s]), size(s) {}

	explicit array_tpl(index s, const T& value) : data(new T[s]), size(s)
	{
		for (index i = 0; i < size; i++) {
			data[i] = value;
		}
	}

	~array_tpl() { delete [] data; }

	index get_count() const { return size; }

	bool empty() const { return size == 0; }

	void clear()
	{
		delete [] data;
		data = 0;
		size = 0;
	}

	void resize(index resize)
	{
		if (size < resize) {
			// extend if needed
			T* new_data = new T[resize];
			for (index i = 0;  i < size; i++) {
				new_data[i] = data[i];
			}
			delete [] data;
			data = new_data;
		}
		size = resize;
	}

	void resize(index resize, const T& value)
	{
		if (size < resize) {
			T* new_data = new T[resize];
			index i;
			for (i = 0;  i < size; i++) {
				new_data[i] = data[i];
			}
			for (; i < resize; i++) {
				new_data[i] = value;
			}
			delete [] data;
			data = new_data;
			size = resize;
		}
	}

	T& operator [](index i)
	{
		if (i >= size) {
			dbg->fatal("array_tpl<T>::[]", "index out of bounds: %d not in 0..%d, T=%s", i, size - 1, typeid(T).name());
		}
		return data[i];
	}

	const T& operator [](index i) const
	{
		if (i >= size) {
			dbg->fatal("array_tpl<T>::[]", "index out of bounds: %d not in 0..%d, T=%s", i, size - 1, typeid(T).name());
		}
		return data[i];
	}

	iterator begin() { return data; }
	iterator end()   { return data + size; }

	const_iterator begin() const { return data; }
	const_iterator end()   const { return data + size; }

private:
	array_tpl(const array_tpl&);
	array_tpl& operator=( array_tpl const& other );

	T* data;
	index size;
};

#endif
