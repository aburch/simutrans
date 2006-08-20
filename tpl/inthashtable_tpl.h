/*
 * a template class which implements a hashtable with integer keys
 */

#ifndef inthashtable_tpl_h
#define inthashtable_tpl_h

#include "hashtable_tpl.h"


/*
 * Define the key characteristica for hashing integer types
 */
template<class key_t>
class inthash_tpl {
public:
    static unsigned int hash(const key_t key)
    {
	return (unsigned int)key;
    }
    static key_t null()
    {
	return 0;
    }
    static void dump(const key_t key)
    {
	printf("%d", (int)key);
    }
    static int comp(key_t key1, key_t key2)
    {
	return (int)key1 - (int)key2;
    }
};


/*
 * Ready to use class for hashing integer types. Note that key can be of any
 * integer type including enums.
 */
template<class key_t, class value_t>
class inthashtable_tpl : public hashtable_tpl<key_t, value_t, inthash_tpl<key_t> >
{
};


template<class key_t, class value_t>
class inthashtable_iterator_tpl : public hashtable_iterator_tpl<key_t, value_t, inthash_tpl<key_t> >
{
public:
    inthashtable_iterator_tpl(const hashtable_tpl<key_t, value_t, inthash_tpl<key_t> > *hashtable) :
	hashtable_iterator_tpl<key_t, value_t, inthash_tpl<key_t> >(hashtable)
    {
    }
    inthashtable_iterator_tpl(const hashtable_tpl<key_t, value_t, inthash_tpl<key_t> > &hashtable) :
	hashtable_iterator_tpl<key_t, value_t, inthash_tpl<key_t> >(hashtable)
    {
    }
};

#endif
