/*
 * id_handle_tpl.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef tpl_id_handle_tpl
#define tpl_id_handle_tpl

#include <typeinfo>

#include "debug_helper.h"
#include "../tpl/inthashtable_tpl.h"

/* forward declarations */
template<class T> class handle_as_refcount_tombstones_tpl;
template<class T> class handle_as_id_tombstones_tpl;

/**
 * handle_unchecked_tpl:
 *
 * Handle implementation with unchecked handles.
 * This may be used if we are sure, no errors occur.
 *
 * @author V. Meyer
 */
template <class T> class handle_unchecked_tpl {
    T *obj;
public:
    long get_id() const { return (long)obj; }

    bool is_bound() const { return obj != NULL; }

    T* operator->() const { return obj; }

    bool operator== (const handle_unchecked_tpl<T> & handle) const
    {
	return (obj == handle.obj);
    }
    bool operator!= (const handle_unchecked_tpl<T> & handle) const
    {
        return (obj != handle.obj);
    }

    handle_unchecked_tpl& operator=(const handle_unchecked_tpl &r)
    {
	obj = r.obj;
        return *this;
    }
    handle_unchecked_tpl() { obj = NULL; }

    handle_unchecked_tpl(const handle_unchecked_tpl& r) { obj = r.obj; }

    ~handle_unchecked_tpl() { unbind(); }

    void unbind() { obj = NULL; }

    void attach(T *obj) { this->obj = obj; }

    T* detach() {
        T* obj = this->obj;
        this->obj = NULL;
        return obj;
    }
};



/**
 * handle_as_id_tpl:
 *
 * Handle implementation with tombstone - mostly the old
 * id_handle_tpl/tombstones_tpl from Hajo.
 * Indirections are checked, but we do not do reference counting.
 *
 * @author Hj. Malthaner/V. Meyer
 */
template <class T> class handle_as_id_tpl {
private:
    /**
     * Tombstone table. Public member for ease of use and efficiency.
     * @author Hj. Malthaner
     */
    static handle_as_id_tombstones_tpl<T> tombstones;

    /**
     * The tombstone id for unbound handles.
     * @author Hj. Malthaner
     */
    enum { UNBOUND = -1 };

    /**
     * The tombstone id. Only positive tombstone id's are valid.
     * a negative tombstone id denotes an unallocated object.
     * @author Hj. Malthaner
     */
    long tombstone_id;

   /**
    * Retrieve a pointer to the bound object, or NULL if there is no object
    * bound to this handle anymore.
    * @author Hj. Malthaner
    */
    const T* retrieve_bound_object() const {
	const T* p = NULL;

	if(tombstone_id >= 0) {
	    p = tombstones.get(tombstone_id);

	    if(p == NULL) {
		ERROR("handle_as_id_tpl<T>::retrieve_bound_object()",
		    "the %s bound to this %s-handle with tombstone id %ld is no longer available!",
		    typeid(p).name(), typeid(T).name(), tombstone_id);
	    }
	} else if(tombstone_id == UNBOUND) {
	    ERROR("handle_as_id_tpl<T>::retrieve_bound_object()",
		"there is no object bound to this %s-handle!",
		typeid(T).name());
	} else {
	    ERROR("handle_as_id_tpl<T>::retrieve_bound_object()",
		"a %s-handle has the invalid tombstone id %ld!",
		typeid(T).name(), tombstone_id);
	}
	return p;
    }
public:

    /**
     * Accessor method for ID.
     * @author Hj. Malthaner
     */
    long get_id() const {return tombstone_id;};

    /**
     * Checks if this handle is valid and bound to an object.
     * @author Hj. Malthaner
     */
    bool is_bound() const {return tombstone_id >= 0;};

    /**
     * Checks if this handle is valid and bound to an object
     * and if the object is still available (alive).
     * @author Hj. Malthaner
     */
    bool deep_check() const {
	return (retrieve_bound_object() != NULL);
    }

    /**
     * Dereference operator. Check all known and checkable error conditions<br>
     * 1) no object bound<br>
     * 2) bound object free'd<br>
     * @author Hj. Malthaner
     */
    T* operator->() const {
    	return const_cast<T *>(retrieve_bound_object());
    }


    bool operator== (const handle_as_id_tpl<T> & handle) const {
        return (tombstone_id == handle.tombstone_id);
    };


    bool operator!= (const handle_as_id_tpl<T> & handle) const {
        return (tombstone_id != handle.tombstone_id);
    };


    handle_as_id_tpl& operator=(const handle_as_id_tpl &r)
    {
	tombstone_id = r.tombstone_id;
	return *this;
    }

    /**
     * Allocate a handle without an object.
     * @author Hj. Malthaner
     */
    handle_as_id_tpl()
    {
        tombstone_id = UNBOUND;
    }

    /**
     * Allocate a handle for T object with tombstone id id.
     * @author Hj. Malthaner
     */
    handle_as_id_tpl(long id)
    {
        tombstone_id = id;
    }

    /**
     * Copy constructor. Usually called frequently.
     * @author Hj. Malthaner
     */
    handle_as_id_tpl(const handle_as_id_tpl& r)
    {
        tombstone_id = r.tombstone_id;
    }

    ~handle_as_id_tpl()
    {
        unbind();
    }

    /**
     * Unbind handle from object.
     * @author Hj. Malthaner
     */
    void unbind()
    {
	tombstone_id = UNBOUND;
    }

    void attach(T *obj)
    {
        tombstone_id = tombstones.put(obj);
    }

    T* detach()
    {
	T* p = tombstones.remove(tombstone_id);

//	printf("detach %p -> %ld\n", p, tombstone_id);

	unbind();
	return p;
    }
};

