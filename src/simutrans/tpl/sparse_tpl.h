/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TPL_SPARSE_TPL_H
#define TPL_SPARSE_TPL_H


#include "../dataobj/koord.h"
#include "../macros.h"
#include "../simtypes.h"


template <class T> class sparse_tpl;
template<class T> void swap(sparse_tpl<T>& a, sparse_tpl<T>& b);

/**
 * A template class for spares 2-dimensional arrays.
 * It's using Compressed Row Storage (CRS).
 * @see array2d_tpl
 */
template <class T>
class sparse_tpl
{
private:
	koord size;

	T* data;
	uint16* col_ind;
	uint16* row_ptr;

	uint16 data_size;
	uint16 data_count;

public:
	sparse_tpl( koord _size ) {
		size = _size;
		data_size = 0;
		data_count = 0;
		data = NULL;
		col_ind = NULL;
		row_ptr = new uint16[ size.y + 1];
		for( uint16 i = 0; i < size.y + 1; i++ ) {
			row_ptr[i] = 0;
		}
	}

	~sparse_tpl() {
		delete[] data;
		data = NULL;
		delete[] col_ind;
		col_ind = NULL;
		delete[] row_ptr;
		row_ptr = NULL;
	}

	void clear() {
		data_count = 0;
		for( uint16 i = 0; i < size.y + 1; i++ ) {
			row_ptr[i] = 0;
		}
		resize_data(0);
	}

	const koord& get_size() const {
		return size;
	}

	uint16 get_data_size() const {
		return data_size;
	}

	uint16 get_data_count() const {
		return data_count;
	}

	T get( koord pos ) const {
		if(  pos.x >= 0  &&  pos.y >= 0  &&  pos.x < size.x  &&  pos.y < size.y  ) {
			return get_unsafe( pos );
		}
		return 0;
	}

	T get( uint16 i, uint16 j ) const {
		return get(koord(i,j));
	}

	/*
		* Access to the nonzero elements. Result is saved in pos and value!
		*/
	void get_nonzero( uint16 index, koord& pos, T& value ) const {
		if(  index > data_count  ) {
			pos = koord::invalid;
			value = 0;
			return;
		}
		value = data[index];
		pos = koord(col_ind[index], 0);
		while( row_ptr[ pos.y+1 ] <= index ) {
			pos.y++;
		}
	}

	void set( koord pos, T value ) {
		if(  pos.x >= 0  &&  pos.y >= 0  &&  pos.x < size.x  &&  pos.y < size.y  ) {
			set_unsafe( pos, value );
		}
	}

	void set( uint16 i, uint16 j, T value ) {
		set(koord(i,j),value);
	}


private:
	T get_unsafe( koord pos ) const {
		uint16 index = pos_to_index( pos );
		if(  index < row_ptr[pos.y+1]  &&  col_ind[index] == pos.x  ) {
			return data[index];
		}
		else {
			return 0;
		}
	}

	void set_unsafe( koord pos, T value ) {
		uint16 index = pos_to_index( pos );
		if(  index < row_ptr[pos.y+1]  &&  col_ind[index] == pos.x  ) {
			if( value == 0 ) {
				move_data(index+1, data_count, -1);
				for( uint16 i = pos.y+1; i < size.y+1; i++ ){
					row_ptr[i]--;
				}
				data_count--;
			}
			else {
				data[index] = value;
			}
		}
		else {
			if( value == 0 ) {
				// Don't add 0 to data!
				return;
			}
			if(  data_count == data_size  ) {
				resize_data(data_size==0 ? 1 : 2*data_size);
			}
			move_data(index, data_count, 1);
			data[index] = value;
			col_ind[index] = pos.x;
			for( uint16 i = pos.y+1; i < size.y+1; i++ ){
				row_ptr[i]++;
			}
			data_count++;
		}
	}

	/*
	 * Moves the elements data[start_index]..data[end_index-1] to
	 * data[start_index+offset]..data[end_index-1+offset]
	 */
	void move_data(uint16 start_index, uint16 end_index, sint8 offset)
	{
		uint16 num = end_index - start_index;
		if(  offset < 0 ) {
			for( uint16 i=0; i<num ; i++ ){
				data[start_index+i+offset] = data[start_index+i];
				col_ind[start_index+i+offset] = col_ind[start_index+i];
			}
		}
		else if(  offset > 0  ) {
			for( uint16 i=num ; i>0 ; i-- ){
				data[start_index+i+offset-1] = data[start_index+i-1];
				col_ind[start_index+i+offset-1] = col_ind[start_index+i-1];
			}
		}
	}

	void resize_data( uint16 new_size ) {
		if(  new_size < data_count  ||  new_size == data_size  ) {
			// new_size is too small or equal our current size.
			return;
		}

		T* new_data;
		uint16* new_col_ind;
		if(  new_size > 0  ) {
			new_data = new T[new_size];
			new_col_ind = new uint16[new_size];
		}
		else
		{
			new_data = NULL;
			new_col_ind = NULL;
		}
		if(  data_size > 0  ) {
			for( uint16 i = 0; i < data_count; i++ ) {
				new_data[i] = data[i];
				new_col_ind[i] = col_ind[i];
			}
			delete[] data;
			delete[] col_ind;
		}
		data_size = new_size;
		data = new_data;
		col_ind = new_col_ind;
	}

	/*
	 * Returns an index i
	 * - to the pos (if pos already in array)
	 * - to the index, where pos can be inserted.
	 */
	uint16 pos_to_index( koord pos ) const {
		uint16 row_start = row_ptr[ pos.y ];
		uint16 row_end = row_ptr[ pos.y + 1 ];
		for( uint16 i = row_start; i < row_end; i++ ) {
			if( col_ind[i] >= pos.x ) {
				return i;
			}
		}
		return row_end;
	}

private:
	friend void swap<>(sparse_tpl<T>& a, sparse_tpl<T>& b);

	sparse_tpl(const sparse_tpl& other);
	sparse_tpl& operator=( sparse_tpl const& other );
};


template<class T> void swap(sparse_tpl<T>& a, sparse_tpl<T>& b)
{
	sim::swap(a.size,  b.size);

	sim::swap(a.data,  b.data);
	sim::swap(a.col_ind,  b.col_ind);
	sim::swap(a.row_ptr,  b.row_ptr);

	sim::swap(a.data_size, b.data_size);
	sim::swap(a.data_count, b.data_count);
}

#endif
