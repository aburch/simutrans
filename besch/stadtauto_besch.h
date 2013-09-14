/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __STADTAUTO_BESCH_H
#define __STADTAUTO_BESCH_H

#include "obj_besch_std_name.h"
#include "bildliste_besch.h"
#include "../dataobj/ribi.h"
#include "../simtypes.h"
#include "../network/checksum.h"


/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *	Automatisch generierte Autos, die in der Stadt umherfahren.
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bildliste
 */
class stadtauto_besch_t : public obj_besch_timelined_t {
	friend class citycar_reader_t;

	uint16 gewichtung;

	/// topspeed in internal speed units !!! not km/h!!!
	uint16 geschw;

public:
	int get_bild_nr(ribi_t::dir dir) const
	{
		bild_besch_t const* const bild = get_child<bildliste_besch_t>(2)->get_bild(dir);
		return bild != NULL ? bild->get_nummer() : IMG_LEER;
	}

	int get_gewichtung() const { return gewichtung; }

	/// topspeed in internal speed units !!! not km/h!!!
	sint32 get_geschw() const { return geschw; }

	void calc_checksum(checksum_t *chk) const
	{
		obj_besch_timelined_t::calc_checksum(chk);
		chk->input(gewichtung);
		chk->input(geschw);
	}
};

#endif
