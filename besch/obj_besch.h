/*
 *
 *  obj_besch.h
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */
#ifndef __OBJ_BESCH_H
#define __OBJ_BESCH_H

/*
 *  includes
 */
#include "../simtypes.h"

/*
 *  forward declarations
 */
class obj_besch_t;

/*
 *  struct:
 *      node_besch_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Internal Node information - the derived class knows,
 *	    how many node child nodes really exist.
 */
typedef obj_besch_t* obj_besch_info_t;

/*
 *  class:
 *      obj_besch_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Basis aller besch_t-Klassen, d.h. Beschreibungen, die aus den .pak
 *	Files geladen werden.
 *	Keine virtuellen Methoden erlaubt !
 */
class obj_besch_t {

 protected:
    obj_besch_t *gib_kind(int i) const { return node_info[i]; }

 public:

    /**
     * Hajo 11-Oct-03: I made this public to allow reader_t subclasses
     * to access the field easily. I recommend noone but reader_t
     * subclasses should write this field!
     */
    obj_besch_info_t *node_info;
};

#endif // __OBJ_BESCH_H
