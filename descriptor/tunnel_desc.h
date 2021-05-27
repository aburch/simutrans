/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_TUNNEL_DESC_H
#define DESCRIPTOR_TUNNEL_DESC_H


#include "../display/simimg.h"
#include "../simtypes.h"
#include "obj_base_desc.h"
#include "skin_desc.h"
#include "image_array.h"
#include "way_desc.h"

/*
 * node structure:
 *  0   Name
 *  1   Copyright
 *  2   Image-list Background
 *  3   Image-list Foreground
 *  4   cursor(image 0) and icon (image 1)
 *[ 5   Image-list Background - snow ] (if present)
 *[ 6   Image-list Foreground - snow ] (if present)
 *[ 7 (or 5 if no snow image) underground way ] (if present)
 */
class tunnel_desc_t : public obj_desc_transport_infrastructure_t {
	friend class tunnel_reader_t;
	friend class tunnel_builder_t; // to convert the old tunnels to new ones

private:
	static int slope_indices[81];

	/* number of seasons (0 = none, 1 = no snow/snow
	 */
	sint8 number_of_seasons;

	/* has underground way image ? ( 0 = no, 1 = yes ) [way parameter]
	 */
	uint8 has_way;

	/* Has broad portals?
	 */
	uint8 broad_portals;

public:
	const image_t *get_background(slope_t::type slope, uint8 season, uint8 type ) const
	{
		const uint8 n = season && number_of_seasons == 1 ? 5 : 2;
		return get_child<image_list_t>(n)->get_image(slope_indices[slope] + 4 * type);
	}

	image_id get_background_id(slope_t::type slope, uint8 season, uint8 type ) const
	{
		const image_t *desc = get_background(slope, season, type );
		return desc != NULL ? desc->get_id() : IMG_EMPTY;
	}

	const image_t *get_foreground(slope_t::type slope, uint8 season, uint8 type ) const
	{
		const uint8 n = season && number_of_seasons == 1 ? 6 : 3;
		return get_child<image_list_t>(n)->get_image(slope_indices[slope] + 4 * type);
	}

	image_id get_foreground_id(slope_t::type slope, uint8 season, uint8 type) const
	{
		const image_t *desc = get_foreground(slope, season, type );
		return desc != NULL ? desc->get_id() :IMG_EMPTY;
	}

	skin_desc_t const* get_cursor() const { return get_child<skin_desc_t>(4); }

	waytype_t get_finance_waytype() const;

	const way_desc_t *get_way_desc() const
	{
		if(has_way) {
			return get_child<way_desc_t>(5 + number_of_seasons * 2);
		}
		return NULL;
	}

	bool has_broad_portals() const { return (broad_portals != 0); }
};

#endif
