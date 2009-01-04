/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 *  node structure:
 *	0   Name
 *	1   Copyright
 *	2   Bildliste Hintergrund
 *	3   Bildliste Vordergrund
 *  4   cursor(image 0) and icon (image 1)
 *	5   Bildliste Hintergrund - snow
 *	6   Bildliste Vordergrund - snow
 */

#ifndef __TUNNEL_BESCH_H
#define __TUNNEL_BESCH_H

#include "../simimg.h"
#include "../simtypes.h"
#include "obj_besch_std_name.h"
#include "skin_besch.h"
#include "bildliste2d_besch.h"


class tunnel_besch_t : public obj_besch_std_name_t {
	friend class tunnel_writer_t;
	friend class tunnel_reader_t;
	friend class tunnelbauer_t;	// to convert the old tunnels to new ones

private:
	static int hang_indices[16];

	uint32 topspeed;	// speed in km/h
	uint32 preis;	// 1/100 credits
	uint32 maintenance;	// monthly cost for bits_per_month=18
	uint8 wegtyp;	// waytype for tunnel

	// allowed eara
	uint16 intro_date;
	uint16 obsolete_date;

	/* number of seasons (0 = none, 1 = no snow/snow
	*/
	sint8 number_seasons;
public:
	const bild_besch_t *get_hintergrund(hang_t::typ hang, uint8 season) const
	{
		const bild_besch_t *bild = NULL;
		if(season && number_seasons == 1) {
			bild = static_cast<const bildliste_besch_t *>(get_kind(5))->get_bild(hang_indices[hang]);
		}
		if(bild == NULL) {
			bild = static_cast<const bildliste_besch_t *>(get_kind(2))->get_bild(hang_indices[hang]);
		}
		return bild;
	}

	image_id get_hintergrund_nr(hang_t::typ hang, uint8 season) const
	{
		const bild_besch_t *besch = get_hintergrund(hang, season);
		return besch != NULL ? besch->get_nummer() : IMG_LEER;
	}

	const bild_besch_t *get_vordergrund(hang_t::typ hang, uint8 season) const
	{
		const bild_besch_t *bild = NULL;
		if(season && number_seasons == 1) {
			bild = static_cast<const bildliste_besch_t *>(get_kind(6))->get_bild(hang_indices[hang]);
		}
		if(bild == NULL) {
			bild = static_cast<const bildliste_besch_t *>(get_kind(3))->get_bild(hang_indices[hang]);
		}
		return bild;
	}

	image_id get_vordergrund_nr(hang_t::typ hang, uint8 season) const
	{
		const bild_besch_t *besch = get_vordergrund(hang, season);
		return besch != NULL ? besch->get_nummer() :IMG_LEER;
	}

	const skin_besch_t *get_cursor() const { return static_cast<const skin_besch_t *>(get_kind(4)); }


	// get costs etc.
	waytype_t get_waytype() const { return static_cast<waytype_t>(wegtyp); }

	sint32 get_preis() const { return preis; }

	sint32 get_wartung() const { return maintenance; }

	uint32  get_topspeed() const { return topspeed; }

	uint16 get_intro_year_month() const { return intro_date; }

	uint16 get_retire_year_month() const { return obsolete_date; }
};

#endif
