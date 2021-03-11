/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TPL_QUICKSTONE_TPL_H
#define TPL_QUICKSTONE_TPL_H


#include "../simtypes.h"
#include "../simdebug.h"

/**
 * An implementation of the tombstone pointer checking method.
 * It uses a table of pointers and indices into that table to
 * implement the tombstone system. Unlike real tombstones, this
 * template reuses entries from the tombstone table, but it tries
 * to leave freed tombstones untouched as long as possible, to
 * detect most of the dangling pointers.
 *
 * This templates goal is to be efficient and fairly safe.
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
	 * The index in the table for this handle.
	 * (only this variable is actually saved, since the rest is static!)
	 */
	uint16 entry;

private:
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
		return enlarge();
	}

	static uint16 enlarge()
	{
		// no free entry found, extend array if possible
		uint16 newsize;
		if (size == 65535) {
			// completely out of handles
			dbg->fatal("quickstone<T>::find_next()","no free index found (size=%i)",size);
			return 0; //dummy for compiler
		} else if (size >= 32768) {
			// max out on handles, don't overflow uint16
			newsize = 65535;
		} else {
			newsize = 2*size;
		}

		// Move data to new extended array
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

public:
	/**
	 * Initializes the tombstone table. Calling init() makes all existing
	 * quickstones invalid.
	 *
	 * @param n number of elements
	 */
	static void init(const uint16 n)
	{
		delete [] data;
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

	// connects with last handle
	explicit quickstone_tpl(T* p, bool)
	{
		uint16 i;

		// scan rest of array
		for(  i=size-1;  i>0;  i++  ) {
			if(  data[i] == 0  ) {
				entry = i;
				data[entry] = p;
				return;
			}
		}
		enlarge();
		// repeat
		for(  i=size-1;  i>0;  i++  ) {
			if(  data[i] == 0  ) {
				entry = i;
				data[entry] = p;
				return;
			}
		}
		dbg->fatal( "quickstone_tpl(bool)", "No more handles!\nShould have already failed with enlarge!" );
	}

	// creates handle with id, fails if already taken
	quickstone_tpl(T* p, uint16 id)
	{
		if(p) {
			if(  id == 0  ) {
				dbg->fatal("quickstone<T>::quickstone_tpl(T*,uint16)","wants to assign non-null pointer to null index");
			}
			while(  id >= size  ) {
				enlarge();
			}
			if(  data[id]!=NULL  &&  data[id]!=p  ) {
				dbg->fatal("quickstone<T>::quickstone_tpl(T*,uint16)","slot (%d) already taken", id);
			}
			entry = id;
			data[entry] = p;
		}
		else {
			if(  id!=0  ) {
				dbg->fatal("quickstone<T>::quickstone_tpl(T*,uint16)","wants to assign null pointer to non-null index");
			}
			// all NULL pointers are mapped to entry 0
			entry = 0;
		}
	}

	// returns true, if no handles left
	static bool is_exhausted()
	{
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
	 */
	T* get_rep() const { return data[entry]; }

	/**
	 * @return the index into the tombstone table. May be used as
	 * an ID for the referenced object.
	 */
	uint16 get_id() const { return entry; }

	/**
	 * Sets the current id: Needed to recreate stuff via network.
	 * ATTENTION: This may be harmful. DO not use unless really really needed!
	 */
	void set_id(uint16 e) { entry=e; }

	/**
	 * Overloaded dereference operator. With this, quickstones can
	 * be used as if they were pointers.
	 */
	T* operator->() const { return data[entry]; }

	T& operator *() const { return *data[entry]; }

	bool operator== (const quickstone_tpl<T> &other) const { return entry == other.entry; }

	bool operator!= (const quickstone_tpl<T> &other) const { return entry != other.entry; }

	static uint16 get_size() { return size; }

	/**
	 * For checking the consistency of handle allocation
	 * among the server and the clients in network mode
	 */
	static uint16 get_next_check() { return next; }
};

template <class T> T** quickstone_tpl<T>::data = 0;

template <class T> uint16 quickstone_tpl<T>::next = 1;
template <class T> uint16 quickstone_tpl<T>::size = 0;

#endif
