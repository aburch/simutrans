/*
 * simpeople.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "simdebug.h"
#include "simworld.h"
#include "simimg.h"

#include "simpeople.h"
#include "simtools.h"
#include "simmem.h"
#include "boden/grund.h"
#include "dataobj/loadsave.h"
#include "tpl/slist_mit_gewichten_tpl.h"

#include "besch/fussgaenger_besch.h"

int fussgaenger_t::count = 0;

int fussgaenger_t::strecke[] = {6000, 11000, 15000, 20000, 25000, 30000, 35000, 40000};

slist_mit_gewichten_tpl<const fussgaenger_besch_t *> fussgaenger_t::liste;
stringhashtable_tpl<const fussgaenger_besch_t *> fussgaenger_t::table;


bool fussgaenger_t::register_besch(const fussgaenger_besch_t *besch)
{
    liste.append(besch);
    table.put(besch->gib_name(), besch);

    return true;
}

bool fussgaenger_t::laden_erfolgreich()
{
    if(liste.count() == 0) {
	DBG_MESSAGE("fussgaenger_t", "No pedestrians found - feature disabled");
    }
    return true ;
}


int fussgaenger_t::gib_anzahl_besch()
{
    return liste.count();
}


fussgaenger_t::fussgaenger_t(karte_t *welt, loadsave_t *file)
 : verkehrsteilnehmer_t(welt)
{
    rdwr(file);

    setze_max_speed(128);
    schritte = strecke[count & 3];
    sum = 0;
    count ++;

    is_sync = true;
    welt->sync_add(this);
}


fussgaenger_t::fussgaenger_t(karte_t *welt, koord3d pos)
 : verkehrsteilnehmer_t(welt, pos)
{
    setze_max_speed(128);

    besch = liste.gib_gewichted(simrand(liste.gib_gesamtgewicht()));

    schritte = strecke[count & 7];
    sum = 0;
    count ++;

    is_sync = false;
}


fussgaenger_t::~fussgaenger_t()
{
  if(is_sync) {
    welt->sync_remove(this);
  }
}


bool fussgaenger_t::sync_step(long delta_t)
{
    // DBG_MESSAGE("fussgaenger_t::sync_step()", "%p called", this);

    verkehrsteilnehmer_t::sync_step(delta_t);

    sum += delta_t;

    return (is_sync = sum < schritte);
}


void fussgaenger_t::rdwr(loadsave_t *file)
{
    verkehrsteilnehmer_t::rdwr(file);

    const char *s = NULL;

    if(!file->is_loading()) {
	s = besch->gib_name();
    }
    file->rdwr_str(s, "N");
    if(file->is_loading()) {

      // besch = liste.gib_gewichted(simrand(liste.gib_gesamtgewicht()));
	besch = table.get(s);

	if(besch == 0) {
	  // Hajo: fallback for missing PAK file

	  besch = liste.at(0);
	}

	guarded_free(const_cast<char *>(s));
    }
}


void fussgaenger_t::calc_bild()
{
    if(welt->lookup(gib_pos())->ist_im_tunnel()) {
	setze_bild(0, -1);
    } else {
	setze_bild(0,
		   besch->gib_bild_nr(ribi_t::gib_dir(gib_fahrtrichtung())));
    }
}
