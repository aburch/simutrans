/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __BILDLISTE_BESCH_H
#define __BILDLISTE_BESCH_H

#include "bild_besch.h"

/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Beschreibung eines eindimensionalen Arrays von Bildern.
 *
 *  Kindknoten:
 *	0   1. Bild
 *	1   2. Bild
 *	... ...
 */
class bildliste_besch_t : public obj_besch_t {
    friend class imagelist_reader_t;
    friend class imagelist_writer_t;

    uint16  anzahl;

public:
	bildliste_besch_t() : anzahl(0) {}

	uint16 get_anzahl() const { return anzahl; }

	bild_besch_t const* get_bild(uint16 i) const { return i < anzahl ? get_child<bild_besch_t>(i) : 0; }

	image_id get_bild_nr(uint16 i) const {
		const bild_besch_t *bild = get_bild(i);
		return bild != NULL ? bild->get_nummer() : IMG_LEER;
	}
};

#endif
