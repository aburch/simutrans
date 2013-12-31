/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __fussgaenger_besch_h
#define __fussgaenger_besch_h

#include "obj_besch_std_name.h"
#include "bildliste_besch.h"
#include "../dataobj/ribi.h"
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
class fussgaenger_besch_t : public obj_besch_std_name_t {
    friend class pedestrian_reader_t;

    uint16 gewichtung;
public:
    int get_bild_nr(ribi_t::dir dir) const
    {
		bild_besch_t const* const bild = get_child<bildliste_besch_t>(2)->get_bild(dir);
		return bild != NULL ? bild->get_nummer() : IMG_LEER;
    }
    int get_gewichtung() const
    {
		return gewichtung;
    }
	void calc_checksum(checksum_t *chk) const
	{
		chk->input(gewichtung);
	}
};

#endif
