#ifndef TPL_VECTOR_H
#define TPL_VECTOR_H

#include <stdlib.h>
#include "../macros.h"
#include "../simtypes.h"
#include "../simdebug.h"


template<class T> class vector_tpl;
template<class T> inline void swap(vector_tpl<T>& a, vector_tpl<T>& b);


/** A template class for a simple vector type */
template<class T> class vector_tpl
{
	public:
		/** Construct a vector for cap elements */
		explicit vector_tpl(uint32 cap) : data(cap > 0 ? new T[cap] : NULL), size(cap), count(0) {}

		~vector_tpl() { delete [] data; }

		/** sets the vector to empty */
		void clear() { count = 0; }

		/**
		 * Resizes the maximum data that can be hold by this vector.
		 * Existing entries are preserved, new_size must be big enough to hold them
		 */
		void resize(uint32 new_size)
		{
			if (new_size <= size) return; // do nothing

			T* new_data = new T[new_size];
			for (uint32 i = 0; i < count; i++) new_data[i] = data[i];
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
			for (uint32 i = 0; i < count; i++) {
				if (data[i] == elem) return true;
			}
			return false;
		}

		/** Appends the element at the end of the vector.  */
		bool append(T elem)
		{
			if (count >= size) {
				dbg->fatal("vector_tpl<T>::append()", "capacity %i exceeded.",size);
				return false;
			}
			data[count++] = elem;
			return true;
		}

		/**
		 * Appends the element at the end of the vector.
		 * if out of space, extend by this amount
		 */
		void append(T elem, uint32 extend)
		{
			if (count >= size) resize(count + extend);
			data[count++] = elem;
		}

		/** Checks if element is contained. Appends only new elements. */
		bool append_unique(T elem)
		{
			if (is_contained(elem)) return false;
			return append(elem);
		}

		/**
		 * Checks if element is contained. Appends only new elements.
		 * extend vector if nessesary
		 */
		bool append_unique(T elem,uint32 extend)
		{
			if (is_contained(elem)) return false;
			append(elem, extend);
			return true;
		}

		/** removes element, if contained */
		bool remove(T elem)
		{
			uint32 i, j;
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
			return true;
		}

		/** insets data at a certain pos */
		void insert_at(uint32 pos, T elem)
		{
			if (pos < count) {
				if (count == size) resize(size + 1);
				for (uint i = count; i > pos; i--) data[i] = data[i - 1];
				data[pos] = elem;
				count++;
			} else {
				append(elem, 1);
			}
		}

		/**
		 * put the data at a certain position
 		 * BEWARE: using this function will create default objects, depending on
		 * the type of the vector
		 */
		void store_at(uint32 pos, T elem)
		{
			if (pos >= size) resize((pos & 0xFFFFFFF7) + 8);
			data[pos] = elem;
			if (pos >= count) count = pos + 1;
		}

		/** removes element at position */
		bool remove_at(uint32 pos)
		{
			if (pos < count) {
				for (uint i = pos; i < count; i++) data[i] = data[i + 1];
				count--;
				return true;
			}
			return false;
		}

		/** Gets the element at position i */
		const T& get(uint32 i) const
		{
			if (i >= count) {
				dbg->fatal("vector_tpl<T>::get()", "index out of bounds: %i not in 0..%d", i, count - 1);
			}
			return data[i];
		}

		/** Accesses the element at position i */
		T& at(uint32 i) const
		{
			if (i >= count) {
				dbg->fatal("vector_tpl<T>::at()", "index out of bounds: %i not in 0..%d\n", i, count - 1);
			}
			return data[i];
		}

		/** Get the number of elements in the vector */
		uint32 get_count() const { return count; }

		/** Get the capacity */
		uint32 get_size() const { return size; }

	private:
		vector_tpl(const vector_tpl&);

		T* data;
		uint32 size;  ///< Capacity
		uint32 count; ///< Number of elements in vector


	friend void swap<>(vector_tpl<T>& a, vector_tpl<T>& b);
};


template<class T> inline void swap(vector_tpl<T>& a, vector_tpl<T>& b)
{
	swap(a.data,  b.data);
	swap(a.size,  b.size);
	swap(a.count, b.count);
}

#endif
