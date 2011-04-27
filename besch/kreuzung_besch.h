/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __KREUZUNG_BESCH_H
#define __KREUZUNG_BESCH_H

#include "obj_besch_std_name.h"
#include "bild_besch.h"
#include "bildliste_besch.h"
#include "../simtypes.h"
#include "../utils/checksum.h"


class checksum_t;

/*
 *  Autor:
 *      Volker Meyer
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bild
 */
class kreuzung_besch_t : public obj_besch_std_name_t {
    friend class crossing_writer_t;
    friend class crossing_reader_t;

private:
	uint8 wegtyp1;
	uint8 wegtyp2;

	sint8 sound;

	uint32 closed_animation_time;
	uint32 open_animation_time;

	sint32 topspeed1;	// the topspeed depeds strongly on the crossing ...
	sint32 topspeed2;

	/**
	 * Introduction/Retire date
	 */
	uint16 intro_date;
	uint16 obsolete_date;

public:
	/* the imagelists are:
	 * open_ns
	 * open_ew
	 * front_open_ns
	 * front_open_ew
	 * closed_ns
	 * ....
	 * =>ns=0 NorthSouth ns=1, East-West
	 */
	const bild_besch_t *get_bild(int ns, bool open, int phase) const
	{
		if(open) {
			return get_child<bildliste_besch_t>(2 + ns)->get_bild(phase);
		}
		else {
			bildliste_besch_t const* const bl = get_child<bildliste_besch_t>(6 + ns);
			return bl ? bl->get_bild(phase) : NULL;
		}
	}

	const bild_besch_t *get_bild_after(int ns, bool open, int phase) const
	{
		int const n = ns + (open ? 4 : 8);
		bildliste_besch_t const* const bl = get_child<bildliste_besch_t>(n);
		return bl ? bl->get_bild(phase) : 0;
	}

	waytype_t get_waytype(int i) const { return (waytype_t)(i==0? wegtyp1 : wegtyp2); }
	sint32 get_maxspeed(int i) const { return i==0 ? topspeed1 : topspeed2; }
	uint16 get_phases(bool open, bool front) const { return get_child<bildliste_besch_t>(6 - 4 * open + 2 * front)->get_anzahl(); }
	uint32 get_animation_time(bool open) const { return open ? open_animation_time : closed_animation_time; }

	sint8 get_sound() const { return sound; }

	/**
	* @return introduction year
	*/
	uint16 get_intro_year_month() const { return intro_date; }

	/**
	* @return time when obsolete
	*/
	uint16 get_retire_year_month() const { return obsolete_date; }

	/**
	* @return true if the crossing is available
	*/
	bool is_available(const uint16 month_now) const
	{
		return month_now==0  ||  (intro_date <= month_now  &&  month_now < obsolete_date);
	}

	void calc_checksum(checksum_t *chk) const
	{
		chk->input(wegtyp1);
		chk->input(wegtyp2);
		chk->input(closed_animation_time);
		chk->input(open_animation_time);
		chk->input(topspeed1);
		chk->input(topspeed2);
		chk->input(intro_date);
		chk->input(obsolete_date);
	}
};

#endif
