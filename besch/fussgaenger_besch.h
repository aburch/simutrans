/*
 *
 *  fussgaenger_besch.h
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
#ifndef __fussgaenger_besch_h
#define __fussgaenger_besch_h

#include "obj_besch_std_name.h"
#include "bildliste_besch.h"
#include "../dataobj/ribi.h"

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
    friend class pedestrian_writer_t;
    friend class pedestrian_reader_t;

    uint16 gewichtung;
public:
    int gib_bild_nr(ribi_t::dir dir) const
    {
	const bild_besch_t *bild = static_cast<const bildliste_besch_t *>(gib_kind(2))->gib_bild(dir);

	return bild != NULL ? bild->gib_nummer() : IMG_LEER;
    }
    int gib_gewichtung() const
    {
	return gewichtung;
    }
};

#endif
