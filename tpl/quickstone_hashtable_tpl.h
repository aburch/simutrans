/*
 * a template class which implements a hashtable with quickstone keys
 * adapted from the pointer hashtable by jamespetts
 */

#ifndef quickstone_hashtable_tpl_h
#define quickstone_hashtable_tpl_h

#include "ptrhashtable_tpl.h"
#include "hashtable_tpl.h"
#include "quickstone_tpl.h"
#include <stdlib.h>

/*
 * Define the key characteristics for hashing pointers. For hashing the
 * direct value is used.
 */
template<class key_t>
class quickstone_hash_tpl {
public:
	static uint32 hash(const quickstone_tpl<key_t> key)
	{
		return (uint32)(size_t)key.get_rep();
	}

	static key_t null()
	{
		return NULL;
	}

	static void dump(const quickstone_tpl<key_t> key)
	{
		printf("%p", (void *)key.get_rep());
	}

	static long comp(quickstone_tpl<key_t> key1, quickstone_tpl<key_t> key2)
	{
		return (long)((size_t)key1.get_rep() - (size_t)key2.get_rep());
	}
};


/*
 * Ready to use class for hashing pointers.
 */
template<class key_t, class value_t>
class quickstone_hashtable_tpl : public hashtable_tpl<quickstone_tpl<key_t>, value_t, quickstone_hash_tpl<key_t> >
{
};


template<class key_t, class value_t>
class quickstone_hashtable_iterator_tpl : public hashtable_iterator_tpl<quickstone_tpl<key_t>, value_t, quickstone_hash_tpl<key_t> >
{
public:
	quickstone_hashtable_iterator_tpl(const hashtable_tpl<quickstone_tpl<key_t>, value_t, quickstone_hash_tpl<key_t> > *hashtable) :
	hashtable_iterator_tpl<quickstone_tpl<key_t>, value_t, ptrhash_tpl<quickstone_tpl<key_t> > >(hashtable)
	{ }

	quickstone_hashtable_iterator_tpl(const hashtable_tpl<quickstone_tpl<key_t>, value_t, quickstone_hash_tpl<key_t> > &hashtable) :
	hashtable_iterator_tpl<quickstone_tpl<key_t>, value_t, quickstone_hash_tpl<key_t> >(hashtable)
	{ }
};

#endif
