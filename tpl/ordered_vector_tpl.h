#ifndef TPL_ORDERED_VECTOR_TPL_H
#define TPL_ORDERED_VECTOR_TPL_H

#ifndef ITERATE
#define ITERATE(collection,i) for(uint16 i = 0; i < collection.get_count(); i++)
#endif

#ifndef ITERATE_PTR
#define ITERATE_PTR(collection,i) for(uint16 i = 0; i < collection->get_count(); i++)
#endif 

#include "../simtypes.h"
#include "../simdebug.h"
#include "../convoihandle_t.h"
#include "../linehandle_t.h"
#include "../halthandle_t.h"

template<class T, class inttype> class ordered_vector_tpl;

//Doesn't work:   :-(
//template<class T> typedef ordered_vector_tpl<T, uint32> ordered_vector;
//template<class T> typedef ordered_vector_tpl<T, uint8> ordered_minivec;

typedef ordered_vector_tpl<uint8, uint8> catg_index_vec;
typedef ordered_vector_tpl<halthandle_t, uint16> halthandle_vec;
typedef ordered_vector_tpl<linehandle_t, uint16> linehandle_vec;
typedef ordered_vector_tpl<convoihandle_t, uint16> convoihandle_vec;

/*
 * Template class for a increasingly ordered vector with unique entries.
 * It needs <= and == for the class T.
 * Since we have also a template for the count and size-variables
 * we are able to define short and long vectors with this class.
 */
