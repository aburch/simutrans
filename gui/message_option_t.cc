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
#include "../simskin.h"
#include "../simmesg.h"
#include "../simgraph.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../utils/cbuffer_t.h"

#include "gui_label.h"


#include "message_option_t.h"


message_option_t::message_option_t(karte_t *welt) : infowin_t(welt)
{
    buttons = new vector_tpl<button_t> (3*9);
    button_t button_def;

	message_t::get_instance()->get_flags( &ticker_msg, &window_msg, &auto_msg, &ignore_msg );

	for(int i=0; i<9; i++) {
		button_def.pos.x = 120;
		button_def.pos.y = 34 + i*2*LINESPACE;
		button_def.setze_typ(button_t::square);
		button_def.pressed = (ticker_msg&(1<<i)) != 0;
		button_def.setze_text( "" );
		buttons->append(button_def);

		button_def.pos.x = 140;
		button_def.pos.y = 34 + i*2*LINESPACE;
		button_def.setze_typ(button_t::square);
		button_def.pressed = (auto_msg&(1<<i)) != 0;
		button_def.setze_text( "" );
		buttons->append(button_def);

		button_def.pos.x = 160;
		button_def.pos.y = 34 + i*2*LINESPACE;
		button_def.pressed = (window_msg&(1<<i)) != 0;
		button_def.setze_typ(button_t::square);
		button_def.setze_text( "" );
		buttons->append(button_def);
	}
}

message_option_t::~message_option_t()
{
	delete buttons;
}

int message_option_t::gib_bild() const
{
    return skinverwaltung_t::message_options==NULL?IMG_LEER:skinverwaltung_t::message_options->gib_bild_nr(0);
}


/**
 * Info for buttons
 */
koord
message_option_t::gib_bild_offset() const
{
  return koord(-5,0);
}


void message_option_t::info(cbuffer_t & buf) const
{
	buf.append(translator::translate("MessageOptionsText"));
}


koord message_option_t::gib_fenstergroesse() const
{
    return koord(180, 230);
}


void message_option_t::infowin_event(const event_t *ev)
{
	infowin_t::infowin_event(ev);

	if(IS_LEFTCLICK(ev)) {
		for(int i=0; i<9; i++) {
			if(buttons->at(i*3).getroffen(ev->mx, ev->my)) {
				ticker_msg ^= (1<<i);
			}
			if(buttons->at(i*3+1).getroffen(ev->mx, ev->my)) {
				auto_msg ^= (1<<i);
			}
			if(buttons->at(i*3+2).getroffen(ev->mx, ev->my)) {
				window_msg ^= (1<<i);
			}
		}
		message_t::get_instance()->set_flags( ticker_msg, window_msg, auto_msg, ignore_msg );
	}
}


vector_tpl<button_t>*
message_option_t::gib_fensterbuttons()
{
	for(int i=0; i<9; i++) {
		buttons->at(i*3+0).pressed = (ticker_msg&(1<<i))!=0;
		buttons->at(i*3+1).pressed = (auto_msg&(1<<i))!=0;
		buttons->at(i*3+2).pressed = (window_msg&(1<<i))!=0;
	}
	return buttons;
}
