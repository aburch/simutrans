/*
 * spieler.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/* optionen.cc
 *
 * Dialog fuer Automatische Spieler
 * Hj. Malthaner, 2000
 */

#include <stdio.h>

#include "../simevent.h"
#include "../simworld.h"
#include "../simplay.h"
#include "../simimg.h"
#include "../simgraph.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../utils/cbuffer_t.h"
#include "spieler.h"

ki_kontroll_t::ki_kontroll_t(karte_t *welt) : infowin_t(welt)
{
    buttons = new vector_tpl<button_t> (10);
    button_t button_def;

    for(int i=0; i<6; i++) {
	button_def.pos.x = 8;
	button_def.pos.y = 34 + i*2*LINESPACE;
	button_def.setze_typ(button_t::square);
	button_def.text = " ";
	buttons->append(button_def);
    }
}

ki_kontroll_t::~ki_kontroll_t()
{
    delete buttons;
}

const char *
ki_kontroll_t::gib_name() const
{
    return "Spielerliste";
}


/**
 * gibt den Besitzer zurück
 *
 * @author Hj. Malthaner
 */
spieler_t*
ki_kontroll_t::gib_besitzer() const
{
  return welt->gib_spieler(0);
}


int ki_kontroll_t::gib_bild() const
{
    return IMG_LEER;
}


void ki_kontroll_t::info(cbuffer_t & buf) const
{
  char tmp [2048];

  sprintf(tmp,
	  translator::translate("SPIELERLISTE"),
	  welt->gib_spieler(2)->gib_konto_als_double(),
	  welt->gib_spieler(3)->gib_konto_als_double(),
	  welt->gib_spieler(4)->gib_konto_als_double(),
	  welt->gib_spieler(5)->gib_konto_als_double(),
	  welt->gib_spieler(6)->gib_konto_als_double(),
	  welt->gib_spieler(7)->gib_konto_als_double());

  buf.append(tmp);
}


koord ki_kontroll_t::gib_fenstergroesse() const
{
    return koord(160, 180);
}


void ki_kontroll_t::infowin_event(const event_t *ev)
{
    infowin_t::infowin_event(ev);

    if(IS_LEFTCLICK(ev)) {
	for(int i=0; i<6; i++) {
	    if(buttons->at(i).getroffen(ev->mx, ev->my)) {
		welt->gib_spieler(i+2)->automat = !welt->gib_spieler(i+2)->automat;
		umgebung_t::automaten[i] = welt->gib_spieler(i+2)->automat;
	    }
	}
    }
}


vector_tpl<button_t>*
ki_kontroll_t::gib_fensterbuttons()
{
    for(int i=0; i<6; i++) {
	buttons->at(i).pressed = welt->gib_spieler(i+2)->automat;
    }

    return buttons;
}