template<class T, class inttype> class ordered_vector_tpl
{
	private:
		T* data;
		inttype size;  ///< Capacity
		inttype count; ///< Number of elements in vector

	public:
		ordered_vector_tpl() :
			data(NULL),
			size(0),
			count(0) {}
		/** Construct a vector for new_size elements */
		explicit ordered_vector_tpl(inttype new_size) :
			data(new_size > 0 ? new T[new_size] : NULL),
			size(new_size),
			count(0) {}

		ordered_vector_tpl(const ordered_vector_tpl<T,inttype> &vT)
		{
			count = size = 0;
			data = NULL;
			resize( vT.get_count() );
			count = vT.get_count();
			for(  inttype i=0;  i<count;  i++  ) {
				data[i] = vT.data[i];
			}
		}

		ordered_vector_tpl& operator = (const ordered_vector_tpl<T, inttype> &vT)
		{
			if(  this != &vT  ) { // Otherwise we will lost all our data!
				count = 0; // Clear our elements so resize will work properly.
				resize( vT.get_count() );
				count = vT.get_count();
				for(  inttype i=0;  i<count;  i++  ) {
					data[i] = vT.data[i];
				}
			}
			return *this;
		}

		~ordered_vector_tpl() {
			if(  data  ) {
				delete [] data;
			}
		}

		/** sets the vector to empty */
		void clear() { count = 0; }

		// Check if elem is contained.
		bool contains(T elem) const
		{
			inttype index = intern_search(elem);
			if(  index < count  &&  data[index] == elem  ) {
				return true;
			}
			else {
				return false;
			}
		}

		/*
		 * Insert only if elem isn't yet contained.
		 * Returns false if elem already contained.
		 */
		bool insert_unique(T elem, inttype i = 1)
		{
			if(  count == size  ) {
				if(  i == 0  ) {
					resize(  size == 0  ? 1 : 2*size );
				}
				else {
					resize( size + i );
				}
			}
			inttype index = intern_search(elem);
			if(  index < count  &&  data[index] == elem  ) {
				return false;
			}
			move_data(index, count, +1);
			data[index] = elem;
			count++;
      return true;
		}

		// Removes elem.
		bool remove(T elem)
		{
			inttype index = intern_search(elem);
			if(  index < count  &&  data[index] == elem  ) {
				remove_at(index);
				return true;
			}
			return false;
		}

		//Removes the element at the given pos.
		void remove_at(inttype pos)
		{
			move_data(pos+1, count, -1); 
			count--;
		}

		/*
		 * Handle set operations.
		 * Assume that all values in this and vT are unique and
		 * in the right order!
		 */
		void set_union(const ordered_vector_tpl<T, inttype> vT)
		{
			T* old_data = data;
			inttype old_count = count;

			data = new T[ count + vT.count ];
			count = 0;

			inttype i,j;
			i = j = 0;
			while(  i < old_count  &&  j < vT.count  ) {
				if(  old_data[i] == vT.data[j]  ) {
					data[ count++ ] = old_data[i];
					i++;
					j++;
				}
				else
					if(  old_data[i] <= vT.data[j]  ) {
						data[ count++ ] = old_data[i];
						i++;
					}
					else
					{
						data[ count++ ] = vT.data[j];
						j++;
					}
			}
			while(  i < old_count  ) {
				data[ count++ ] = old_data[i];
				i++;
			}
			while(  j < vT.count  ) {
				data[ count++ ] = vT.data[j];
				j++;
			}
		};

		void set_diff(const ordered_vector_tpl<T, inttype> vT)
		{
			inttype i,j;
			i = j = 0;
			while(  i < count  &&  j < vT.count  ) {
				if(  data[i] == vT.data[j]  ) {
					i++;
					j++;
				}
				else
					if(  data[i] <= vT.data[j]  ) {
						remove_at(i);
					}
					else
					{
						j++;
					}
			}
			// Delete remaining elements:
			count = i;
		};

		void set_minus(const ordered_vector_tpl<T, inttype> vT)
		{
			inttype i,j;
			i = j = 0;
			while(  i < count  &&  j < vT.count  ) {
				if(  data[i] == vT.data[j]  ) {
					remove_at(i);
					j++;
				}
				else
					if(  data[i] <= vT.data[j]  ) {
						i++;
					}
					else
					{
						j++;
					}
			}
		};

		/**
		 * Resizes the maximum data that can be hold by this vector.
		 * Existing entries are preserved, new_size must be big enough to hold them.
		 */
		void resize(inttype new_size)
		{
			if (  new_size < count  ||  new_size == size  ) {
				return;
			}

			T* new_data;
			if(  new_size > 0  ) {
				new_data = new T[new_size];
			}
			else {
				new_data = NULL;
			}
			if(size>0) {
				for (inttype i = 0; i < count; i++) {
					new_data[i] = data[i];
				}
				delete [] data;
			}
			size = new_size;
			data = new_data;
		}

		// Returns index of elem.
		inttype index_of(T elem) const
		{
			inttype index = intern_search(elem);
			if(  index < count  &&  data[index] == elem  ){
				return index;
			}
			else
			{
				assert(false);
				return 0-1;
			}
		}

		// Useful if accessing the vector by a pointer:
		T& get_by_index (inttype i) { return data[i]; }

		const T& get_by_index (inttype i) const { return data[i]; }

		T& operator [] (inttype i) { return data[i]; }

		const T& operator [] (inttype i) const { return data[i]; }

		inttype get_count() const { return count; }

		inttype get_size() const { return size; }

		bool is_empty() const { return count==0; }
	private:
		/*
		 * Moves the elements data[start_index]..data[end_index-1] to
		 * data[start_index+offset]..data[end_index-1+offset]
		 */
		void move_data(inttype start_index, inttype end_index, sint8 offset)
		{
			inttype num = end_index - start_index;
			if(  offset < 0 ) {
				for( inttype i=0; i<num ; i++ ){
					data[start_index+i+offset] = data[start_index+i];
				}
			}
			else if(  offset > 0  ) {
				for( inttype i=num ; i>0 ; i-- ){
					data[start_index+i+offset-1] = data[start_index+i-1];
				}
			}
		}

		/*
		 * Returns an index i with the following properties:
		 * 0<= i <= count
		 * data[i-1] < elem (if 0<= i-1 < count)
		 * data[i] >= elem  (if 0<= i   < count)
		 */
		inttype intern_search(T elem) const
		{
			// Test some special cases:
			if(  count == 0  ||  !(elem <= data[count-1])  ) {
				return count;
			}
			if( elem <= data[0]  ) {
				return 0;
			}
			inttype index_bottom=0, index_top=count-1;
			inttype index_mid;
			while(  index_top > index_bottom + 1  ){
				/*
				 * In this loop the following two statements hold
				 *   data[index_bottom] < elem
				 *   data[index_top] >= elem
				 */
				index_mid = (index_top + index_bottom) / 2;
				if(  elem <= data[index_mid]  ){
					index_top = index_mid;
				}
				else {
					index_bottom = index_mid;
				}
			}
			return index_top;
		}
};
#endif