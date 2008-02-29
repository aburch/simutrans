/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef dataobj_warenziel_h
#define dataobj_warenziel_h

#include "../halthandle_t.h"
#include "../besch/ware_besch.h"

class loadsave_t;

/**
 * Diese Klasse wird zur Verwaltung von Zielen von
 * Haltestellen benutzt. Grundlegende Elemente sind
 * eine Koordinate und ein Zeitstempel.
 *
 * @author Hj. Malthaner
 */

class warenziel_t
{
private:
//    koord ziel;
		halthandle_t halt;
    uint8 catg_index;

public:
	/*
    // don't use them, or fix them: Actually, these should check for stops
    // but they are needed for list search ...
    int operator==(const warenziel_t &wz) {
        return (ziel == wz.gib_ziel()  &&  typ==wz.gib_typ());
    }
    int operator!=(const warenziel_t &wz) {
        return ziel!=wz.gib_ziel()  ||  typ!=wz.gib_typ();
    }
*/
    // don't use them, or fix them: Actually, these should check for stops
    // but they are needed for list search ...
    int operator==(const warenziel_t &wz) {
        return (halt == wz.gib_zielhalt()  &&  catg_index==wz.gib_catg_index());
    }
    int operator!=(const warenziel_t &wz) {
        return halt!=wz.gib_zielhalt()  ||  catg_index!=wz.gib_catg_index();
    }

		warenziel_t() { catg_index = 255; halt = halthandle_t();}

		warenziel_t(halthandle_t &h, const ware_besch_t *b) { halt = h; catg_index = b->gib_catg_index(); }

    warenziel_t(loadsave_t *file);

    void setze_zielhalt(halthandle_t &h) { halt = h; }
    const halthandle_t gib_zielhalt() const { return halt; }

    uint8 gib_catg_index() const { return catg_index; }

    void rdwr(loadsave_t *file);
};

#endif
