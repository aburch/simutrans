/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_GROUNDOBJ_DESC_H
#define DESCRIPTOR_GROUNDOBJ_DESC_H


#include "../simtypes.h"
#include "obj_base_desc.h"
#include "image_array.h"
#include "../network/checksum.h"

/**
 * This is the description for ground objects like small lakes, hunting post, birds, flower patch, stones, sheeps, ...
 * these can either reside on a tile (moving=0)
 * or move around the map (water_t=only on water, air_t=everywhere)
 * They are removable with certain costs.
 *
 * Child nodes:
 *  0   Name
 *  1   Copyright
 *  2   Image-array
 */
class groundobj_desc_t : public obj_named_desc_t
{
	friend class groundobj_reader_t;
	friend class groundobj_t;
	friend class movingobj_t;

	climate_bits allowed_climates;
	uint16       distribution_weight;
	uint8        number_of_seasons;
	sint32       speed;
	uint16       index;
	bool         trees_on_top;
	waytype_t    wtyp;
	sint32       price;

public:
	uint16 get_distribution_weight() const { return distribution_weight; }

	bool is_allowed_climate( climate cl ) const { return ((1<<cl)&allowed_climates)!=0; }

	// the right house for this area?
	bool is_allowed_climate_bits( climate_bits cl ) const { return (cl&allowed_climates)!=0; }

	// needed by place_finder
	climate_bits get_allowed_climate_bits() const { return allowed_climates; }

	const image_t *get_image(uint8 season, uint16 phase) const {
		return get_child<image_array_t>(2)->get_image(phase, season);
	}

	image_id get_image_id(uint8 season, uint16 phase) const {
		const image_t *image = get_child<image_array_t>(2)->get_image(phase, season);
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}

	// moving stuff should have eight
	// otherwise up to 16 for all slopes are ok
	// if count==1, this will not appear on slopes
	uint16 get_phases() const
	{
		return get_child<image_array_t>(2)->get_count();
	}

	uint8 get_seasons() const { return number_of_seasons; }

	sint32 get_speed() const { return speed; }

	bool can_build_trees_here() const { return trees_on_top; }

	waytype_t get_waytype() const { return wtyp; }

	sint32 get_price() const { return price; }

	uint16 get_index() const { return index; }

	void calc_checksum(checksum_t *chk) const
	{
		chk->input((uint8)allowed_climates);
		chk->input(distribution_weight);
		chk->input(number_of_seasons);
		chk->input(speed);
		chk->input(trees_on_top);
		chk->input((uint8)wtyp);
		chk->input(price);
	}
};

#endif