/**
 * Helper template to make a tombstone table without reference counting.
 *
 * @author Hj. Malthaner/V. Meyer
 */
template<class T> class handle_as_id_tombstones_tpl {
private:
    long index;

    inthashtable_tpl<long, T *> table;

public:

    handle_as_id_tombstones_tpl() {
	index = 0;
    }


    /**
     * Gets an object from the table.
     * @param the tombstone id
     * @return the object
     * @author Hj. Malthaner
     */
    T* get(long id) {
	return table.get(id);
    }


    /**
     * Puts an object into the table.
     * @return the tombstone id
     * @author Hj. Malthaner
     */
    long put(T* p) {
	const long tmp = index;

	table.put(index, p);
	index = (index + 1) & 0x7FFFFFFF;

	return tmp;
    }


    /**
     * Removes an object from the table.
     * @param the tombstone id
     * @return the object
     * @author Hj. Malthaner
     */
    T* remove(long id) {
	T* p = get(id);

	table.remove(id);

	return p;
    }
};

template<class T> handle_as_id_tombstones_tpl<T> handle_as_id_tpl<T>::tombstones;


/**
 * handle_as_refcount_tpl:
 *
 * Handle implementation with tombstone and refcounting
 * Slowest, but safest implementation
 *
 * @author V. Meyer
 */
template<class T> class handle_as_refcount_tpl {
private:
    /**
     * Tombstone table. Public member for ease of use and efficiency.
     * @author Hj. Malthaner
     */
    static handle_as_refcount_tombstones_tpl<T> tombstones;

    /**
     * The tombstone id for unbound handles.
     * @author Hj. Malthaner
     */
    enum { UNBOUND = -1 };

    /**
     * The tombstone id. Only positive tombstone id's are valid.
     * a negative tombstone id denotes an unallocated object.
     * @author Hj. Malthaner
     */
    long tombstone_id;

   /**
    * Retrieve a pointer to the bound object, or NULL if there is no object
    * bound to this handle anymore.
    * @author Hj. Malthaner
    */
    T* retrieve_bound_object() const {
	T* p = NULL;

	if(tombstone_id >= 0) {
	    p = tombstones.get(*this);

	    if(p == NULL) {
		ERROR("handle_as_refcount_tpl<T>::retrieve_bound_object()",
		    "the %s bound to this %s-handle with tombstone id %ld is no longer available!",
		    typeid(p).name(), typeid(T).name(), tombstone_id);
	    }
	} else if(tombstone_id == UNBOUND) {
	    ERROR("handle_as_refcount_tpl<T>::retrieve_bound_object()",
		"there is no object bound to this %s-handle!",
		typeid(T).name());
	} else {
	    ERROR("handle_as_refcount_tpl<T>::retrieve_bound_object()",
		"a %s-handle has the invalid tombstone id %ld!",
		typeid(T).name(), tombstone_id);
	}
	return p;
    }
public:

    /**
     * Accessor method for ID.
     * @author Hj. Malthaner
     */
    long get_id() const {return tombstone_id;};

    /**
     * Checks if this handle is valid and bound to an object.
     * @author Hj. Malthaner
     */
    bool is_bound() const {return tombstone_id >= 0;};

    /**
     * Checks if this handle is valid and bound to an object
     * and if the object is still available (alive).
     * @author Hj. Malthaner
     */
    bool deep_check() const {
	return (retrieve_bound_object() != NULL);
    }

    /**
     * Dereference operator. Check all known and checkable error conditions<br>
     * 1) no object bound<br>
     * 2) bound object free'd<br>
     * @author Hj. Malthaner
     */
    T* operator->() const {
    	return retrieve_bound_object();
    }


    bool operator== (const handle_as_refcount_tpl<T> & handle) const {
        return (tombstone_id == handle.tombstone_id);
    };


    bool operator!= (const handle_as_refcount_tpl<T> & handle) const {
        return (tombstone_id != handle.tombstone_id);
    };


    handle_as_refcount_tpl& operator=(const handle_as_refcount_tpl &r)
    {
	tombstones.remove_ref(tombstone_id);
	tombstone_id = r.tombstone_id;
	tombstones.add_ref(tombstone_id);
    // printf("Message: %s-handle for %ld assigned\n", typeid(T).name(), tombstone_id);
    return *this;
  }


    /**
     * Allocate a handle without an object.
     * @author Hj. Malthaner
     */
    handle_as_refcount_tpl()
    {
        tombstone_id = UNBOUND;
    }

    /**
     * Allocate a handle for T object with tombstone id id.
     * @author Hj. Malthaner
     */
    handle_as_refcount_tpl(long id)
    {
        tombstone_id = id;
        tombstones.add_ref(tombstone_id);
    }

    /**
     * Copy constructor. Usually called frequently.
     * @author Hj. Malthaner
     */
    handle_as_refcount_tpl(const handle_as_refcount_tpl& r)
    {
        tombstone_id = r.tombstone_id;
        tombstones.add_ref(tombstone_id);
    }

    ~handle_as_refcount_tpl()
    {
        unbind();
    }

    /**
     * Unbind handle from object.
     * @author Hj. Malthaner
     */
    void unbind()
    {
        tombstones.remove_ref(tombstone_id);
        tombstone_id = UNBOUND;
    }
    void attach(T *obj)
    {
        unbind();
        *this = tombstones.put(obj);
    }
    T* detach()
    {
        return tombstones.remove(*this);
    }
};

