/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __SKIN_BESCH_H
#define __SKIN_BESCH_H

#include "../simimg.h"
#include "obj_besch_std_name.h"
#include "bildliste2d_besch.h"


/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Skin ist im wesentlichen erstmal eine Bildliste.
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bildliste
 */
class skin_besch_t : public obj_besch_std_name_t {
	friend class skin_writer_t;

public:
	const bild_besch_t *get_bild(int i) const  { return static_cast<const bildliste_besch_t *>(get_kind(2))->get_bild(i); }

	int get_bild_anzahl() const { return static_cast<const bildliste_besch_t *>(get_kind(2))->get_anzahl(); }

	image_id get_bild_nr(int i) const
	{
		const bild_besch_t *bild = get_bild(i);
		return bild != NULL ? bild->get_nummer() : IMG_LEER;
	}
};

#endif
