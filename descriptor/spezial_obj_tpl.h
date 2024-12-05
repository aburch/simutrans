/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_SPEZIAL_OBJ_TPL_H
#define DESCRIPTOR_SPEZIAL_OBJ_TPL_H


#include <string.h>
#include <typeinfo>
#include "../simdebug.h"

/*
 * Routines to manage special object descriptors lists used in the program.
 */

/**
 * Descriptors of required objects. The following functions manage
 * the list. The list is "{NULL, NULL}" terminated.
 */
template<class desc_t> struct special_obj_tpl {
	const desc_t** desc;
	const char* name;
};


/**
 * An object pointer is set on the passed list, if the name of the
 * object belongs to one of the objects mentioned in the list.
 * @param so List to operate over.
 * @parem desc Descriptor to add.
 */
template<class desc_t> bool register_desc(special_obj_tpl<desc_t> const* so, desc_t const* const desc)
{
	for (; so->name; ++so) {
		if (strcmp(so->name, desc->get_name()) == 0) {
			if (*so->desc != NULL  ) {
				// these doublettes are harmless, and hence only recored at debug level 3
//				dbg->doubled( "object", desc->get_name() );
				dbg->message("register_desc()", "Notice: obj %s already defined", so->name);
			}
			*so->desc = desc;
			return true;
		}
	}
	return false;
}


/**
 * Verifies the passed list for all objects to be not NULL, ie are loaded.
 * @param so List to check.
 */
template<class desc_t> bool successfully_loaded(special_obj_tpl<desc_t> const* so)
{
	for (; so->name; ++so) {
		if (!*so->desc) {
			dbg->fatal("successfully_loaded()", "%s-object %s not found.\n*** PLEASE INSTALL PROPER BASE FILE AND CHECK PATH ***", typeid(**so->desc).name(), so->name);
			return false;
		}
	}
	return true;
}

/**
 * Shows debug messages showing which descriptors lack definition.
 * @param so List to check.
 */
template<class desc_t> void warn_missing_objects(special_obj_tpl<desc_t> const* so)
{
	for (; so->name; ++so) {
		if (!*so->desc) {
			dbg->message("warn_missing_objects", "Object %s not found, feature disabled", so->name);
		}
	}
}

#endif
