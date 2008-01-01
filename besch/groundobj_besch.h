/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 * 2004-2008 by Markus Pristovsek
 *
 */
#ifndef __GROUNDOBJ_BESCH_H
#define __GROUNDOBJ_BESCH_H

#include "../simtypes.h"
#include "obj_besch_std_name.h"
#include "bildliste2d_besch.h"

/*
 *  Autor:
 *      Markus Pristovsek
 *
 * this is the description for ground objects like small lakes, hunting post, birds, flower patch, stones, sheeps, ...
 * these can either reside on a tile (moving=0)
 * or move around the map (water_t=only on water, air_t=everywhere)
 * They are removable with certain costs.
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bildliste2D
 */


class groundobj_besch_t : public obj_besch_std_name_t {
	friend class groundobj_writer_t;
	friend class groundobj_reader_t;

	climate_bits	allowed_climates;
	uint16 distribution_weight;
	uint8  number_of_seasons;
	uint16  speed;
	bool  trees_on_top;
	waytype_t waytype;
	sint32 cost_removal;

public:
	uint16 gib_distribution_weight() const { return distribution_weight; }

	bool is_allowed_climate( climate cl ) const { return ((1<<cl)&allowed_climates)!=0; }

	const bild_besch_t *gib_bild(int season, int phase) const  	{
		return static_cast<const bildliste2d_besch_t *>(gib_kind(2))->gib_bild(phase, season);
	}

	// moving stuff should have eight
	// otherwise up to 16 for all slopes are ok
	// if anzahl==1, this will not appear on slopes
	const uint16 gib_phases() const {
		return static_cast<const bildliste2d_besch_t *>(gib_kind(2))->gib_anzahl();
	}

	const int gib_seasons() const { return number_of_seasons; }

	const bool get_speed() const { return speed; }

	const bool can_built_trees_here() const { return trees_on_top; }

	const bool get_waytype() const { return waytype; }

	const sint32 gib_preis() const { return cost_removal; }
};

#endif
