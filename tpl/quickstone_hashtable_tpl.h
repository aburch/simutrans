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
 * Define the key characteristics for hashing IDs.
 */
template<class key_t>
class quickstone_hash_tpl {
public:
	static uint16 hash(const quickstone_tpl<key_t> key)
	{
		return key.get_id();
	}

	static key_t null()
	{
		return NULL;
	}

	static void dump(const quickstone_tpl<key_t> key)
	{
		printf("%p", (void *)key.get_id());
	}

	static long comp(quickstone_tpl<key_t> key1, quickstone_tpl<key_t> key2)
	{
		return (long)(key1.get_id() - key2.get_id());
	}
};


/*
 * Ready to use class for hashing quickstones.
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
	hashtable_iterator_tpl<quickstone_tpl<key_t>, value_t, inthash_tpl<quickstone_tpl<key_t> > >(hashtable)
	{ }

	quickstone_hashtable_iterator_tpl(const hashtable_tpl<quickstone_tpl<key_t>, value_t, quickstone_hash_tpl<key_t> > &hashtable) :
	hashtable_iterator_tpl<quickstone_tpl<key_t>, value_t, quickstone_hash_tpl<key_t> >(hashtable)
	{ }
};

#endif
