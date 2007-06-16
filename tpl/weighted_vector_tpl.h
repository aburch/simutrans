#ifndef TPL_WEIGHTED_VECTOR_H
#define TPL_WEIGHTED_VECTOR_H

#include "../macros.h"
#include "../simdebug.h"


template<class T> class weighted_vector_tpl;
template<class T> void swap(weighted_vector_tpl<T>&, weighted_vector_tpl<T>&);


/** A template class for a simple vector type */
template<class T> class weighted_vector_tpl
{
	private:
		struct nodestruct
		{
			T data;
			unsigned long weight;
		};

	public:
		class const_iterator;

		class iterator
		{
			public:
				T& operator *() const { return ptr->data; }

				iterator& operator ++() { ++ptr; return *this; }

				bool operator !=(const iterator& o) { return ptr != o.ptr; }

			private:
				explicit iterator(nodestruct* ptr_) : ptr(ptr_) {}

				nodestruct* ptr;

			friend class weighted_vector_tpl;
			friend class const_iterator;
		};

		class const_iterator
		{
			public:
				const_iterator(const iterator& o) : ptr(o.ptr) {}

				const T& operator *() const { return ptr->data; }

				const_iterator& operator ++() { ++ptr; return *this; }

				bool operator !=(const const_iterator& o) { return ptr != o.ptr; }

			private:
				explicit const_iterator(nodestruct* ptr_) : ptr(ptr_) {}

				const nodestruct* ptr;

			friend class weighted_vector_tpl;
		};

		/** Construct a vector for size elements */
		explicit weighted_vector_tpl(uint size)
		{
			this->size  = size;
			nodes = (size > 0 ? new nodestruct[size] : NULL);
			count = 0;
			total_weight = 0;
		}

		~weighted_vector_tpl() { delete [] nodes; }

		/** sets the vector to empty */
		void clear()
		{
			count = 0;
			total_weight = 0;
		}

		/**
		 * Resizes the maximum data that can be hold by this vector.
		 * Existing entries are preserved, new_size must be big enough
		 * to hold them.
		 */
		void resize(uint new_size)
		{
			if (new_size <= size) return; // do nothing

			nodestruct* new_nodes = new nodestruct[new_size];
			for (uint i = 0; i < count; i++) new_nodes[i] = nodes[i];
			delete [] nodes;
			size  = new_size;
			nodes = new_nodes;
		}

		/**
		 * Checks if element elem is contained in vector.
		 * Uses the == operator for comparison.
		 */
		bool is_contained(T elem) const
		{
			for (uint i = 0; i < count; i++) {
				if (nodes[i].data == elem) return true;
			}
			return false;
		}


		/**
		 * Appends the element at the end of the vector.
		 */
		bool append(T elem, unsigned long weight)
		{
#ifdef IGNORE_ZERO_WEIGHT
			if (weight == 0) {
				// ignore unused entries ...
				return false;
			}
#endif
			if (count >= size) {
				dbg->fatal("weighted_vector_tpl<T>::append()", "capacity %i exceeded.", size);
				return false;
			}
			nodes[count].data   = elem;
			nodes[count].weight = total_weight;
			count++;
			total_weight += weight;
			return true;
		}

		/**
		 * Appends the element at the end of the vector.
		 * of out of spce, extend with this factor
		 * @author prissi
		 */
		bool append(T elem, unsigned long weight, uint extend)
		{
#ifdef IGNORE_ZERO_WEIGHT
			if (weight == 0) {
				// ignore unused entries ...
				return false;
			}
#endif
			if (count == size) resize(count + extend);
			nodes[count].data   = elem;
			nodes[count].weight = total_weight;
			count++;
			total_weight += weight;
			return true;
		}

		/**
		 * Checks if element is contained. Appends only new elements.
		 */
		bool append_unique(T elem, unsigned long weight)
		{
			return is_contained(elem) || append(elem, weight);
		}

		/**
		 * Checks if element is contained. Appends only new elements.
		 * extend vector if nessesary
		 */
		bool append_unique(T elem, unsigned long weight, uint extend)
		{
			return is_contained(elem) || append(elem, weight, extend);
		}

		/** inserts data at a certain pos */
		bool insert_at(uint pos, T elem, unsigned long weight)
		{
#ifdef IGNORE_ZERO_WEIGHT
			if (weight == 0) {
				// ignore unused entries ...
				return false;
			}
#endif
			if (pos < count) {
				if (count == size) resize(size + 1);
				for (uint i = count; i > pos; i--) {
					nodes[i].data   = nodes[i - 1].data;
					nodes[i].weight = nodes[i - 1].weight + weight;
				}
				nodes[pos].data = elem;
				total_weight += weight;
				count++;
				return true;
			} else {
				return append(elem, weight, 1);
			}
		}

		/** removes element, if contained */
		bool remove(T elem)
		{
			unsigned long diff_weight = 0;
			uint i;
			uint j;

			for (i = j = 0; i < count; i++, j++) {
				if (nodes[i].data == elem) {
					// skip this one
					j++;
					// get the change in the weight; must check, if it isn't the last element
					diff_weight = (i + 1 < count ? nodes[i + 1].weight - nodes[i].weight : total_weight - nodes[i].weight);
					count--;
				}
				// maybe we copy too often ...
				if (j < size) {
					nodes[i].data   = nodes[j].data;
					nodes[i].weight = nodes[j].weight - diff_weight;
				}
			}
			total_weight -= diff_weight;
			return true;
		}

		/** removes element at position */
		bool remove_at(uint pos)
		{
			if (pos >= count) return false;
			// get the change in the weight; must check, if it isn't the last element
			const unsigned long diff_weight = (pos + 1 < count ? nodes[pos + 1].weight - nodes[pos].weight : total_weight - nodes[pos].weight);
			for (uint i = pos; i < count - 1; i++) {
				nodes[i].data   = nodes[i + 1].data;
				nodes[i].weight = nodes[i + 1].weight - diff_weight;
			}
			count--;
			total_weight -= diff_weight;
			return true;
		}

		T& operator [](uint i)
		{
			if (i >= count) dbg->fatal("weighted_vector_tpl<T>::get()", "index out of bounds: %i not in 0..%d", i, count - 1);
			return nodes[i].data;
		}

		const T& operator [](uint i) const
		{
			if (i >= count) dbg->fatal("weighted_vector_tpl<T>::get()", "index out of bounds: %i not in 0..%d", i, count - 1);
			return nodes[i].data;
		}

		T& front() { return nodes[0].data; }

		/** returns the weight at a position */
		unsigned long weight_at(uint pos) const
		{
			return (pos < count) ? nodes[pos].weight : total_weight + 1;
		}

		/** Accesses the element at position i by weight */
		T& at_weight(const unsigned long target_weight) const
		{
			if (target_weight > total_weight) {
				dbg->fatal("weighted_vector_tpl<T>::at_weight()", "weight out of bounds: %i not in 0..%d", target_weight, total_weight);
			}
#if 0
			// that is the main idea (but slower, the more entries are in the list)
			uint pos;
			unsigned long current_weight = 0;
			for (pos = 0;  pos < count - 1 && current_weight + nodes[pos + 1].weight < target_weight; pos++) {
				current_weight += nodes[pos + 1].weight;
			}
#else
			// ... and that the much faster binary search
			uint diff = 1;
			sint8 counter = 0;
			// now make sure, diff is 2^n
			while (diff <= count) {
				diff <<= 1;
				counter++;
			}
			diff >>= 1;

			uint pos = diff;

			// now search
			while (counter-- > 0) {
				diff = (diff + 1) / 2;
				if (pos < count && weight_at(pos) <= target_weight) {
					if (weight_at(pos + 1) > target_weight) break;
					pos += diff;
				} else {
					pos -= diff;
				}
			}
#endif
			//DBG_DEBUG("at_weight()", "target_weight=%i found_weight=%i at pos %i", target_weight, weight_at(pos), pos);
			return nodes[pos].data;
		}

		/** Gets the number of elements in the vector */
		uint get_count() const { return count; }

		/** Gets the capacity */
		uint get_size() const { return size; }

		/** Gets the total weight */
		uint get_sum_weight() const { return total_weight; }

		bool empty() const { return count == 0; }

		iterator begin() { return iterator(nodes); }
		iterator end()   { return iterator(nodes + count); }

		const_iterator begin() const { return const_iterator(nodes); }
		const_iterator end()   const { return const_iterator(nodes + count); }

	private:
		nodestruct* nodes;
		uint size;                  ///< Capacity
		uint count;                 ///< Number of elements in vector
		unsigned long total_weight; ///< Sum of all weights

	friend void swap<>(weighted_vector_tpl<T>&, weighted_vector_tpl<T>&);
};


template<class T> void swap(weighted_vector_tpl<T>& a, weighted_vector_tpl<T>& b)
{
	sim::swap(a.nodes, b.nodes);
	sim::swap(a.size, b.size);
	sim::swap(a.count, b.count);
	sim::swap(a.total_weight, b.total_weight);
}

#endif
