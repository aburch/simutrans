/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TPL_KOORDHASHTABLE_TPL_H
#define TPL_KOORDHASHTABLE_TPL_H


#include "hashtable_tpl.h"
#include "../dataobj/koord.h"


/*
 * Define the key characteristics for hashing 2d koord types
 */
template<class key_t>
class koordhash_tpl
{
public:
	typedef int diff_type;

    static uint32 hash(const key_t key)
    {
		return (uint32)key.y << 16 | key.x;
    }

	static key_t null()
    {
		return koord::invalid;
    }

	static void dump(const key_t key)
    {
		printf("%d, %d", key.x, key.y);
    }

	static diff_type comp(key_t key1, key_t key2)
    {
		diff_type d = key1.y - key2.y;
		if (!d)	d = key1.x - key2.x;
		return d;
    }
};


/*
 * Ready to use class for hashing 2d koord types.
 */
template<class key_t, class value_t, size_t n_bags>
class koordhashtable_tpl : public hashtable_tpl<key_t, value_t, koordhash_tpl<key_t>, n_bags>
{
};


#endif
