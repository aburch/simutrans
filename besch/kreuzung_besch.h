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

/*
 *  includes
 */
#include "text_besch.h"
#include "bild_besch.h"
#include "../simtypes.h"


/*
 *  class:
 *      kreuzung_besch_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      ...
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bild
 */
class kreuzung_besch_t : public obj_besch_t {
    friend class crossing_writer_t;
    friend class crossing_reader_t;

private:
    uint8  wegtyp_ns;
    uint8  wegtyp_ow;

public:
	const char *gib_name() const { return static_cast<const text_besch_t *>(gib_kind(0))->gib_text(); }
	const char *gib_copyright() const { return static_cast<const text_besch_t *>(gib_kind(1))->gib_text(); }

	const bild_besch_t *gib_bild() const { return static_cast<const bild_besch_t *>(gib_kind(2)); }
	int gib_bild_nr() const
	{
		const bild_besch_t *bild = gib_bild();
		return bild ? bild->bild_nr : -1;
	}

	waytype_t gib_wegtyp_ns() const { return static_cast<waytype_t>(wegtyp_ns); }
	waytype_t gib_wegtyp_ow() const { return static_cast<waytype_t>(wegtyp_ow); }
};

#endif // __KREUZUNG_BESCH_H
