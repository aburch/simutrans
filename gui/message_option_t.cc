/*
 * Settings for player message options
 *
 * Copyright (c) 2006 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simskin.h"
#include "../besch/skin_besch.h"
#include "../simmesg.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "message_option_t.h"


karte_t *message_option_t::welt = NULL;


message_option_t::message_option_t(karte_t *welt) :
	gui_frame_t("Mailbox Options"),
	text_label(translator::translate("MessageOptionsText")),
	legend( skinverwaltung_t::message_options->gib_bild_nr(0) )
{
	this->welt = welt;
	text_label.setze_pos( koord(10,-2) );
	add_komponente( &text_label );

	legend.setze_pos( koord(110,0) );
	add_komponente( &legend );

	welt->get_message()->get_message_flags( &ticker_msg, &window_msg, &auto_msg, &ignore_msg );

	for(int i=0; i<MAX_MESSAGE_TYP; i++) {
		buttons[i*3].pos = koord(120,18+i*2*LINESPACE);
		buttons[i*3].setze_typ(button_t::square_state);
		buttons[i*3].pressed = (ticker_msg>>i)&1;
		buttons[i*3].add_listener(this);
		add_komponente( buttons+i*3 );

		buttons[i*3+1].pos = koord(140,18+i*2*LINESPACE);
		buttons[i*3+1].setze_typ(button_t::square_state);
		buttons[i*3+1].pressed = (auto_msg>>i)&1;
		buttons[i*3+1].add_listener(this);
		add_komponente( buttons+i*3+1 );

		buttons[i*3+2].pos = koord(160,18+i*2*LINESPACE);
		buttons[i*3+2].setze_typ(button_t::square_state);
		buttons[i*3+2].pressed = (window_msg>>i)&1;
		buttons[i*3+2].add_listener(this);
		add_komponente( buttons+i*3+2 );
	}
	setze_fenstergroesse( koord(180, 248) );
}



bool
message_option_t::action_triggered(gui_komponente_t *komp, value_t )
{
	((button_t*)komp)->pressed ^= 1;
	for(int i=0; i<MAX_MESSAGE_TYP; i++) {
		if(&buttons[i*3]==komp) {
			ticker_msg ^= (1<<i);
		}
		if(&buttons[i*3+1]==komp) {
			auto_msg ^= (1<<i);
		}
		if(&buttons[i*3+2]==komp) {
			window_msg ^= (1<<i);
		}
	}
	welt->get_message()->set_message_flags( ticker_msg, window_msg, auto_msg, ignore_msg );
	return true;
}
