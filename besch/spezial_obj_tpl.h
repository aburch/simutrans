/*
 * Hilfstemplates, um Beschreibungen, die das Programm direkt benötigt, zu
 * initialisieren.
 */

#ifndef __SPEZIAL_OBJ_TPL_H
#define __SPEZIAL_OBJ_TPL_H

#include <string.h>
#include <typeinfo>
#include "../simdebug.h"


/*
 * Beschreibung eines erforderlichen Objekts. Für die nachfolgenden Funktionen
 * wird eine Liste dieser Beschreibungen angelegt, wobei die Liste mit einem
 * "{NULL, NULL}" Eintrag terminiert wird.
 */
template<class besch_t> struct spezial_obj_tpl {
	const besch_t** besch;
	const char* name;
};


/*
 * Ein Objektzeiger wird anhand der übergebenen Liste gesetzt, falls der Name
 * des Objektes einem der in der Liste erwähnten Objekte gehört.
 */
template<class besch_t> bool register_besch(spezial_obj_tpl<besch_t>* so, const besch_t* besch)
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


/*
 * Überprüft die übergebene Liste, ob alle Objekte ungleich NULL, sprich
 * geladen sind.
 */
template<class besch_t> bool alles_geladen(spezial_obj_tpl<besch_t>* so)
{
	for (; so->name; ++so) {
		if (!*so->besch) {
			dbg->fatal("alles_geladen()", "%s-object %s not found.\n*** PLEASE INSTALL PROPER BASE FILE AND CHECK PATH ***", typeid(**so->besch).name(), so->name);
			return false;
		}
	}
	return true;
}


template<class besch_t> void warne_ungeladene(spezial_obj_tpl<besch_t>* so, int count)
{
	for (; count-- && so->name; ++so) {
		if (!*so->besch) {
			dbg->message("warne_ungeladene", "Object %s not found, feature disabled", so->name);
		}
	}
}

#endif
