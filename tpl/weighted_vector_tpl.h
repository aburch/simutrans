#ifndef TPL_WEIGHTED_VECTOR_H
#define TPL_WEIGHTED_VECTOR_H

#ifndef ITERATE
#define ITERATE(collection,enumerator) for(uint16 enumerator = 0; enumerator < collection.get_count(); enumerator++)
#endif

#ifndef ITERATE_PTR
#define ITERATE_PTR(collection,enumerator) for(uint16 enumerator = 0; enumerator < collection->get_count(); enumerator++)
#endif 

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

		weighted_vector_tpl() : nodes(NULL), size(0), count(0), total_weight(0) {}

		/** Construct a vector for size elements */
		explicit weighted_vector_tpl(uint32 size)
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
		void resize(uint32 new_size)
		{
			if (new_size <= size) return; // do nothing

			nodestruct* new_nodes = new nodestruct[new_size];
			for (uint32 i = 0; i < count; i++) new_nodes[i] = nodes[i];
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
			for (uint32 i = 0; i < count; i++) {
				if (nodes[i].data == elem) return true;
			}
			return false;
		}

		/**
		 * Checks if element elem is contained in vector.
		 * Uses the == operator for comparison.
		 */
		template<typename U> uint32 index_of(U elem) const
		{
			for (uint32 i = 0; i < count; i++) {
				if (nodes[i].data == elem) return i;
			}
			dbg->fatal("weighted_vector_tpl<T>::index_of()", "not contained" );
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
		bool append(T elem, unsigned long weight, uint32 extend)
		{
#ifdef IGNORE_ZERO_WEIGHT
			if (weight == 0) {
				// ignore unused entries ...
				return false;
			}
#endif
			if (count == size) {
				resize(count + extend);
			}
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
		bool append_unique(T elem, unsigned long weight, uint32 extend)
		{
			return is_contained(elem) || append(elem, weight, extend);
		}

		/** inserts data at a certain pos */
		bool insert_at(uint32 pos, T elem, unsigned long weight, uint32 extend = 1u)
		{
#ifdef IGNORE_ZERO_WEIGHT
			if (weight == 0) {
				// ignore unused entries ...
				return false;
			}
#endif
			if (pos < count) {
				if(  count==size  ) {
					resize(size + extend);
				}
				for (uint32 i = count; i > pos; i--) {
					nodes[i].data   = nodes[i - 1].data;
					nodes[i].weight = nodes[i - 1].weight + weight;
				}
				nodes[pos].data = elem;
				total_weight += weight;
				count++;
				return true;
			}
			else {
				return append(elem, weight, 1);
			}
		}

		/**
		 * Insert `elem' with respect to ordering.
		 */
		template<class StrictWeakOrdering>
		void insert_ordered(const T& elem, unsigned long weight, StrictWeakOrdering comp, uint32 extend = 1u)
		{
			if(  count==size  ) {
				resize(size + extend);
			}
			sint32 high = count, low = -1;
			while(  high-low>1  ) {
				const sint32 mid = ((uint32)(high + low)) >> 1;
				if(  comp(elem, nodes[mid].data)  ) {
					high = mid;
				}
				else {
					low = mid;
				}
			}
			insert_at(high, elem, weight);
		}

		/**
		 * Only insert `elem' if not already contained in this vector.
		 * Respects the ordering and assumes the vector is ordered.
		 * Returns NULL if insertion is successful;
		 * Otherwise return the address of the element in conflict.
		 */
		template<class StrictWeakOrdering>
		T* insert_unique_ordered(const T& elem, unsigned long weight, StrictWeakOrdering comp, uint32 extend = 1u)
		{
			if(  count==size  ) {
				resize(size + extend);
			}
			sint32 high = count, low = -1;
			while(  high-low>1  ) {
				const sint32 mid = ((uint32)(high + low)) >> 1;
				T &mid_elem = nodes[mid].data;
				if(  elem==mid_elem  ) {
					return &mid_elem;
				}
				else if(  comp(elem, mid_elem)  ) {
					high = mid;
				}
				else {
					low = mid;
				}
			}
			insert_at(high, elem, weight);
			return NULL;
		}

		/**
		 * Search for an element, assuming that the vector is sorted.
		 * If the element is found, return its address; otherwise, return NULL.
		 */
		template<class StrictWeakOrdering>
		T* search_ordered(const T& elem, StrictWeakOrdering comp)
		{
			sint32 high = count, low = -1;
			while(  high-low>1  ) {
				const sint32 mid = ((uint32)(high + low)) >> 1;
				T &mid_elem = nodes[mid].data;
				if(  elem==mid_elem  ) {
					return &mid_elem;
				}
				else if(  comp(elem, mid_elem)  ) {
					high = mid;
				}
				else {
					low = mid;
				}
			}
			return NULL;
		}

		/**
		 * A class for representing a subset of a weighted vector's elements
		 * @author Knightly
		 */
		class subset
		{
		private:

			const weighted_vector_tpl<T> *weighted_vector;
			uint32 lower_index;	// inclusive in the range of elements
			uint32 upper_index;	// exclusive in the range of elements

			subset(const weighted_vector_tpl<T> *const vector, const uint32 lower_idx, const uint32 upper_idx)
				: weighted_vector(vector), lower_index(lower_idx), upper_index(upper_idx)
			{
				// invalid boundaries or zero sum of weight -> reduce to empty subset
				if(  upper_index>weighted_vector->get_count()  ||  lower_index>=upper_index  ||  get_sum_weight()==0  ) {
					lower_index = upper_index = 0;
				}
			}

		public:

			bool is_empty() const { return lower_index == upper_index; }

			uint32 get_count() const { return upper_index - lower_index; }

			unsigned long weight_at(const uint32 index) const
			{
				if(  index>=get_count()  ) {
					dbg->fatal("weighted_vector_tpl<T>::subset::weight_at()", "index out of bounds: %u not in 0..%u range", index, get_count() - 1);
				}
				return weighted_vector->weight_at(lower_index + index) - weighted_vector->weight_at(lower_index);
			}

			unsigned long get_sum_weight() const
			{
				return weighted_vector->weight_at(upper_index) - weighted_vector->weight_at(lower_index);
			}

			T& at_weight(const unsigned long target_weight) const
			{
				const uint32 adjusted_weight = weighted_vector->weight_at(lower_index) + target_weight;
				if(  adjusted_weight>=weighted_vector->weight_at(upper_index)  ) {
					dbg->fatal("weighted_vector_tpl<T>::subset::at_weight()", "weight out of bounds: %u not in 0..%u range", target_weight, weighted_vector->weight_at(upper_index) - weighted_vector->weight_at(lower_index) - 1);
				}
				return weighted_vector->at_weight(adjusted_weight);
			}

			const T& operator [] (const uint32 index) const
			{
				if(  index>=get_count()  ) {
					dbg->fatal("weighted_vector_tpl<T>::subset::operator[]", "index out of bounds: %u not in 0..%u range", index, get_count() - 1);
				}
				return (*weighted_vector)[lower_index + index];
			}

			friend class weighted_vector_tpl;
		};

		/**
		 * Search for the closest index, assuming that the vector is sorted.
		 */
		template<class StrictWeakOrdering>
		uint32 search_index_ordered(const T& elem, StrictWeakOrdering comp) const
		{
			sint32 high = count, low = -1;
			while(  high-low>1  ) {
				const sint32 mid = ((uint32)(high + low)) >> 1;
				if(  comp(elem, nodes[mid].data)  ) {
					high = mid;
				}
				else {
					low = mid;
				}
			}
			return high;
		}

		/**
		 * Return a subset of the vector's elements in the specified range.
		 * The vector is assumed to be sorted. Both lower and upper bounds are inclusive.
		 * @author Knightly
		 */
		template<class OrderEqual, class OrderNotEqual>
		subset get_subset_ordered(const T& lower_bound, const T& upper_bound, OrderEqual order_equal, OrderNotEqual order_not_equal) const
		{
			return subset( this, search_index_ordered(lower_bound, order_equal), search_index_ordered(upper_bound, order_not_equal) );
		}

		/**
		 * Return a subset of the vector's elements in the specified range of indices.
		 * The lower index is inclusive, but the upper index is exclusive, from the range.
		 * @author Knightly
		 */
		subset get_subset(const uint32 lower_index, const uint32 upper_index) const
		{
			return subset( this, lower_index, upper_index );
		}

		/**
		 * Update the weight of the element, if contained
		 * @author Knightly
		 */
		bool update(T elem, unsigned long weight)
		{
			for(  uint32 i=0;  i<count;  ++i  ) {
				if(  nodes[i].data==elem  ) {
					return update_at(i, weight);
				}
			}
			return false;
		}

		/**
		 * Update the weight of the element at the specified position
		 * @author Knightly
		 */
		bool update_at(uint32 pos, unsigned long weight)
		{
			if(  pos>=count  ) {
				return false;
			}
			const unsigned long elem_weight = ( pos + 1 < count ? nodes[pos + 1].weight : total_weight ) - nodes[pos].weight;
			if(  weight>elem_weight  ) {
				const unsigned long delta_weight = weight - elem_weight;
				while(  ++pos<count  ) {
					nodes[pos].weight += delta_weight;
				}
				total_weight += delta_weight;
			}
			else if(  weight<elem_weight  ) {
				const unsigned long delta_weight = elem_weight - weight;
				while(  ++pos<count  ) {
					nodes[pos].weight -= delta_weight;
				}
				total_weight -= delta_weight;
			}
			return true;
		}

		/** removes element, if contained */
		bool remove(T elem)
		{
			for (uint32 i = 0; i < count; i++)
			{
				if (nodes != NULL && nodes[i].data == elem) return remove_at(i);
			}
			return false;
		}

		/** removes element at position */
		bool remove_at(uint32 pos)
		{
			if (pos >= count) return false;
			// get the change in the weight; must check, if it isn't the last element
			const unsigned long diff_weight = ( pos + 1 < count ? nodes[pos + 1].weight : total_weight ) - nodes[pos].weight;
			for (uint32 i = pos; i < count - 1; i++) {
				nodes[i].data   = nodes[i + 1].data;
				nodes[i].weight = nodes[i + 1].weight - diff_weight;
			}
			count--;
			total_weight -= diff_weight;
			return true;
		}

		T& operator [](uint32 i)
		{
			if (i >= count) dbg->fatal("weighted_vector_tpl<T>::get()", "index out of bounds: %i not in 0..%d", i, count - 1);
			return nodes[i].data;
		}

		const T& operator [](uint32 i) const
		{
			if (i >= count) dbg->fatal("weighted_vector_tpl<T>::get()", "index out of bounds: %i not in 0..%d", i, count - 1);
			return nodes[i].data;
		}

		T& front() { return nodes[0].data; }

		/** returns the weight at a position */
		unsigned long weight_at(uint32 pos) const
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
			uint32 pos;
			unsigned long current_weight = 0;
			for (pos = 0;  pos < count - 1 && current_weight + nodes[pos + 1].weight < target_weight; pos++) {
				current_weight += nodes[pos + 1].weight;
			}
#else
			// ... and that the much faster binary search
			uint32 diff = 1;
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
				diff = (diff + 1) >> 1;
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
		uint32 get_count() const { return count; }

		/** Gets the capacity */
		uint32 get_size() const { return size; }

		/** Gets the total weight */
		unsigned long get_sum_weight() const { return total_weight; }

		bool empty() const { return count == 0; }

		iterator begin() { return iterator(nodes); }
		iterator end()   { return iterator(nodes + count); }

		const_iterator begin() const { return const_iterator(nodes); }
		const_iterator end()   const { return const_iterator(nodes + count); }

	private:
		nodestruct* nodes;
		uint32 size;                  ///< Capacity
		uint32 count;                 ///< Number of elements in vector
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
