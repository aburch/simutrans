/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __BAUM_BESCH_H
#define __BAUM_BESCH_H

#include "../simtypes.h"
#include "obj_besch_std_name.h"
#include "bildliste2d_besch.h"

/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Beschreibung eines Baumes in Simutrans
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bildliste2D
 */

 // season 0 is always summer
 // season 1 is winter for two seasons
 // otherwise 0 summer, next seasons (autumn, winter, spring) ....

class baum_besch_t : public obj_besch_std_name_t {
	friend class tree_writer_t;
	friend class tree_reader_t;

	climate_bits	allowed_climates;
	uint8		distribution_weight;
	uint8		number_of_seasons;

public:
	uint16 get_distribution_weight() const { return distribution_weight; }

	bool is_allowed_climate( climate cl ) const { return ((1<<cl)&allowed_climates)!=0; }

	climate_bits get_allowed_climate_bits() const { return allowed_climates; }

	image_id get_bild_nr(int season, int i) const
	{
		if(number_of_seasons==0) {
			// comapility mode
			i += season*5;
			season = 0;
		}
		return static_cast<const bildliste2d_besch_t *>(get_child(2))->get_bild(i, season)->get_nummer();
	}

	// old style trees and new style tree support ...
	int get_seasons() const
	{
		if(number_of_seasons==0) {
			return static_cast<const bildliste2d_besch_t *>(get_child(2))->get_anzahl()/5;
		}
		return number_of_seasons;
	}
};

#endif
