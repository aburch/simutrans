/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 *  node structure:
 *  0   Name
 *  1   Copyright
 *  2   Image-list Background
 *  3   Image-list Foreground
 *  4   cursor(image 0) and icon (image 1)
 *[ 5   Image-list Background - snow ] (if present)
 *[ 6   Image-list Foreground - snow ] (if present)
 *[ 7 (or 5 if no snow image) underground way ] (if present)
 */

#ifndef __TUNNEL_BESCH_H
#define __TUNNEL_BESCH_H

#include "../display/simimg.h"
#include "../simtypes.h"
#include "../dataobj/way_constraints.h"
#include "obj_base_desc.h"
#include "skin_desc.h"
#include "image_array.h"
#include "way_desc.h"


class tunnel_desc_t : public obj_desc_transport_infrastructure_t {
	friend class tunnel_reader_t;
	friend class tunnel_builder_t;	// to convert the old tunnels to new ones

private:
	static int slope_indices[81];

	/* number of seasons (0 = none, 1 = no snow/snow)
	*/
	sint8 number_of_seasons;

	/*Way constraints for, e.g., loading gauges, types of electrification, etc.
	* @author: jamespetts*/
	way_constraints_of_way_t way_constraints;

	/* has underground way image ? (0 = no, 1 = old type way, 2 = underground tunnel images, 3 = both)  [way parameter]
	*/
	uint8 has_way;

	/* Has broad portals?
	 */
	uint8 broad_portals;

public:

	/* 
	* Nodes:
	* Data (0), 
	* way (1), 
	* back portal images (normal) (2),
	* front portal images (normal) (3), 
	* cursor images (4), 
	* back portal images (snow) (5),
	* front portal images (snow) (6), 
	* back underground flat (7), 
	* back underground slope (8),
	* back underground diagonal (9), 
	* front underground flat (10), 
	* front underground slope (11), 
	* front underground diagonal (12). 
	*/
	
	const image_t *get_background(slope_t::type slope, uint8 season, uint8 type ) const
	{
		int const n = season && number_of_seasons == 1 ? 5 : 2;
		return get_child<image_list_t>(n)->get_image(slope_indices[slope] + 4 * type);
	}

	image_id get_background_id(slope_t::type slope, uint8 season, uint8 type ) const
	{
		const image_t *desc = get_background(slope, season, type );
		return desc != NULL ? desc->get_id() : IMG_EMPTY;
	}

	const image_t *get_foreground(slope_t::type slope, uint8 season, uint8 type ) const
	{
		int const n = season && number_of_seasons == 1 ? 6 : 3;
		return get_child<image_list_t>(n)->get_image(slope_indices[slope] + 4 * type);
	}

	image_id get_foreground_id(slope_t::type slope, uint8 season, uint8 type) const
	{
		const image_t *desc = get_foreground(slope, season, type );
		return desc != NULL ? desc->get_id() : IMG_EMPTY;
	}

	image_id get_underground_background_id(ribi_t::ribi ribi, slope_t::type slope) const
	{
		return slope == slope_t::flat ? get_child<image_list_t>(7)->get_image_id(ribi) : get_slope_image_nr(slope, false);
	}

	image_id get_underground_foreground_id(ribi_t::ribi ribi, slope_t::type slope) const
	{
		return slope == slope_t::flat ? get_child<image_list_t>(10)->get_image_id(ribi) : get_slope_image_nr(slope, true);
	}

	image_id get_diagonal_image_id(ribi_t::ribi ribi, bool front) const
	{
		const uint16 n = front ? 12 : 9;
		return get_child<image_list_t>(n)->get_image_id(ribi / 3 - 1);
	}

private:

	image_id get_slope_image_nr(slope_t::type slope,  bool front) const
	{
		int const n = front ? 11 : 8;

		if(slope_indices[n] < 0)
		{
			return IMG_EMPTY;
		}

		int nr;
		switch (slope)
		{
		case 4:
			nr = 0;
			break;
		case 12:
			nr = 1;
			break;
		case 28:
			nr = 2;
			break;
		case 36:
			nr = 3;
			break;
		case 8:
			nr = 4;
			break;
		case 24:
			nr = 5;
			break;
		case 56:
			nr = 6;
			break;
		case 72:
			nr = 7;
			break;
		default:
			return IMG_EMPTY;
		}

		image_id slope_img = get_child<image_list_t>(n)->get_image_id(nr);

		if (nr > 3 && slope_img == IMG_EMPTY  &&  get_child<image_list_t>(n)->get_count() <= 4) 
		{
			// hack for old ways without double height images to use single slope images for both
			nr -= 4;
			slope_img = get_child<image_list_t>(n)->get_image_id(nr);
		}
		return slope_img;
	}
	
public:

	skin_desc_t const* get_cursor() const { return get_child<skin_desc_t>(4); }

	waytype_t get_finance_waytype() const;

	uint32  get_max_axle_load() const { return axle_load; }

	inline bool get_has_way() const { return has_way == 1 || has_way == 3; }

	inline bool has_tunnel_internal_images() const { return has_way == 2 || has_way == 3;  }
	
	/* Way constraints: determines whether vehicles
	 * can travel on this way. This method decodes
	 * the byte into bool values. See here for
	 * information on bitwise operations: 
	 * http://www.cprogramming.com/tutorial/bitwise_operators.html
	 * @author: jamespetts
	 * */
	const way_constraints_of_way_t& get_way_constraints() const { return way_constraints; }
	void set_way_constraints(const way_constraints_of_way_t& value) { way_constraints = value; }

	const way_desc_t *get_way_desc() const
	{
		if(get_has_way())
		{
			return get_child<way_desc_t>(5 + number_of_seasons * 2);
		}
		return NULL;
	}

	bool has_broad_portals() const { return (broad_portals != 0); };

	void calc_checksum(checksum_t *chk) const;
};

#endif
