#ifndef TPL_ARRAY_TPL_H
#define TPL_ARRAY_TPL_H

#include <typeinfo>
#include "../simdebug.h"
#include "../simtypes.h"

/**
 * A template class for bounds checked 1-dimesnional arrays.
 * This is kept as simple as possible. Does not use exceptions
 * for error handling.
 */
template<class T> class array_tpl
{
	public:
		typedef uint16 index;

		explicit array_tpl(index s) : data(new T[s]), size(s) {}

		array_tpl(index s, const T* arr) : data(new T[s]), size(s)
		{
			for (int i = 0; i < size; i++) data[i] = arr[i];
		}

		~array_tpl() { delete [] data; }

		index get_size() const { return size; }

		void resize(index resize)
		{
			if (size < resize) {
				T *new_data = new T[size];
				for (index i = 0;  i < size;  i++ ) new_data[i] = data[i];
				delete [] data;
				data = new_data;
				size = resize;
			}
		}

		T& at(index i) const
		{
			if (i >= size) {
				dbg->fatal("array_tpl<%s>::at()", "index out of bounds: %d not in 0..%d", typeid(T).name(), i, size - 1);
			}
			return data[i];
		}

		const T& get(index i) const
		{
			if (i >= size) {
				dbg->fatal("array_tpl<%s>::get()", "index out of bounds: %d not in 0..%d", typeid(T).name(), i, size - 1);
			}
			return data[i];
		}

	private:
		array_tpl(const array_tpl&);

		T* data;
		index size;
};

#endif
