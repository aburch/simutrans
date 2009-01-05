/*
 *  Copyright (c) 2006 by prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 *  Modulbeschreibung:
 *      signs on roads and other ways
 */

#ifndef __ROADSIGN_BESCH_H
#define __ROADSIGN_BESCH_H

#include "obj_besch_std_name.h"
#include "bildliste_besch.h"
#include "../dataobj/ribi.h"
#include "../simtypes.h"


class skin_besch_t;

/*
 *  Autor:
 *      prissi
 *
 *  Beschreibung:
 *	Straﬂenschildere
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bildliste
 */
class roadsign_besch_t : public obj_besch_std_name_t {
	friend class roadsign_writer_t;
	friend class roadsign_reader_t;

private:
	uint8 flags;

	/**
	* Way type: i.e. road or track
	* @see waytype_t
	* @author prissi
	*/
	uint8 wtyp;

	uint16 min_speed;	// 0 = no min speed

	uint32 cost;

	/**
	* Introduction date
	* @author prissi
	*/
	uint16 intro_date;
	uint16 obsolete_date;

public:
	enum types {ONE_WAY=1, FREE_ROUTE=2, PRIVATE_ROAD=4, SIGN_SIGNAL=8, SIGN_PRE_SIGNAL=16, ONLY_BACKIMAGE=32, SIGN_LONGBLOCK_SIGNAL=64, END_OF_CHOOSE_AREA=128 };

	int get_bild_nr(ribi_t::dir dir) const
	{
		const bild_besch_t *bild = static_cast<const bildliste_besch_t *>(get_child(2))->get_bild(dir);
		return bild != NULL ? bild->get_nummer() : IMG_LEER;
	}

	int get_bild_anzahl() const { return static_cast<const bildliste_besch_t *>(get_child(2))->get_anzahl(); }

	const skin_besch_t *get_cursor() const { return (const skin_besch_t *)get_child(3); }

	/**
	 * get way type
	 * @see waytype_t
	 * @author Hj. Malthaner
	 */
	waytype_t get_wtyp() const { return (waytype_t)wtyp; }

	int get_min_speed() const { return min_speed; }

	sint32 get_preis() const { return cost; }

	bool is_single_way() const { return (flags&ONE_WAY)!=0; }

	bool is_private_way() const { return (flags&PRIVATE_ROAD)!=0; }

	//  return true for a traffic light
	bool is_traffic_light() const { return (get_bild_anzahl()>4); }

	bool is_free_route() const { return flags&FREE_ROUTE; }

	//  return true for signal
	bool is_signal() const { return flags&SIGN_SIGNAL; }

	//  return true for presignal
	bool is_pre_signal() const { return flags&SIGN_PRE_SIGNAL; }

	//  return true for single track section signal
	bool is_longblock_signal() const { return flags&SIGN_LONGBLOCK_SIGNAL; }

	bool is_signal_type() const { return (flags&(SIGN_SIGNAL|SIGN_PRE_SIGNAL|SIGN_LONGBLOCK_SIGNAL))!=0; }

	uint8 get_flags() const { return flags; }

	/**
	* @return introduction year
	* @author prissi
	*/
	uint16 get_intro_year_month() const { return intro_date; }

	/**
	* @return introduction month
	* @author prissi
	*/
	uint16 get_retire_year_month() const { return obsolete_date; }
};

#endif
