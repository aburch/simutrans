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
#include "../network/checksum.h"

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
	friend class groundobj_reader_t;
	friend class groundobj_t;
	friend class movingobj_t;

	climate_bits allowed_climates;
	uint16 distribution_weight;
	uint8  number_of_seasons;
	sint32  speed;
	uint16 index;
	bool  trees_on_top;
	waytype_t waytype;
	sint32 cost_removal;

public:
	uint16 get_distribution_weight() const { return distribution_weight; }

	bool is_allowed_climate( climate cl ) const { return ((1<<cl)&allowed_climates)!=0; }

	const bild_besch_t *get_bild(int season, int phase) const  	{
		return get_child<bildliste2d_besch_t>(2)->get_bild(phase, season);
	}

	// moving stuff should have eight
	// otherwise up to 16 for all slopes are ok
	// if anzahl==1, this will not appear on slopes
	uint16 get_phases() const
	{
		return get_child<bildliste2d_besch_t>(2)->get_anzahl();
	}

	uint8 get_seasons() const { return number_of_seasons; }

	sint32 get_speed() const { return speed; }

	bool can_built_trees_here() const { return trees_on_top; }

	waytype_t get_waytype() const { return waytype; }

	sint32 get_preis() const { return cost_removal; }

	uint16 get_index() const { return index; }

	void calc_checksum(checksum_t *chk) const
	{
		chk->input((uint8)allowed_climates);
		chk->input(distribution_weight);
		chk->input(number_of_seasons);
		chk->input(speed);
		chk->input(index);
		chk->input(trees_on_top);
		chk->input((uint8)waytype);
		chk->input(cost_removal);
	}
};

#endif
