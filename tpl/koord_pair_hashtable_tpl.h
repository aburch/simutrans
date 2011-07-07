/*
 * a template class which implements a hashtable with 2d koord keys
 */

#ifndef koord_pair_hashtable_tpl_h
#define koord_pair_hashtable_tpl_h

#include "hashtable_tpl.h"
#include "../dataobj/koord.h"


/*
 * Define the key characteristics for hashing 2d koord types
 */
template<class key_t>
class koord_pair_hash_tpl 
{
public:
    static uint32 hash(const key_t key)
    {
		uint32 hash_first = key.first.y << 16;
		hash_first |= key.first.x;

		uint32 hash_second = key.second.y << 16;
		hash_second |= key.second.x;

		uint32 hash_final = hash_first << 16;
		hash_final |= hash_second;

		return hash_final;
    }

	static key_t null()
    {
		return koord_pair();
    }

	static void dump(const key_t key)
    {
		printf("%d, %d; %d, %d", (koord)key.first.x, (koord)key.first.y, (koord)key.second.x, (koord)key.second.y);
    }

	static bool comp(key_t key1, key_t key2)
    {
		return key1 != key2;
    }
};


/*
 * Ready to use class for hashing pairs of 2d koord types. 
 */
template<class key_t, class value_t>
class koord_pair_hashtable_tpl : public hashtable_tpl<key_t, value_t, koord_pair_hash_tpl<key_t> >
{
};


template<class key_t, class value_t>
class koord_pair_hashtable_iterator_tpl : public hashtable_iterator_tpl<key_t, value_t, koord_pair_hash_tpl<key_t> >
{
public:
    koord_pair_hashtable_iterator_tpl(const hashtable_tpl<key_t, value_t, koord_pair_hash_tpl<key_t> > *hashtable) :
	hashtable_iterator_tpl<key_t, value_t, koord_pair_hash_tpl<key_t> >(hashtable)
    {
    }
    koord_pair_hashtable_iterator_tpl(const hashtable_tpl<key_t, value_t, koord_pair_hash_tpl<key_t> > &hashtable) :
	hashtable_iterator_tpl<key_t, value_t, koord_pair_hash_tpl<key_t> >(hashtable)
    {
    }
};

#endif
