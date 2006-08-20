/*
 *
 *  bildliste2d_besch.h
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
#ifndef __BILDLISTE2D_BESCH_H
#define __BILDLISTE2D_BESCH_H

/*
 *  includes
 */

#include "bildliste_besch.h"

/*
 *  class:
 *      bildliste2d_besch_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      2 dimensionales Bilder-Array
 *
 *  Kindknoten:
 *	0   1. Bildliste
 *	1   2. Bildliste
 *	... ...
 */
class bildliste2d_besch_t : public obj_besch_t {
    friend class imagelist2d_writer_t;

    uint16  anzahl;

public:
    bildliste2d_besch_t() : anzahl(0) {}

    int gib_anzahl() const
    {
	return anzahl;
    }
    const bild_besch_t *gib_bild(int i, int j) const
    {
	return i >= 0 && i < anzahl ?
	    static_cast<const bildliste_besch_t *>(gib_kind(i))->gib_bild(j) : 0;
    }
    const bildliste_besch_t *gib_liste(int i) const
    {
	return i >= 0 && i < anzahl ?
	    static_cast<const bildliste_besch_t *>(gib_kind(i)) : 0;
    }
};

#endif // __BILDLISTE2D_BESCH_H
