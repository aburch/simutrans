#ifndef plainstringhashtable_tpl_h
#define plainstringhashtable_tpl_h

#include "stringhashtable_tpl.h"
#include "../utils/plainstring.h"

/**
 * Hashtable with plainstring keys.
 *
 * Keys will be copied when creating entries,
 * unlike stringhashtable_tpl
 */
template <class value_t>class plainstringhashtable_tpl :
public hashtable_tpl<plainstring, value_t, stringhash_t>
{
public:
	plainstringhashtable_tpl() : hashtable_tpl<plainstring, value_t, stringhash_t>() {}
private:
	plainstringhashtable_tpl(const plainstringhashtable_tpl&);
	plainstringhashtable_tpl& operator=( plainstringhashtable_tpl const&);
};

#endif