/**
 * Helper template to make a tombstone table.
 *
 * @author Hj. Malthaner
 */
template<class T> class handle_as_refcount_tombstones_tpl {
private:
    long index;

    /**
     *  If we count handles, T* and the count have to be stored in each
     *  hashtable entry.
     *
     * @author V. Meyer
     */
    class entry {
    public:
        T *obj;
        int refs;

        entry(T *_obj) : obj(_obj), refs(0) { }
        ~entry()
        {
            if(refs > 0) {
		dbg->error("handle_as_refcount_tombstones_tpl::entry::~entry()", "%s-object refcount greater than 0 (%d)",
                    typeid(T).name(), refs);
            }
        }
    };
    inthashtable_tpl<long, entry> table;

public:
    /**
     * Gets an object from the table.
     * @param the tombstone id
     * @return the object
     * @author Hj. Malthaner
     */
    T* get(handle_as_refcount_tpl<T> handle) {
	entry e = table.get(handle.get_id());

        return e.obj;
    }

    /**
     * Puts an object into the table.
     * @return the tombstone id
     * @author Hj. Malthaner
     */
    long put(T* p) {
	const long tmp = index;
	entry e(p);

	table.put(index, e);
	index = (index + 1) & 0x7FFFFFFF;

	return tmp;
    }

    /**
     * Removes an object from the table.
     * @param the tombstone id
     * @return the object
     * @author Hj. Malthaner
     */
    T* remove(handle_as_refcount_tpl<T> &handle) {

        entry *e = table.get(handle.get_id());

        if(!e) {
            dbg->error("handle_as_refcount_tombstones_tpl::remove", "%s-handle invalid (%d)",
                typeid(T).name(), handle.get_id());
            return NULL;
        }
        T *p = e ? e->obj : NULL;
        e->obj = NULL;

        if(e->refs == 1) {
            table.remove(handle.get_id());
        }
	return p;
    }

    void remove_ref(long tombstone_id)
    {
        if(tombstone_id >= 0) {
            entry *e = table.get(tombstone_id);

            if(!e) {
                dbg->warning("handle_as_refcount_tombstones_tpl::remove_ref", "%s-handle invalid (%ld)",
                    typeid(T).name(), tombstone_id);
                return;
            }
            e->refs--;
            if(e->refs == 0) {
                if(e->obj) {
                    dbg->warning("handle_as_refcount_tombstones_tpl::remove_ref", "deleting last %s-handle %ld, leaving object %p unreferenced",
                        typeid(T).name(), tombstone_id, e->obj);
		}
                table.remove(tombstone_id);
            }
        }
    }
    void add_ref(long tombstone_id)
    {
        if(tombstone_id >= 0) {
            entry *e = table.get(tombstone_id);

            if(!e) {
                dbg->warning("handle_as_refcount_tombstones_tpl::add_ref", "%s-handle invalid (%ld)",
                    typeid(T).name(), tombstone_id);
                return;
            }
            e->refs++;
        }
    }
};

template<class T> handle_as_refcount_tombstones_tpl<T> handle_as_refcount_tpl<T>::tombstones;



#endif
