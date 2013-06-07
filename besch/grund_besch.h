/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __GRUND_BESCH_H
#define __GRUND_BESCH_H

#include "obj_besch_std_name.h"
#include "bildliste2d_besch.h"
#include "../simtypes.h"
#include "../dataobj/ribi.h"
#include "../boden/grund.h"

/*
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
private:
	static karte_t *welt;

	static image_id image_offset;

public:
	static uint16 water_animation_stages;
	static sint16 water_depth_levels;

	// only these textures need external access
	static const grund_besch_t *fundament;
	static const grund_besch_t *slopes;
	static const grund_besch_t *fences;
	static const grund_besch_t *marker;
	static const grund_besch_t *borders;
	static const grund_besch_t *sea;	// different water depth
	static const grund_besch_t *ausserhalb;

	static char const* get_climate_name_from_bit(climate n);

	static bool double_grounds;

	static const uint8 slopetable[80];
	// returns the correct hang number for this slope
	static inline int get_double_hang(hang_t::typ typ) {
		return slopetable[typ];
	}

	// returns the pointer to an image structure
	const bild_besch_t *get_bild_ptr(int typ, int stage=0) const
	{
		bildliste2d_besch_t const* const bl2   = get_child<bildliste2d_besch_t>(2);
		bildliste_besch_t   const* const liste = bl2->get_liste(typ);
		if(liste && liste->get_anzahl() > 0) {
			bild_besch_t const* const bild = bl2->get_bild(typ, stage);
			return bild;
		}
		return NULL;
	}

	// image for all non-climate stuff like foundations ...
	image_id get_bild(int typ, int stage=0) const
	{
		bild_besch_t const* const bild = get_bild_ptr(typ, stage);
		return bild ? bild->get_nummer() : IMG_LEER;
	}

	// image for all ground tiles
	static image_id get_ground_tile(grund_t *gr);

	static image_id get_water_tile(hang_t::typ slope);
	static image_id get_climate_tile(climate cl, hang_t::typ slope);
	static image_id get_snow_tile(hang_t::typ slope);
	static image_id get_beach_tile(hang_t::typ slope, uint8 corners);
	static image_id get_alpha_tile(hang_t::typ slope);
	static image_id get_alpha_tile(hang_t::typ slope, uint8 corners);

	static bool register_besch(const grund_besch_t *besch);

	static bool alles_geladen();

	/**
	 * Generates ground texture images, transition maps, etc.
	 */
	static void init_ground_textures(karte_t *welt);

	static image_id get_marker_image(hang_t::typ slope_in, bool background)
	{
		uint8 slope = double_grounds ? slope_in : slopetable[slope_in];
		uint8 index = background ? (double_grounds ? (slope % 3) + 3 * ((uint8)(slope / 9)) + 27
		                                           : ((slope & 1) + ((slope >> 1) & 6) + 8))
		                         : (double_grounds ?  slope % 27
		                                           : (slope & 7 ));
		return marker->get_bild(index);
	}

	static image_id get_border_image(hang_t::typ slope_in)
	{
		uint8 slope = double_grounds ? slope_in : slopetable[slope_in];
		uint8 index = double_grounds ? (slope % 3) + 3 * ((uint8)(slope / 9)) : (slope & 1) + ((slope >> 1) & 6);
		return borders->get_bild(index);
	}
};

#endif
