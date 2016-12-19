/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 *  node structure:
 *  0   Name
 *  1   Copyright
 *  2   Bildliste Hintergrund
 *  3   Bildliste Vordergrund
 *  4   cursor(image 0) and icon (image 1)
 *[ 5   Bildliste Hintergrund - snow ] (if present)
 *[ 6   Bildliste Vordergrund - snow ] (if present)
 *[ 7 (or 5 if no snow image) underground way ] (if present)
 */

#ifndef __TUNNEL_BESCH_H
#define __TUNNEL_BESCH_H

#include "../display/simimg.h"
#include "../simtypes.h"
#include "../dataobj/way_constraints.h"
#include "obj_besch_std_name.h"
#include "skin_besch.h"
#include "bildliste2d_besch.h"
#include "weg_besch.h"


class tunnel_besch_t : public obj_besch_transport_infrastructure_t {
	friend class tunnel_reader_t;
	friend class tunnelbauer_t;	// to convert the old tunnels to new ones

private:
	static int hang_indices[81];

	/* number of seasons (0 = none, 1 = no snow/snow)
	*/
	sint8 number_seasons;

	/*Way constraints for, e.g., loading gauges, types of electrification, etc.
	* @author: jamespetts*/
	way_constraints_of_way_t way_constraints;

	/* has underground way image ? (0 = no, 1 = yes)
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
	
	const bild_besch_t *get_hintergrund(hang_t::typ hang, uint8 season, uint8 type ) const
	{
		int const n = season && number_seasons == 1 ? 5 : 2;
		return get_child<bildliste_besch_t>(n)->get_image(hang_indices[hang] + 4 * type);
	}

	image_id get_hintergrund_nr(hang_t::typ hang, uint8 season, uint8 type ) const
	{
		const bild_besch_t *besch = get_hintergrund(hang, season, type );
		return besch != NULL ? besch->get_nummer() : IMG_LEER;
	}

	const bild_besch_t *get_vordergrund(hang_t::typ hang, uint8 season, uint8 type ) const
	{
		int const n = season && number_seasons == 1 ? 6 : 3;
		return get_child<bildliste_besch_t>(n)->get_image(hang_indices[hang] + 4 * type);
	}

	image_id get_vordergrund_nr(hang_t::typ hang, uint8 season, uint8 type) const
	{
		const bild_besch_t *besch = get_vordergrund(hang, season, type );
		return besch != NULL ? besch->get_nummer() : IMG_LEER;
	}

	image_id get_underground_backimage_nr(ribi_t::ribi ribi, hang_t::typ hang) const
	{
		return hang == hang_t::flach ? get_child<bildliste_besch_t>(7)->get_bild_nr(ribi) : get_hang_bild_nr(hang, false);
	}

	image_id get_underground_frontimage_nr(ribi_t::ribi ribi, hang_t::typ hang) const
	{
		return hang == hang_t::flach ? get_child<bildliste_besch_t>(10)->get_bild_nr(ribi) : get_hang_bild_nr(hang, true);
	}

	image_id get_diagonal_bild_nr(ribi_t::ribi ribi, bool front) const
	{
		const uint16 n = front ? 12 : 9;
		return get_child<bildliste_besch_t>(n)->get_bild_nr(ribi / 3 - 1);
	}

private:

	image_id get_hang_bild_nr(hang_t::typ hang,  bool front) const
	{
		int const n = front ? 11 : 8;
		int nr;
		switch (hang)
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
			return IMG_LEER;
		}

		image_id hang_img = get_child<bildliste_besch_t>(n)->get_bild_nr(nr);

		if (nr > 3 && hang_img == IMG_LEER  &&  get_child<bildliste_besch_t>(n)->get_anzahl() <= 4) 
		{
			// hack for old ways without double height images to use single slope images for both
			nr -= 4;
			hang_img = get_child<bildliste_besch_t>(n)->get_bild_nr(nr);
		}
		return hang_img;
	}
	
public:

	skin_besch_t const* get_cursor() const { return get_child<skin_besch_t>(4); }

	waytype_t get_finance_waytype() const;

	uint32  get_max_axle_load() const { return axle_load; }
	
	/* Way constraints: determines whether vehicles
	 * can travel on this way. This method decodes
	 * the byte into bool values. See here for
	 * information on bitwise operations: 
	 * http://www.cprogramming.com/tutorial/bitwise_operators.html
	 * @author: jamespetts
	 * */
	const way_constraints_of_way_t& get_way_constraints() const { return way_constraints; }
	void set_way_constraints(const way_constraints_of_way_t& value) { way_constraints = value; }

	const weg_besch_t *get_weg_besch() const
	{
		if(has_way) {
			return get_child<weg_besch_t>(5 + number_seasons * 2);
		}
		return NULL;
	}

	bool has_broad_portals() const { return (broad_portals != 0); };

	void calc_checksum(checksum_t *chk) const;
};

#endif
