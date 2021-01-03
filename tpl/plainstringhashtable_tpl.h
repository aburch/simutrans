/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TPL_PLAINSTRINGHASHTABLE_TPL_H
#define TPL_PLAINSTRINGHASHTABLE_TPL_H


#include "stringhashtable_tpl.h"
#include "../utils/plainstring.h"

/**
 * Hashtable with plainstring keys.
 *
 * Keys will be copied when creating entries,
 * unlike stringhashtable_tpl
 */
template <class value_t, size_t n_bags>class plainstringhashtable_tpl :
public hashtable_tpl<plainstring, value_t, stringhash_t, n_bags>
{
public:
	plainstringhashtable_tpl() : hashtable_tpl<plainstring, value_t, stringhash_t, n_bags>() {}
private:
	plainstringhashtable_tpl(const plainstringhashtable_tpl&);
	plainstringhashtable_tpl& operator=( plainstringhashtable_tpl const&);
};

#endif
