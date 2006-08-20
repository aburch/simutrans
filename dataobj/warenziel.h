/*
 * warenziel.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef dataobj_warenziel_h
#define dataobj_warenziel_h

#include "koord.h"

class ware_besch_t;

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
    koord ziel;
    const ware_besch_t *typ;

public:
    // don't use them, or fix them: Actually, these should check for stops
    // but they are needed for list search ...
    int operator==(const warenziel_t &wz) {
        return (ziel == wz.gib_ziel()  &&  typ==wz.gib_typ());
    }
    int operator!=(const warenziel_t &wz) {
        return ziel!=wz.gib_ziel()  ||  typ!=wz.gib_typ();
    }

    warenziel_t() {typ = 0;};

    warenziel_t(koord ziel, const ware_besch_t *b);

    warenziel_t(loadsave_t *file);

    void setze_ziel(koord z) {ziel=z;};
    const koord & gib_ziel() const {return ziel;};

    const ware_besch_t * gib_typ() const {return typ;};

    void rdwr(loadsave_t *file);

    void * operator new(size_t s);
    void operator delete(void *p);
};


#endif
