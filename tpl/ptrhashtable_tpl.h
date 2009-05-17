/*
 * a template class which implements a hashtable with pointer keys
 */

#ifndef ptrhashtable_tpl_h
#define ptrhashtable_tpl_h

#include "hashtable_tpl.h"
#include <stdlib.h>

/*
 * Define the key characteristics for hashing pointers. For hashing the
 * direct value is used.
 */
template<class key_t>
class ptrhash_tpl {
public:
	static uint32 hash(const key_t key)
	{
		return (uint32)(size_t)key;
	}

	static key_t null()
	{
		return NULL;
	}

	static void dump(const key_t key)
	{
		printf("%p", (void *)key);
	}

	static long comp(key_t key1, key_t key2)
	{
		return (long)((size_t)key1 - (size_t)key2);
	}
};


/*
 * Ready to use class for hashing pointers.
 */
template<class key_t, class value_t>
class ptrhashtable_tpl : public hashtable_tpl<key_t, value_t, ptrhash_tpl<key_t> >
{
};


template<class key_t, class value_t>
class ptrhashtable_iterator_tpl : public hashtable_iterator_tpl<key_t, value_t, ptrhash_tpl<key_t> >
{
public:
	ptrhashtable_iterator_tpl(const hashtable_tpl<key_t, value_t, ptrhash_tpl<key_t> > *hashtable) :
	hashtable_iterator_tpl<key_t, value_t, ptrhash_tpl<key_t> >(hashtable)
	{ }

	ptrhashtable_iterator_tpl(const hashtable_tpl<key_t, value_t, ptrhash_tpl<key_t> > &hashtable) :
	hashtable_iterator_tpl<key_t, value_t, ptrhash_tpl<key_t> >(hashtable)
	{ }
};

#endif
