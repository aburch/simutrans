/*
 * handle_tpl.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef tpl_handle_tpl_h
#define tpl_handle_tpl_h

// define this to use key_lock error checking
// may inly be used with guarded_alloc and guarded_free
#define USE_KEYLOCK


/**
 * a template class for reference counting. Supports automatic
 * deallocation of objects if all references are removed.
 *
 * @date March 2001
 * @author Hj. Malthaner
 */
template<class T> class handle_tpl
{
private:

    T * rep;
    int * count;

    int check;

public:

    /**
     * Overloaded dereference operator. With this, handles can used as if
     * they were pointers.
     *
     * @author Hj. Malthaner
     */
    T* operator->() {
#ifdef USE_KEYLOCK
	if(check != *( ((int *)rep)-1 )) {
	    printf("Error: handle_tpl::operator->(): check != sig, check=%d, sig=%d\n", check, *( ((int *)rep)-1 ));
	}
#endif

	return rep;
    }


    /**
     * Basic constructor. Constructs a new handle.
     *
     * @author Hj. Malthaner
     */
    handle_tpl(T * p) : rep(p), count(new int(1))
    {
#ifdef USE_KEYLOCK
	check = *( ((int *)p)-1 );
#endif

	printf("1 ref\n");
    }


    /**
     * Copy constructor. Constructs a new handle from another one.
     *
     * @author Hj. Malthaner
     */
    handle_tpl(const handle_tpl& r) : rep(r.rep), count(r.count)
    {
	(*count) ++;
	check = r.check;

	printf("%d refs\n", *count);
    }


    /**
     * Assignment operator. Adjusts counters if one handle is
     * assigned ot another one.
     *
     * @author Hj. Malthaner
     */
    handle_tpl& operator=(const handle_tpl &r)
    {
	// same object?
	if(rep == r.rep) {
	    return *this;
	}

	// different Object, one ref less
	if(--(*count) == 0) {
	    delete rep;
	    delete count;
	    printf("Last ref assigned to other object, deleting object\n");
	}

	// add ref
	rep = r.rep;
	count = r.count;
	(*count)++;

	printf("inc: %d refs\n", *count);

	return *this;
    }


    /**
     * Destructor. Deletes object if this was the last handle.
     *
     * @author Hj. Malthaner
     */
    ~handle_tpl()
    {
	if(--(*count) == 0) {
	    delete rep;
	    delete count;

	    printf("Last ref destructed, deleting object\n");
	} else {
	    printf("dec: %d refs\n", *count);
	}
    }


    /**
     * Conversion operator. Withthis handles can be converted to superclass
     * handles automatically.
     *
     * @author Hj. Malthaner
     */
    template<class T2> handle_tpl<T>::operator handle_tpl<T2> ()
    {
	return handle_tpl<T2> (rep);
    }
};

#endif
