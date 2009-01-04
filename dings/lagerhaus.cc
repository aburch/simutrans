/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#ifdef LAGER_NOT_IN_USE

#include "../simdebug.h"
#include "../simtypes.h"
#include "../simimg.h"
#include "../simhalt.h"
#include "../simplay.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "lagerhaus.h"

int lagerhaus_t::max_lager = 500;


lagerhaus_t::lagerhaus_t(karte_t *welt, loadsave_t *file) :
    gebaeude_t(welt)
{
    for(int i=0; i<warenbauer_t::MAX_WAREN; i++) {
	lager[i].set_typ(i);
    }
    rdwr(file);
    set_bild(0, IMG_LAGERHAUS);
}


lagerhaus_t::lagerhaus_t(karte_t *welt, koord3d pos, spieler_t *sp) :
    gebaeude_t(welt, pos, sp, gebaeude_t::unbekannt, 1)
{
    for(int i=0; i<warenbauer_t::MAX_WAREN; i++) {
	lager[i].menge = 0;
	lager[i].set_typ(i);
    }

    set_bild(0, IMG_LAGERHAUS);
}

lagerhaus_t::~lagerhaus_t()
{
    halthandle_t halt = get_besitzer()->is_my_halt(get_pos().get_2d());

    if(halt.is_bound()) {
	halt->set_lager( NULL );
    } else {
	printf("~lagerhaus: konnte lagerhaus nicht bei haltestelle abmelden!\n");
    }
}


bool
lagerhaus_t::nimmt_an(int wtyp) const
{
    // typ ungleich Passagiere
    if(wtyp != 0) {
	return lager[wtyp].menge < max_lager;
    } else {
	return false;
    }
}

bool
lagerhaus_t::gibt_ab(int wtyp) const
{
    return lager[wtyp].menge > 0;
}

/**
 * holt ware ab
 * @return abgeholte menge
 */
int
lagerhaus_t::hole_ab(int wtyp, int menge )
{
    int abgeholt = menge;

    if(menge > lager[wtyp].menge) {
	abgeholt = lager[wtyp].menge;
    }
    lager[wtyp].menge -= abgeholt;

    return abgeholt;
}

/**
 * liefert ware an
 * @return angenommene menge
 */
int
lagerhaus_t::liefere_an(int wtyp, int menge )
{
    if(lager[wtyp].menge + menge > max_lager) {
	menge = max_lager - lager[wtyp].menge;
    }

    lager[wtyp].menge += menge;

    return menge;
}


char *
lagerhaus_t::info_lagerbestand(char *buf) const
{
    for(int i=1; i<warenbauer_t::MAX_WAREN; i++) {
	buf += sprintf(buf, "%s  %d %s\n",
                       translator::translate(lager[i].name()),
		       lager[i].menge,
                       translator::translate(lager[i].mass())
		      );
    }
    return buf;
}

void
lagerhaus_t::rdwr(loadsave_t *file)
{
    gebaeude_t::rdwr(file);

    for(int i=0; i<warenbauer_t::MAX_WAREN; i++) {
	file->rdwr_long(lager[i].menge, "\n");
    }
}
#endif
