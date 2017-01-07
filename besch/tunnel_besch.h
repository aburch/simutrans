/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 *  node structure:
 *  0   Name
 *  1   Copyright
 *  2   Image-list Hintergrund
 *  3   Image-list Vordergrund
 *  4   cursor(image 0) and icon (image 1)
 *[ 5   Image-list Hintergrund - snow ] (if present)
 *[ 6   Image-list Vordergrund - snow ] (if present)
 *[ 7 (or 5 if no snow image) underground way ] (if present)
 */

#ifndef __TUNNEL_BESCH_H
#define __TUNNEL_BESCH_H

#include "../display/simimg.h"
#include "../simtypes.h"
#include "obj_besch_std_name.h"
#include "skin_besch.h"
#include "bildliste2d_besch.h"
#include "weg_besch.h"


class tunnel_besch_t : public obj_besch_transport_infrastructure_t {
	friend class tunnel_reader_t;
	friend class tunnelbauer_t;	// to convert the old tunnels to new ones

private:
	static int hang_indices[81];

	/* number of seasons (0 = none, 1 = no snow/snow
	 */
	sint8 number_seasons;

	/* has underground way image ? ( 0 = no, 1 = yes
	 */
	uint8 has_way;

	/* Has broad portals?
	 */
	uint8 broad_portals;

public:
	const image_t *get_background(slope_t::type hang, uint8 season, uint8 type ) const
	{
		const uint8 n = season && number_seasons == 1 ? 5 : 2;
		return get_child<image_list_t>(n)->get_image(hang_indices[hang] + 4 * type);
	}

	image_id get_background_nr(slope_t::type hang, uint8 season, uint8 type ) const
	{
		const image_t *desc = get_background(hang, season, type );
		return desc != NULL ? desc->get_id() : IMG_EMPTY;
	}

	const image_t *get_foreground(slope_t::type hang, uint8 season, uint8 type ) const
	{
		const uint8 n = season && number_seasons == 1 ? 6 : 3;
		return get_child<image_list_t>(n)->get_image(hang_indices[hang] + 4 * type);
	}

	image_id get_foreground_nr(slope_t::type hang, uint8 season, uint8 type) const
	{
		const image_t *desc = get_foreground(hang, season, type );
		return desc != NULL ? desc->get_id() :IMG_EMPTY;
	}

	skin_besch_t const* get_cursor() const { return get_child<skin_besch_t>(4); }

	waytype_t get_finance_waytype() const;

	const weg_besch_t *get_weg_besch() const
	{
		if(has_way) {
			return get_child<weg_besch_t>(5 + number_seasons * 2);
		}
		return NULL;
	}

	bool has_broad_portals() const { return (broad_portals != 0); };
};

#endif
