#ifndef quickstone_tpl_h
#define quickstone_tpl_h

#include "../simtypes.h"
#include "../simdebug.h"

/**
 * An implementation of the tombstone pointer checking method.
 * It uses an table of pointers and indizes into that table to
 * implement the tombstone system. Unlike real tombstones, this
 * template reuses entries from the tombstone tabel, but it tries
 * to leave free'd tombstones untouched as long as possible, to
 * detect most of the dangling pointers.
 *
 * This templates goal is to be efficient and fairly safe.
 *
 * @author Hj. Malthaner
 */
template <class T> class quickstone_tpl
{
private:
	/**
	 * Array of pointers. The first entry is always NULL!
	 */
	static T ** data;

	/**
	 * Next entry to check
	 */
	static uint16 next;

	/**
	 * Size of tombstone table
	 */
	static uint16 size;

	/**
	 * Retrieves next free tombstone index
	 */
	static uint16 find_next() {
		uint16 i;

		// scan rest of array
		for(  i=next;  i<size;  i++  ) {
			if(  data[i] == 0  ) {
				next = i+1;
				return i;
			}
		}

		// scan whole array
		for(  i=1;  i<size;  i++  ) {
			if(  data[i]==0  ) {
				next = i+1;
				return i;
			}
		}

		// no free entry found, extent array if possible
		uint16 newsize = (uint16)min( 65535ul, 2l*size );
		if(  size<newsize  ) {
			T ** newdata = new T* [newsize];
			memcpy( newdata, data, sizeof(T*)*size );
			for(  uint16 i=size;  i<newsize;  i++  ) {
				newdata[i] = 0;
			}
			delete [] data;
			data = newdata;
			next = size+1;
			size = newsize;
			return next-1;
		}

		// completely out of handles
		dbg->fatal("quickstone<T>::find_next()","no free index found (size=%i)",size);
		return 0; //dummy for compiler
	}

	/**
	 * The index in the table for this handle.
	 * (only this variable is actially saved, since the rest is static!)
	 */
	uint16 entry;

public:
	/**
	 * Initializes the tombstone table. Calling init() makes all existing
	 * quickstones invalid.
	 *
	 * @param n number of elements
	 * @author Hj. Malthaner
	 */
	static void init(const uint16 n)
	{
		if(data) {
			delete [] data;
		}
		size = n;
		data = new T* [size];

		// all NULL pointers are mapped to entry 0
		for(int i=0; i<size; i++) {
			data[i] = 0;
		}
		next = 1;
	}

	// empty handle (entry 0 is always zero)
	quickstone_tpl()
	{
		entry = 0;
	}

	// connects with free handle
	explicit quickstone_tpl(T* p)
	{
		if(p) {
			entry = find_next();
			data[entry] = p;
		}
		else {
			// all NULL pointers are mapped to entry 0
			entry = 0;
		}
	}

	quickstone_tpl(const quickstone_tpl& r) : entry(r.entry) {}

#if 0	// shoudl be unneccessary, since compiler handles this much faster
	/**
	 * Assignment operator. Adjusts counters if one handle is
	 * assigned ot another one.
	 *
	 * @author Hj. Malthaner
	 */
	quickstone_tpl operator=(const quickstone_tpl &other)
	{
		entry = other.entry;
		return *this;
	}
#endif

	// returns true, if no handles left
	static bool is_exhausted() {

		if(  size==65535  ) {
			// scan  array
			for(  uint16 i = 1; i<size; i++) {
				if(data[i] == 0) {
					// still empty handles left
					return false;
				}
			}
			// no handles left => cannot extent
			return true;
		}
		// can extent in any case => ok
		return false;
	}


	inline bool is_bound() const
	{
		return data[entry] != 0;
	}

	/**
	 * Removes the object from the tombstone table - this affects all
	 * handles to the object!
	 * @author Hj. Malthaner
	 */
	T* detach()
	{
		T* p = data[entry];
		data[entry] = 0;
		return p;
	}

	/**
	 * Danger - use with care.
	 * Useful to hand the underlying pointer to subsystems
	 * that don't know about quickstones - but take care that such pointers
	 * are never ever deleted or that by some means detach() is called
	 * upon deletion, i.e. from the ~T() destructor!!!
	 *
	 * @author Hj. Malthaner
	 */
	T* get_rep() const { return data[entry]; }

	/**
	 * @return the index into the tombstone table. May be used as
	 * an ID for the referenced object.
	 */
	uint16 get_id() const { return entry; }

	/**
	 * Overloaded dereference operator. With this, quickstones can
	 * be used as if they were pointers.
	 *
	 * @author Hj. Malthaner
	 */
	T* operator->() const { return data[entry]; }

	bool operator== (const quickstone_tpl<T> &other) const
	{
		return entry == other.entry;
	}

	bool operator!= (const quickstone_tpl<T> &other) const
	{
		return entry != other.entry;
	}

	static uint16 get_size() { return size; }
};

template <class T> T** quickstone_tpl<T>::data = 0;

template <class T> uint16 quickstone_tpl<T>::next = 1;
template <class T> uint16 quickstone_tpl<T>::size = 0;

#endif
