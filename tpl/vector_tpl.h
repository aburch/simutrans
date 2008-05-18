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
		typedef const T* const_iterator;
		typedef       T* iterator;

		/** Construct a vector for cap elements */
		vector_tpl() : data(NULL), size(0), count(0) {}
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
			if(size>0) {
				for (uint32 i = 0; i < count; i++) {
					new_data[i] = data[i];
				}
				delete [] data;
			}
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

		/**
		 * Checks if element elem is contained in vector.
		 * Uses the == operator for comparison.
		 */
		uint32 index_of(T elem) const
		{
			for (uint32 i = 0; i < count; i++) {
				if (data[i] == elem) return i;
			}
			return 0xFFFFFFFFu;
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

		void push_back(const T& elem)
		{
			if (count == size) resize(size == 0 ? 1 : size * 2);
			data[count++] = elem;
		}

		/**
		 * Checks if element is contained. Appends only new elements.
		 * extend vector if nessesary
		 */
		bool push_back_unique(T elem)
		{
			if (is_contained(elem)) return false;
			push_back(elem);
			return true;
		}

		/** removes element, if contained */
		void remove(T elem)
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
		void remove_at(uint32 pos)
		{
			assert(pos<count);
			for (uint i = pos; i < count - 1; i++) {
				data[i] = data[i + 1];
			}
			count--;
		}

		T& operator [](uint i)
		{
			if (i >= count) {
				dbg->fatal("vector_tpl<T>::[]", "index out of bounds: %i not in 0..%d", i, count - 1);
			}
			return data[i];
		}

		const T& operator [](uint i) const
		{
			if (i >= count) {
				dbg->fatal("vector_tpl<T>::[]", "index out of bounds: %i not in 0..%d", i, count - 1);
			}
			return data[i];
		}

		T& back() const { return data[count - 1]; }

		iterator begin() { return data; }
		iterator end()   { return data + count; }

		const_iterator begin() const { return data; }
		const_iterator end()   const { return data + count; }

		/** Get the number of elements in the vector */
		uint32 get_count() const { return count; }

		/** Get the capacity */
		uint32 get_size() const { return size; }

		bool empty() const { return count == 0; }

	private:
		vector_tpl(const vector_tpl&);

		T* data;
		uint32 size;  ///< Capacity
		uint32 count; ///< Number of elements in vector


	friend void swap<>(vector_tpl<T>& a, vector_tpl<T>& b);
};


template<class T> inline void swap(vector_tpl<T>& a, vector_tpl<T>& b)
{
	sim::swap(a.data,  b.data);
	sim::swap(a.size,  b.size);
	sim::swap(a.count, b.count);
}

#endif
