/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TPL_INTHASHTABLE_TPL_H
#define TPL_INTHASHTABLE_TPL_H


#include "hashtable_tpl.h"

/**
 * Define type for differences of integers.
 * Default is int.
 * Obvious specializations for 64bit integers.
 */
template<class inttype> struct int_diff_type {
	typedef int diff_type;
};
template<> struct int_diff_type<sint64> {
	typedef sint64 diff_type;
};
template<> struct int_diff_type<uint64> {
	typedef sint64 diff_type;
};


/**
 * Define the key characteristics for hashing integer types
 */
template<class key_t>
class inthash_tpl {
public:
	typedef typename int_diff_type<key_t>::diff_type diff_type;

	static uint32 hash(const key_t key)
	{
		return (uint32)key;
	}

	static diff_type comp(key_t key1, key_t key2)
	{
		return key1 - key2;
	}
};


/*
 * Ready to use class for hashing integer types. Note that key can be of any
 * integer type including enums.
 */
template<class key_t, class value_t>
class inthashtable_tpl : public hashtable_tpl<key_t, value_t, inthash_tpl<key_t> >
{
public:
	inthashtable_tpl() : hashtable_tpl<key_t, value_t, inthash_tpl<key_t> >() {}
private:
	inthashtable_tpl(const inthashtable_tpl&);
	inthashtable_tpl& operator=( inthashtable_tpl const&);
};

#endif
