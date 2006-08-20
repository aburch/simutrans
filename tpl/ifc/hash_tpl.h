/*
 * hash_tpl.h
 *
 * Copyright (c) 1997 - 2002 V. Meyer
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef hash_tpl_h
#define hash_tpl_h


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//	template<class key_t> hash_tpl()
//
//---------------------------------------------------------------------------
//  Description:
//	This is the interface for a helper class to hashtable_tpl.
//	When implementing this interface, You define the used hash function
//	and some other stuff.
/////////////////////////////////////////////////////////////////////////////
//@EDOC
template<class key_t>
class hash_tpl {
public:
    //
    // Return the hash-value for a given key value
    //
    static int hash(const key_t key);
    //
    // Return a NULL key vlaue
    //
    static key_t null();
    //
    // Prints key to stdout
    //
    static void dump(const key_t key);
    //
    // Compares to keys. Return is interpreted as in strcmp().
    //
    static int comp(key_t key1, key_t key2) const;
};

#endif
