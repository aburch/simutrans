/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Dialog fuer Automatische Spieler
 * Hj. Malthaner, 2000
 */

#include <stdio.h>
#include <string.h>

#include "components/list_button.h"

#include "../simcolor.h"
#include "../simworld.h"
#include "../dataobj/umgebung.h"

#include "../simplay.h"
#include "../utils/simstring.h"

#include "money_frame.h" // for the finances
#include "player_frame_t.h"

ki_kontroll_t::ki_kontroll_t(karte_t *wl) :
	gui_frame_t("Spielerliste")
{
	this->welt = wl;

	for(int i=0; i<MAX_PLAYER_COUNT; i++) {

	    player_change_to[i].init(button_t::arrowright, " ", koord(16+4,6+i*2*LINESPACE), koord(10,BUTTON_HEIGHT));
	    player_change_to[i].add_listener(this);
	    add_komponente(player_change_to+i);

		if(i>=2) {
			player_active[i-2].init(button_t::square, " ", koord(4,6+i*2*LINESPACE), koord(10,BUTTON_HEIGHT));
			player_active[i-2].add_listener(this);
			player_active[i-2].pressed = umgebung_t::automaten[i-2];
			add_komponente( player_active+i-2 );
		}

		player_get_finances[i].init(button_t::box, welt->gib_spieler(i)->gib_name(), koord(34,4+i*2*LINESPACE), koord(120,BUTTON_HEIGHT));
		player_get_finances[i].background =PLAYER_FLAG|(welt->gib_spieler(i)->get_player_color1()+4 );
		player_get_finances[i].add_listener(this);
		add_komponente( player_get_finances+i );

		ai_income[i] = new gui_label_t(account_str[i], MONEY_PLUS, gui_label_t::money);
		ai_income[i]->setze_pos( koord( 225, 8+i*2*LINESPACE ) );
		add_komponente( ai_income[i] );
	}
	setze_fenstergroesse(koord(260, MAX_PLAYER_COUNT*LINESPACE*2+16));
}



ki_kontroll_t::~ki_kontroll_t()
{
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		delete ai_income[i];
	}
}



/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool
ki_kontroll_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(i>=2  &&  komp==(player_active+i-2)) {
			// switch AI on/off
			umgebung_t::automaten[i-2] = welt->gib_spieler(i)->set_active( !welt->gib_spieler(i)->is_active() );
			break;
		}
		if(komp==(player_get_finances+i)) {
			// get finances
			player_get_finances[i].pressed = false;
			welt->gib_spieler(i)->zeige_info();
			break;
		}
		if(komp==(player_change_to+i)) {
			// make active player
			welt->switch_active_player(i);
			break;
		}
	}
	return true;
}



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void
ki_kontroll_t::zeichnen(koord pos, koord gr)
{
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {

		player_change_to[i].pressed = false;

		if(i<MAX_PLAYER_COUNT-2) {
			player_active[i].pressed = umgebung_t::automaten[i];
		}

		double account=welt->gib_spieler(i)->gib_konto_als_double();
		money_to_string(account_str[i], account );
		ai_income[i]->set_color( account>=0.0 ? MONEY_PLUS : MONEY_MINUS );
	}

	player_change_to[welt->get_active_player_nr()].pressed = true;

	gui_frame_t::zeichnen(pos, gr);
}
