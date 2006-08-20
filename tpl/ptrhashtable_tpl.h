/*
 * ptrhashtable_tpl.h
 *
 * Copyright (c) 1997 - 2002 V. Meyer
 *
 * a template class which implements a hashtable with pointer keys
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef ptrhashtable_tpl_h
#define ptrhashtable_tpl_h

#include "hashtable_tpl.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//	template<class key_t> ptrhash_tpl()
//
//---------------------------------------------------------------------------
//  Description:
//	Define the key characteristica for hashing pointers. For hashing the
//	direct value is used.
/////////////////////////////////////////////////////////////////////////////
//@EDOC
template<class key_t>
class ptrhash_tpl {
public:
    static int hash(const key_t key)
    {
	return ((int)key) & 0x7FFFFFFF;
    }
    static key_t null()
    {
	return NULL;
    }
    static void dump(const key_t key)
    {
	printf("%p", (void *)key);
    }
    static int comp(key_t key1, key_t key2)
    {
	return (int)key1 - (int)key2;
    }
};


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//	ptrhashtable_tpl()
//
//---------------------------------------------------------------------------
//  Description:
//	Ready to use class for hashing pointers.
/////////////////////////////////////////////////////////////////////////////
//@EDOC
template<class key_t, class value_t>
class ptrhashtable_tpl : public hashtable_tpl<key_t, value_t, ptrhash_tpl<key_t> >
{
};

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//      ptrhashtable_iterator_tpl()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
template<class key_t, class value_t>
class ptrhashtable_iterator_tpl : public hashtable_iterator_tpl<key_t, value_t, ptrhash_tpl<key_t> >
{
public:
    ptrhashtable_iterator_tpl(const hashtable_tpl<key_t, value_t, ptrhash_tpl<key_t> > *hashtable) :
	hashtable_iterator_tpl<key_t, value_t, ptrhash_tpl<key_t> >(hashtable)
    {
    }
    ptrhashtable_iterator_tpl(const hashtable_tpl<key_t, value_t, ptrhash_tpl<key_t> > &hashtable) :
	hashtable_iterator_tpl<key_t, value_t, ptrhash_tpl<key_t> >(hashtable)
    {
    }
};

#endif
