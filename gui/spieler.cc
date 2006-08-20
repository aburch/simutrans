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
#include <string.h>

#include "../simevent.h"
#include "../simworld.h"
#include "../simplay.h"
#include "../simimg.h"
#include "../simgraph.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../utils/cbuffer_t.h"

#include "gui_label.h"

#include "money_frame.h" // for the finances

#include "spieler.h"


ki_kontroll_t::ki_kontroll_t(karte_t *welt) : infowin_t(welt)
{
    buttons = new vector_tpl<button_t> (16);
    button_t button_def;

    for(int i=0; i<6; i++) {
	button_def.pos.x = 8;
	button_def.pos.y = 34 + i*2*LINESPACE;
	button_def.setze_typ(button_t::square);
	button_def.text= " ";
	buttons->append(button_def);
}
    for(int i=0; i<6; i++) {
	button_def.pos.x = 30;
	button_def.pos.y = 31 + i*2*LINESPACE;
	button_def.groesse.x = 120;
	button_def.groesse.y = 16;
	button_def.setze_typ(button_t::box);
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
	for( int i=2;  i<8; i++ ) {
		char tmp [256];

		sprintf(tmp, "\n                                        %.2f$\n", welt->gib_spieler(i)->gib_konto_als_double() );
		buf.append(tmp);
	}
}


koord ki_kontroll_t::gib_fenstergroesse() const
{
    return koord(260, 180);
}


void ki_kontroll_t::infowin_event(const event_t *ev)
{
	infowin_t::infowin_event(ev);

	if(IS_LEFTCLICK(ev)) {
		for(int i=0; i<12; i++) {
			if(buttons->at(i).getroffen(ev->mx, ev->my)) {
				if(i>=6) {
					buttons->at(i).pressed = false;
					create_win(-1, -1, -1, new money_frame_t(welt->gib_spieler(i-6+2)), w_autodelete );
				}
				else {
					welt->gib_spieler(i+2)->automat = !welt->gib_spieler(i+2)->automat;
					umgebung_t::automaten[i] = welt->gib_spieler(i+2)->automat;
				}
			}
		}
	}
}


vector_tpl<button_t>*
ki_kontroll_t::gib_fensterbuttons()
{
	for(int i=0; i<6; i++) {
		buttons->at(i).pressed = welt->gib_spieler(i+2)->automat;
		// buttons
		buttons->at(6+i).setze_text( welt->gib_spieler(i+2)->gib_name() );
		buttons->at(6+i).background = welt->gib_spieler(i+2)->kennfarbe+3;
	}
	return buttons;
}
