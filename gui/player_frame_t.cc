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

	for(int i=0; i<MAX_PLAYER_COUNT-2; i++) {
		player_active[i].init(button_t::square, " ", koord(8,6+i*2*LINESPACE), koord(BUTTON_HEIGHT,BUTTON_HEIGHT));
		player_active[i].add_listener(this);
		player_active[i].pressed = umgebung_t::automaten[i];
		add_komponente( player_active+i );

		player_get_finances[i].init(button_t::box, welt->gib_spieler(i+2)->gib_name(), koord(30,4+i*2*LINESPACE), koord(120,BUTTON_HEIGHT));
		player_get_finances[i].background = welt->gib_spieler(i+2)->kennfarbe+3;
		player_get_finances[i].add_listener(this);
		add_komponente( player_get_finances+i );

		ai_income[i] = new gui_label_t(account_str[i], MONEY_PLUS, gui_label_t::money);
		ai_income[i]->setze_pos( koord( 225, 8+i*2*LINESPACE ) );
		add_komponente( ai_income[i] );
	}
	setze_opaque(true);
	setze_fenstergroesse(koord(260, MAX_PLAYER_COUNT*LINESPACE*2-16));
}



/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool
ki_kontroll_t::action_triggered(gui_komponente_t *komp)
{
	for(int i=0; i<MAX_PLAYER_COUNT-2; i++) {
		if(komp==(player_active+i)) {
			umgebung_t::automaten[i] = welt->gib_spieler(i+2)->set_active( !welt->gib_spieler(i+2)->is_active() );
			break;
		}
		if(komp==(player_get_finances+i)) {
			player_get_finances[i].pressed = false;
			create_win(-1, -1, -1, welt->gib_spieler(i+2)->gib_money_frame(), w_info );
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
	for(int i=0; i<MAX_PLAYER_COUNT-2; i++) {
		player_active[i].pressed = umgebung_t::automaten[i];
		double account=welt->gib_spieler(i+2)->gib_konto_als_double();
		money_to_string(account_str[i], account );
		ai_income[i]->set_color( account>=0.0 ? MONEY_PLUS : MONEY_MINUS );
	}

	gui_frame_t::zeichnen(pos, gr);
}
