/*
 * a template class which implements a hashtable with pointer keys
 */

#ifndef ptrhashtable_tpl_h
#define ptrhashtable_tpl_h

#include "hashtable_tpl.h"
#include <stdlib.h>

/*
 * Define the key characteristica for hashing pointers. For hashing the
 * direct value is used.
 */
template<class key_t>
class ptrhash_tpl {
public:
	typedef ptrdiff_t diff_type;

	static uint32 hash(const key_t key)
	{
		return (uint32)(size_t)key;
	}

	static void dump(const key_t key)
	{
		printf("%p", (void *)key);
	}

	static diff_type comp(key_t key1, key_t key2)
	{
		return key1 - key2;
	}
};


/*
 * Ready to use class for hashing pointers.
 */
template<class key_t, class value_t>
class ptrhashtable_tpl : public hashtable_tpl<key_t, value_t, ptrhash_tpl<key_t> >
{
public:
	ptrhashtable_tpl() : hashtable_tpl<key_t, value_t, ptrhash_tpl<key_t> >() {}
private:
	ptrhashtable_tpl(const ptrhashtable_tpl&);
	ptrhashtable_tpl& operator=( ptrhashtable_tpl const&);
};

#endif
