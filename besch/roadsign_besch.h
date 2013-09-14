/*
 *  Copyright (c) 2006 by prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 *  Module description:
 *      signs on roads and other ways
 */

#ifndef __ROADSIGN_BESCH_H
#define __ROADSIGN_BESCH_H

#include "obj_besch_std_name.h"
#include "bildliste_besch.h"
#include "skin_besch.h"
#include "../dataobj/ribi.h"
#include "../simtypes.h"
#include "../network/checksum.h"


/*
 *  Autor:
 *      prissi
 *
 *  Description:
 *	Road signs
 *
 *  Child nodes:
 *	0   Name
 *	1   Copyright
 *	2   Image list (Bildliste)
 */
class roadsign_besch_t : public obj_besch_transport_infrastructure_t {
	friend class roadsign_reader_t;

private:
	uint8 flags;

	uint16 min_speed;	// 0 = no min speed

public:
	enum types {
		NONE                  = 0,
		ONE_WAY               = 1U << 0,
		CHOOSE_SIGN           = 1U << 1,
		PRIVATE_ROAD          = 1U << 2,
		SIGN_SIGNAL           = 1U << 3,
		SIGN_PRE_SIGNAL       = 1U << 4,
		ONLY_BACKIMAGE        = 1U << 5,
		SIGN_LONGBLOCK_SIGNAL = 1U << 6,
		END_OF_CHOOSE_AREA    = 1U << 7
	};

	int get_bild_nr(ribi_t::dir dir) const
	{
		bild_besch_t const* const bild = get_child<bildliste_besch_t>(2)->get_bild(dir);
		return bild != NULL ? bild->get_nummer() : IMG_LEER;
	}

	int get_bild_anzahl() const { return get_child<bildliste_besch_t>(2)->get_anzahl(); }

	skin_besch_t const* get_cursor() const { return get_child<skin_besch_t>(3); }

	sint32 get_min_speed() const { return min_speed; }

	bool is_single_way() const { return (flags&ONE_WAY)!=0; }

	bool is_private_way() const { return (flags&PRIVATE_ROAD)!=0; }

	//  return true for a traffic light
	bool is_traffic_light() const { return (get_bild_anzahl()>4); }

	bool is_choose_sign() const { return flags&CHOOSE_SIGN; }

	//  return true for signal
	bool is_signal() const { return flags&SIGN_SIGNAL; }

	//  return true for presignal
	bool is_pre_signal() const { return flags&SIGN_PRE_SIGNAL; }

	//  return true for single track section signal
	bool is_longblock_signal() const { return flags&SIGN_LONGBLOCK_SIGNAL; }

	bool is_end_choose_signal() const { return flags&END_OF_CHOOSE_AREA; }

	bool is_signal_type() const
	{
		return (flags&(SIGN_SIGNAL|SIGN_PRE_SIGNAL|SIGN_LONGBLOCK_SIGNAL))!=0;
	}

	types get_flags() const { return (types)flags; }

	void calc_checksum(checksum_t *chk) const
	{
		obj_besch_transport_infrastructure_t::calc_checksum(chk);
		chk->input(flags);
		chk->input(min_speed);
	}
};

ENUM_BITSET(roadsign_besch_t::types)

#endif
