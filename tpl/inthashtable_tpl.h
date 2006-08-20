/*
 * inthashtable_tpl.h
 *
 * Copyright (c) 1997 - 2002 Hj.Malthaner / V. Meyer
 *
 * a template class which implements a hashtable with integer keys
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef inthashtable_tpl_h
#define inthashtable_tpl_h

#include "hashtable_tpl.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//	template<class key_t> class inthash_tpl:
//
//---------------------------------------------------------------------------
//  Description:
//	Define the key characteristica for hashing integer types
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
template<class key_t>
class inthash_tpl {
public:
    static unsigned int hash(const key_t key)
    {
	return (unsigned int)key;
    }
    static key_t null()
    {
	return 0;
    }
    static void dump(const key_t key)
    {
	printf("%d", (int)key);
    }
    static int comp(key_t key1, key_t key2)
    {
	return (int)key1 - (int)key2;
    }
};


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//	template<class key_t, class value_t> inthashtable_tpl
//
//---------------------------------------------------------------------------
//  Description:
//	Ready to use class for hashing integer types. Note that key can be of
//	any integer type including enums.
/////////////////////////////////////////////////////////////////////////////
//@EDOC
template<class key_t, class value_t>
class inthashtable_tpl : public hashtable_tpl<key_t, value_t, inthash_tpl<key_t> >
{
};

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      inthashtable_iterator_tpl()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
template<class key_t, class value_t>
class inthashtable_iterator_tpl : public hashtable_iterator_tpl<key_t, value_t, inthash_tpl<key_t> >
{
public:
    inthashtable_iterator_tpl(const hashtable_tpl<key_t, value_t, inthash_tpl<key_t> > *hashtable) :
	hashtable_iterator_tpl<key_t, value_t, inthash_tpl<key_t> >(hashtable)
    {
    }
    inthashtable_iterator_tpl(const hashtable_tpl<key_t, value_t, inthash_tpl<key_t> > &hashtable) :
	hashtable_iterator_tpl<key_t, value_t, inthash_tpl<key_t> >(hashtable)
    {
    }
};

#endif
