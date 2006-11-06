/*
 *
 *  grund_besch.h
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
#ifndef __GRUND_BESCH_H
#define __GRUND_BESCH_H

/*
 *  includes
 */
#include "obj_besch_std_name.h"
#include "bildliste2d_besch.h"
#include "../simtypes.h"
#include "../dataobj/ribi.h"

/*
 *  class:
 *      grund_besch_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Verschiedene Untergründe - viellcht bald weisse Berge?
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bildliste2D
 */

class karte_t;

class grund_besch_t : public obj_besch_std_name_t {
	friend class ground_writer_t;

private:
	static karte_t *welt;

	static image_id grund_besch_t::image_offset;

public:
	// only these textures need external access
	static const grund_besch_t *grund_besch_t::fundament;
	static const grund_besch_t *grund_besch_t::slopes;
	static const grund_besch_t *grund_besch_t::fences;
	static const grund_besch_t *grund_besch_t::marker;
	static const grund_besch_t *grund_besch_t::borders;
	static const grund_besch_t *grund_besch_t::ausserhalb;

	static const char *get_climate_name_from_bit( enum climate n );

#ifdef DOUBLE_GROUNDS
    static const uint8 slopetable[80];
    // returns the correct hang number for this slope
    static inline int get_double_hang(hang_t::typ typ) {
		return slopetable[typ];
    }
#endif

	// returns the pointer to an image structure
	const bild_besch_t *gib_bild_ptr(int typ) const
	{
		const bildliste_besch_t *liste = static_cast<const bildliste2d_besch_t *>(gib_kind(2))->gib_liste(typ);
		if(liste && liste->gib_anzahl() > 0) {
			const bild_besch_t *bild = static_cast<const bildliste2d_besch_t *>(gib_kind(2))->gib_bild(typ,0);
			return bild;
		}
		return NULL;
	}

	// image for all non-climate stuff like foundations ...
	image_id gib_bild(int typ) const
	{
		const bildliste_besch_t *liste = static_cast<const bildliste2d_besch_t *>(gib_kind(2))->gib_liste(typ);
		if(liste && liste->gib_anzahl() > 0) {
			const bild_besch_t *bild = static_cast<const bildliste2d_besch_t *>(gib_kind(2))->gib_bild(typ,0);
			if(bild) {
				return bild->bild_nr;
			}
		}
		return IMG_LEER;
	}

	// image for all ground tiles
	static image_id gib_ground_tile(hang_t::typ slope, sint16 height );

	static bool register_besch(const grund_besch_t *besch);

	static bool alles_geladen();

	/* this routine is called during the creation of a new map
	 * it will recalculate all transitions according the given water level
	 * and put the result in height_to_climate
	 */
	static void calc_water_level(karte_t *welt, uint8 *height_to_climate);

};

#endif // __GRUND_BESCH_H
