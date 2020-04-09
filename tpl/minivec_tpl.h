/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TPL_MINIVEC_TPL_H
#define TPL_MINIVEC_TPL_H


#include "../simdebug.h"
#include "../simtypes.h"


/** A template class for a simple vector type */
template<class T> class minivec_tpl
{
public:
	typedef const T* const_iterator;
	typedef       T* iterator;

	minivec_tpl() : data(NULL), size(0), count(0) {}

	/** Construct a vector for cap elements */
	explicit minivec_tpl(uint8 cap) : data(cap > 0 ? new T[cap] : NULL), size(cap), count(0) {}

	~minivec_tpl() { delete [] data; }

	/** sets the vector to empty */
	void clear() { count = 0; }

	/**
	 * Resizes the maximum data that can be hold by this vector.
	 * Existing entries are preserved, new_size must be big enough to hold them
	 */
	void resize(uint new_size)
	{
		if (new_size > 255) {
			dbg->fatal("minivec_tpl<T>::resize()", "new size %u too large (>255).", new_size);
		}
		// not yet used, but resize may be called anyway
		if(size<=0) {
			size = new_size;
			data = new T[size];
			return;
		}

		if (new_size <= size) return; // do nothing

		T* new_data = new T[new_size];
		for (uint i = 0; i < count; i++) new_data[i] = data[i];
		delete [] data;
		size = new_size;
		data = new_data;
	}

	/**
	 * Checks if element elem is contained in vector.
	 * Uses the == operator for comparison.
	 */
	bool is_contained(T elem) const
	{
		for (uint i = 0; i < count; i++) {
			if (data[i] == elem) return true;
		}
		return false;
	}


	/**
	 * Appends the element at the end of the vector.
	 * if out of space, extend it
	 */
	void append(T elem, uint8 extend = 1)
	{
		if (count >= size) {
			resize( count > 255-extend ? 255 : count+extend);
		}
		data[count++] = elem;
	}

	/** Checks if element is contained. Appends only new elements. */
	bool append_unique(T elem)
	{
		if (is_contained(elem)) return false;
		append(elem);
		return true;
	}

	/** Removes element, if contained */
	void remove(T elem)
	{
		uint i, j;
		for (i = j = 0; i < count; i++, j++) {
			if (data[i] == elem) {
				// skip this one
				j++;
				count--;
			}
			// maybe we copy too often ...
			if (j < size) {
				data[i] = data[j];
			}
		}
	}

	/** Inserts data at a certain pos */
	void insert_at(uint8 pos, T elem)
	{
		if (pos > count) {
			dbg->fatal("minivec_tpl<T>::append()", "cannot insert at %i! Only %i elements.", pos, count);
		}

		if (pos < count) {
			if (count == size) resize(count + 1);
			for (uint i = count; i > pos; i--) data[i] = data[i - 1];
			data[pos] = elem;
			count++;
		} else {
			append(elem, 1);
		}
	}

	/** Removes element at position */
	bool remove_at(uint8 pos)
	{
		if (pos < count) {
			for (uint i = pos+1; i < count; i++) {
				data[i-1] = data[i];
			}
			count--;
			return true;
		}
		return false;
	}

	T& operator [](uint8 i)
	{
		if (i >= count) dbg->fatal("minivec_tpl<T>::[]", "index out of bounds: %i not in 0..%d", i, count - 1);
		return data[i];
	}

	const T& operator [](uint8 i) const
	{
		if (i >= count) dbg->fatal("minivec_tpl<T>::[]", "index out of bounds: %i not in 0..%d", i, count - 1);
		return data[i];
	}

	T& back() { return data[count - 1]; }
	const T& back() const { return data[count - 1]; }

	iterator begin() { return data; }
	iterator end()   { return data + count; }

	const_iterator begin() const { return data; }
	const_iterator end()   const { return data + count; }

	/** Get the number of elements in the vector */
	uint8 get_count() const { return count; }


	/** Get the capacity */
	uint8 get_size() const { return size; }

	bool empty() const { return count == 0; }

private:
	minivec_tpl(const minivec_tpl&);
	minivec_tpl& operator=( minivec_tpl const& other );

	T* data;
	uint8 size;  ///< Capacity
	uint8 count; ///< Number of elements in vector
} GCC_PACKED;

#endif
