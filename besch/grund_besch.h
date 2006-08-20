/*
 *
 *  grund_besch.h
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
#ifndef __GRUND_BESCH_H
#define __GRUND_BESCH_H

/*
 *  includes
 */
#include "text_besch.h"
#include "bildliste2d_besch.h"
#include "../dataobj/ribi.h"

/*
 *  class:
 *      grund_besch_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Verschiedene Untergründe - viellcht bald weisse Berge?
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bildliste2D
 */
class grund_besch_t : public obj_besch_t {
    friend class ground_writer_t;

public:
    const char *gib_name() const
    {
        return static_cast<const text_besch_t *>(gib_kind(0))->gib_text();
    }
    const char *gib_copyright() const
    {
        return static_cast<const text_besch_t *>(gib_kind(1))->gib_text();
    }
    int gib_phasen(hang_t::typ typ) const
    {
	const bildliste_besch_t *liste = static_cast<const bildliste2d_besch_t *>(gib_kind(2))->gib_liste(typ);
	if(liste) {
	    return liste->gib_anzahl();
	}
	return 0;
    }
    int gib_bild(int typ, int phase  = 0) const
    {
	const bildliste_besch_t *liste = static_cast<const bildliste2d_besch_t *>(gib_kind(2))->gib_liste(typ);
	if(liste && liste->gib_anzahl() > 0) {
	    const bild_besch_t *bild = static_cast<const bildliste2d_besch_t *>(
		gib_kind(2))->gib_bild(typ, phase % liste->gib_anzahl());
	    if(bild) {
		return bild->bild_nr;
	    }
	}
	return -1;
    }
    /*
     * Die Klasse verwaltet ihre Instanzen selber:
     */
    static const grund_besch_t *boden;
    static const grund_besch_t *ufer;
    static const grund_besch_t *wasser;
    static const grund_besch_t *fundament;
    static const grund_besch_t *ausserhalb;

    static bool register_besch(const grund_besch_t *besch);

    static bool alles_geladen();
};

#endif // __GRUND_BESCH_H
