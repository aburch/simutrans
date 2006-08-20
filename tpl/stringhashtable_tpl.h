/*
 * stringhashtable_tpl.h
 *
 * Copyright (c) 1997 - 2002 Hj.Malthaner / V. Meyer
 *
 * a template class which implements a hashtable with string keys
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef stringhashtable_tpl_h
#define stringhashtable_tpl_h

#include <string.h>

#include "hashtable_tpl.h"


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//	stringhash_t()
//
//---------------------------------------------------------------------------
//  Description:
//	Define the key characteristica for hashing "const char *".
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class stringhash_t {
public:
    static int hash(const char *key)
    {
	if(key[0] == 0) {
	    return 0;
	} else {
	    return ( ((unsigned char)key[0]) + (((unsigned char)key[1])<<8));
	}
    }
    static const char *null()
    {
	return NULL;
    }
    static void dump(const char *key)
    {
	printf("%s", key);
    }
    static int comp(const char *key1, const char *key2)
    {
	return strcmp(key1, key2);
    }
};

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//	stringhash2_t()
//
//---------------------------------------------------------------------------
//  Description:
//	Define the key characteristica for hashing "const char *".
// 	Alternate implementation with whole string for hashcode.
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
class stringhash2_t {
public:
    static int hash(const char *key)
    {
	int code = 0;
	while (*key)
		code = (code<<5) + code + *key++;
	return code;
    }
    static const char *null()
    {
	return NULL;
    }
    static void dump(const char *key)
    {
	printf("%s", key);
    }
    static int comp(const char *key1, const char *key2)
    {
	return strcmp(key1, key2);
    }
};

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//	template <class T> stringhashtable_tpl()
//
//---------------------------------------------------------------------------
//  Description:
//	Ready to use class for hashing strings. Hashkey is calculated from
//	first and second char.
/////////////////////////////////////////////////////////////////////////////
//@EDOC
template <class value_t>
class stringhashtable_tpl : public hashtable_tpl<const char *, value_t, stringhash_t>
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
template<class value_t>
class stringhashtable_iterator_tpl : public hashtable_iterator_tpl<const char *, value_t, stringhash_t>
{
public:
    stringhashtable_iterator_tpl(const hashtable_tpl<const char *, value_t, stringhash_t> *hashtable) :
	hashtable_iterator_tpl<const char *, value_t, stringhash_t>(hashtable)
    {
    }
    stringhashtable_iterator_tpl(const hashtable_tpl<const char *, value_t, stringhash_t> &hashtable) :
	hashtable_iterator_tpl<const char *, value_t, stringhash_t>(hashtable)
    {
    }
};

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  class:
//	template <class T> stringhashtable2_tpl()
//
//---------------------------------------------------------------------------
//  Description:
//	Ready to use class for hashing strings. Hashkey is calculated from
//	while string.
/////////////////////////////////////////////////////////////////////////////
//@EDOC
template <class value_t>
class stringhashtable2_tpl : public hashtable_tpl<const char *, value_t, stringhash2_t>
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
template<class value_t>
class stringhashtable2_iterator_tpl : public hashtable_iterator_tpl<const char *, value_t, stringhash2_t>
{
public:
    stringhashtable2_iterator_tpl(const hashtable_tpl<const char *, value_t, stringhash2_t> *hashtable) :
	hashtable_iterator_tpl<const char *, value_t, stringhash2_t>(hashtable)
    {
    }
    stringhashtable2_iterator_tpl(const hashtable_tpl<const char *, value_t, stringhash2_t> &hashtable) :
	hashtable_iterator_tpl<const char *, value_t, stringhash2_t>(hashtable)
    {
    }
};

#endif
