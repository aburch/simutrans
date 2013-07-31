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
 *	2   Image list
 */
class skin_besch_t : public obj_besch_std_name_t {
public:
	bild_besch_t const* get_bild(int i) const { return get_child<bildliste_besch_t>(2)->get_bild(i); }

	int get_bild_anzahl() const { return get_child<bildliste_besch_t>(2)->get_anzahl(); }

	image_id get_bild_nr(int i) const
	{
		const bild_besch_t *bild = get_bild(i);
		return bild != NULL ? bild->get_nummer() : IMG_LEER;
	}
};

#endif
