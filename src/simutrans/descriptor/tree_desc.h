/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_TREE_DESC_H
#define DESCRIPTOR_TREE_DESC_H


#include "../simtypes.h"
#include "obj_base_desc.h"
#include "image_array.h"

#include "../network/checksum.h"

/**
 * Tree type description in Simutrans
 *
 * Child nodes:
 *  0   Name
 *  1   Copyright
 *  2   Image-array
 *
 * season 0 is always summer
 * season 1 is winter for two seasons
 * otherwise 0 summer, next seasons (autumn, winter, spring) ....
 */
class tree_desc_t : public obj_named_desc_t {
	friend class tree_reader_t;

	climate_bits allowed_climates;
	uint8        distribution_weight;
	uint8        number_of_seasons;

public:
	uint16 get_distribution_weight() const { return distribution_weight; }

	bool is_allowed_climate( climate cl ) const { return ((1<<cl)&allowed_climates)!=0; }

	climate_bits get_allowed_climate_bits() const { return allowed_climates; }

	image_id get_image_id(uint8 season, uint16 i) const
	{
		if(number_of_seasons==0) {
			// compatibility mode
			i += season*5;
			season = 0;
		}
		return get_child<image_array_t>(2)->get_image(i, season)->get_id();
	}

	// old style trees and new style tree support ...
	uint8 get_seasons() const
	{
		if(number_of_seasons==0) {
			return get_child<image_array_t>(2)->get_count() / 5;
		}
		return number_of_seasons;
	}

	void calc_checksum(checksum_t *chk) const
	{
		chk->input((uint8)allowed_climates);
		chk->input(distribution_weight);
	}
};

#endif
