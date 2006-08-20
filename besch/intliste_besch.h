/*
 *
 *  intliste_besch.h
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
#ifndef __INTLISTE_BESCH_H
#define __INTLISTE_BESCH_H

/*
 *  includes
 */
#include "obj_besch.h"

/*
 *  class:
 *      intliste_besch_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      1 dimensionales Array von integer
 *
 *  Kindknoten:
 *	(keine)
 */
class intliste_besch_t : public obj_besch_t {
    friend class intlist_writer_t;

    uint16  anzahl;

public:
    intliste_besch_t() : anzahl(0) {}

    int gib_anzahl() const
    {
	return anzahl;
    }
    const int gib_int(int i) const
    {
	return i >= 0 && i < anzahl ? (&anzahl)[i + 1] : 0;
    }
};

#endif // __INTLISTE_BESCH_H
