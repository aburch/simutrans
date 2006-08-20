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

/*
 *  includes
 */
#include "text_besch.h"
#include "bildliste_besch.h"
#include "../dataobj/ribi.h"
#include "../simtypes.h"

/*
 *  class:
 *      stadtauto_besch_t()
 *
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
class stadtauto_besch_t : public obj_besch_t {
    friend class citycar_writer_t;
    friend class citycar_reader_t;

    uint16 gewichtung;
    // when was this car used?
    uint16 intro_date;
    uint16 obsolete_date;
public:
    const char *gib_name() const
    {
        return static_cast<const text_besch_t *>(gib_kind(0))->gib_text();
    }
    const char *gib_copyright() const
    {
        return static_cast<const text_besch_t *>(gib_kind(1))->gib_text();
    }
    int gib_bild_nr(ribi_t::dir dir) const
    {
	const bild_besch_t *bild = static_cast<const bildliste_besch_t *>(gib_kind(2))->gib_bild(dir);

	return bild ? bild->bild_nr : -1;
    }
    int gib_gewichtung() const
    {
	return gewichtung;
    }
};

#endif // __STADTAUTO_BESCH_H
