/*
 *
 *  stadtauto_besch.h
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */
#ifndef __STADTAUTO_BESCH_H
#define __STADTAUTO_BESCH_H

#include "obj_besch_std_name.h"
#include "bildliste_besch.h"
#include "../dataobj/ribi.h"
#include "../simtypes.h"

#include "intro_dates.h"


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
class stadtauto_besch_t : public obj_besch_std_name_t {
    friend class citycar_writer_t;
    friend class citycar_reader_t;

    uint16 gewichtung;
    // max speed
    uint16 geschw;
    // when was this car used?
    uint16 intro_date;
    uint16 obsolete_date;

	uint8	length[8];	// length of pixel until leaving the field (not used)

public:
    int gib_bild_nr(ribi_t::dir dir) const
    {
	const bild_besch_t *bild = static_cast<const bildliste_besch_t *>(gib_kind(2))->gib_bild(dir);

	return bild ? bild->bild_nr : -1;
    }
    int gib_gewichtung() const
    {
	return gewichtung;
  }

	int gib_geschw() const
	{
		return geschw;
	}

      /**
     * @return introduction year
     * @author Hj. Malthaner
     */
    int get_intro_year_month() const {
      return intro_date;
    }

    /**
     * @return time when obsolete
     * @author prissi
     */
    int get_retire_year_month() const {
      return obsolete_date;
    }

	/* @return the normalized distance to the next vehicle
	 * @author prissi
	 */
	uint8 get_length_to_next( uint8 next_dir ) const {
		return length[next_dir];
	}
};

#endif
