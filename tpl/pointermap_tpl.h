/*
 * pointermap_tpl.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */
#ifndef __POINTERMAP_TPL_H
#define __POINTERMAP_TPL_H

#include "slist_tpl.h"


/**
 * template <class key_t, class value_t> gib_key_obj:
 *
 * In case the user provides a member "gib_key_obj()", this
 * function template automatically generates the connecting
 * function. See comment below
 *
 * @author V. Meyer
 */
template <class key_t, class value_t>
const key_t *gib_key_obj(const value_t *value)
{
    return value->gib_key_obj();
}


/**
 * template <class key_t, class value_t> pointermap_tpl:
 *
 * Implements a simple map as a linked list between two
 * pointer types.
 * The value is stored in the list. keys are available via
 * "gib_key_obj(value)".
 *
 * Caution: You have to provide either a public function
 *	"const key_t *gib_key_obj(const value_t *value)"
 * or a public member of class value_t
 *	"key_t *gib_key_obj() const",
 * caues otherwise the get() method does not compile.
 *
 * @author V. Meyer
 */
template <class key_t, class value_t>
class pointermap_tpl : private slist_tpl<value_t *> {
public:
    void put(value_t *value)
    {
	insert(value);
    }
    value_t *get(const key_t *key) const
    {
	slist_iterator_tpl<value_t *> iter(this);

        while(iter.next()) {
	    value_t *value = iter.get_current();
	    if(gib_key_obj<key_t, value_t>(value) == key) {
		return iter.get_current();
	    }
        }
	return NULL;
    }
    bool remove(value_t *value)
    {
	return slist_tpl<value_t *>::remove(value);
    }
};

#endif // __POINTERMAP_TPL_H
