/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TPL_STRINGHASHTABLE_TPL_H
#define TPL_STRINGHASHTABLE_TPL_H


#include "hashtable_tpl.h"
#include <string.h>

/*
 * a template class which implements a hashtable with string keys
 */

/*
 * Define the key characteristics for hashing "const char *".
 */
class stringhash_t {
public:
	typedef int diff_type;

	static uint32 hash(const char *key)
	{
		uint32 hash = 0;
#if 1
		for(  sint8 i=16;  i*key[0]!=0;  i--  ) {
			hash += (uint8)(*key++);
		}
#else
		while (*key != '\0') {
			hash = hash * 33 + *key++;
		}
#endif
		return hash;
	}

	static diff_type comp(const char *key1, const char *key2)
	{
		return strcmp(key1, key2);
	}
};


/*
 * Ready to use class for hashing strings.
 */
template <class value_t>class stringhashtable_tpl :
	public hashtable_tpl<const char *, value_t, stringhash_t>
{
public:
	stringhashtable_tpl() : hashtable_tpl<const char *, value_t, stringhash_t>() {}
private:
	stringhashtable_tpl(const stringhashtable_tpl&);
	stringhashtable_tpl& operator=( stringhashtable_tpl const&);
};

#endif
