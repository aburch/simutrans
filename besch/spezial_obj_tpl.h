/*
 * Routines to manage special object descriptors (beschs) lists used in the program.
 */

#ifndef __SPEZIAL_OBJ_TPL_H
#define __SPEZIAL_OBJ_TPL_H

#include <string.h>
#include <typeinfo>
#include "../simdebug.h"


/**
 * Descriptors (beschs) of required objects. The following functions manage
 * the list. The list is "{NULL, NULL}" terminated.
 */
template<class besch_t> struct spezial_obj_tpl {
	const besch_t** besch;
	const char* name;
};


/**
 * An object pointer is set on the passed list, if the name of the
 * object belongs to one of the objects mentioned in the list.
 * @param so List to operate over.
 * @parem besch Descriptor to add.
 */
template<class besch_t> bool register_besch(spezial_obj_tpl<besch_t> const* so, besch_t const* const besch)
{
	for (; so->name; ++so) {
		if (strcmp(so->name, besch->get_name()) == 0) {
			if (*so->besch != NULL) {
				dbg->message("register_besch()", "Notice: obj %s already defined", so->name);
			}
			*so->besch = besch;
			return true;
		}
	}
	return false;
}


/**
 * Verifies the passed list for all objects to be not NULL, ie are loaded.
 * @param so List to check.
 */
template<class besch_t> bool alles_geladen(spezial_obj_tpl<besch_t> const* so)
{
	for (; so->name; ++so) {
		if (!*so->besch) {
			dbg->fatal("alles_geladen()", "%s-object %s not found.\n*** PLEASE INSTALL PROPER BASE FILE AND CHECK PATH ***", typeid(**so->besch).name(), so->name);
			return false;
		}
	}
	return true;
}

/**
 * Shows debug messages showing which descriptors lack definition.
 * @param so List to check.
 * @param count Number of elements to check.
 */
template<class besch_t> void warne_ungeladene(spezial_obj_tpl<besch_t> const* so)
{
	for (; so->name; ++so) {
		if (!*so->besch) {
			dbg->message("warne_ungeladene", "Object %s not found, feature disabled", so->name);
		}
	}
}

#endif
