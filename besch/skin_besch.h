/*
 *
 *  skin_besch.h
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
#ifndef __SKIN_BESCH_H
#define __SKIN_BESCH_H

/*
 *  includes
 */
#include "text_besch.h"
#include "bildliste2d_besch.h"


/*
 *  class:
 *      skin_besch_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Skin ist im wesentlichen erstmal eine Bildliste.
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bildliste
 */
class skin_besch_t : public obj_besch_t {
    friend class skin_writer_t;

public:
    const char *gib_name() const
    {
	return static_cast<const text_besch_t *>(gib_kind(0))->gib_text();
    }
    const char *gib_copyright() const
    {
	return static_cast<const text_besch_t *>(gib_kind(1))->gib_text();
    }
    const bild_besch_t *gib_bild(int i) const
    {
	return static_cast<const bildliste_besch_t *>(gib_kind(2))->gib_bild(i);
    }
    int gib_bild_anzahl() const
    {
	return static_cast<const bildliste_besch_t *>(gib_kind(2))->gib_anzahl();
    }
    int gib_bild_nr(int i) const
    {
	const bild_besch_t *bild = gib_bild(i);

	return bild ? bild->bild_nr : -1;
    }
};

#endif // __SKIN_BESCH_H
