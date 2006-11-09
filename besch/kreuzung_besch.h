/*
 *
 *  kreuzung_besch.h
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
#ifndef __KREUZUNG_BESCH_H
#define __KREUZUNG_BESCH_H

#include "obj_besch_std_name.h"
#include "bild_besch.h"
#include "../simtypes.h"


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
    uint8  wegtyp_ns;
    uint8  wegtyp_ow;

public:
	const bild_besch_t *gib_bild() const { return static_cast<const bild_besch_t *>(gib_kind(2)); }
	int gib_bild_nr() const
	{
		const bild_besch_t *bild = gib_bild();
		return bild != NULL ? bild->gib_nummer() : IMG_LEER;
	}

	waytype_t gib_wegtyp_ns() const { return static_cast<waytype_t>(wegtyp_ns); }
	waytype_t gib_wegtyp_ow() const { return static_cast<waytype_t>(wegtyp_ow); }
};

#endif
