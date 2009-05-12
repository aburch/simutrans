/*
 * a template class which implements a hashtable with 2d koord keys
 */

#ifndef koordhashtable_tpl_h
#define koordhashtable_tpl_h

#include "hashtable_tpl.h"
#include "../dataobj/koord.h"


/*
 * Define the key characteristics for hashing 2d koord types
 */
template<class key_t>
class koordhash_tpl 
{
public:
    static uint32 hash(const key_t key)
    {
		uint32 hash = key.y << 16;
		hash |= key.x;
		return hash;
    }

	static key_t null()
    {
		return koord::invalid;
    }

	static void dump(const key_t key)
    {
		printf("%d, %d", (koord)key.x, (koord)key.y);
    }

	static bool comp(key_t key1, key_t key2)
    {
		return key1 != key2;
    }
};


/*
 * Ready to use class for hashing 2d koord types. 
 */
template<class key_t, class value_t>
class koordhashtable_tpl : public hashtable_tpl<key_t, value_t, koordhash_tpl<key_t> >
{
};


template<class key_t, class value_t>
class koordhashtable_iterator_tpl : public hashtable_iterator_tpl<key_t, value_t, koordhash_tpl<key_t> >
{
public:
    koordhashtable_iterator_tpl(const hashtable_tpl<key_t, value_t, koordhash_tpl<key_t> > *hashtable) :
	hashtable_iterator_tpl<key_t, value_t, koordhash_tpl<key_t> >(hashtable)
    {
    }
    koordhashtable_iterator_tpl(const hashtable_tpl<key_t, value_t, koordhash_tpl<key_t> > &hashtable) :
	hashtable_iterator_tpl<key_t, value_t, koordhash_tpl<key_t> >(hashtable)
    {
    }
};

#endif
