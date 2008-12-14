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
#include "../dataobj/translator.h"

#include "../simwin.h"
#include "../utils/simstring.h"

#include "money_frame.h" // for the finances
#include "player_frame_t.h"


karte_t *ki_kontroll_t::welt = NULL;


ki_kontroll_t::ki_kontroll_t(karte_t *wl) :
	gui_frame_t("Spielerliste")
{
	this->welt = wl;

	for(int i=0; i<MAX_PLAYER_COUNT-1; i++) {

		player_change_to[i].init(button_t::arrowright_state, " ", koord(16+4,6+i*2*LINESPACE), koord(10,BUTTON_HEIGHT));
		player_change_to[i].add_listener(this);

		ai_income[i] = new gui_label_t(account_str[i], MONEY_PLUS, gui_label_t::money);
		ai_income[i]->setze_pos( koord( 225, 8+i*2*LINESPACE ) );

		if(i>=2) {
			player_active[i-2].init(button_t::square_state, " ", koord(4,6+i*2*LINESPACE), koord(10,BUTTON_HEIGHT));
			player_active[i-2].add_listener(this);
			if(  welt->gib_einstellungen()->get_player_type(i)!=spieler_t::EMPTY  ) {
				add_komponente( player_active+i-2 );
			}
		}

		if(welt->gib_spieler(i)!=NULL  &&  welt->gib_einstellungen()->gib_allow_player_change()) {
			// allow change to human and public
			add_komponente(player_change_to+i);
		}

		player_get_finances[i].init(button_t::box, "", koord(34,4+i*2*LINESPACE), koord(120,BUTTON_HEIGHT));
		player_get_finances[i].background =PLAYER_FLAG|((wl->gib_spieler(i)?welt->gib_spieler(i)->get_player_color1():i*8)+4);
		player_get_finances[i].add_listener(this);
		player_select[i].setze_pos( koord(34,4+i*2*LINESPACE) );
		player_select[i].setze_groesse( koord(120,BUTTON_HEIGHT) );
		player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("slot empty"), COL_BLACK ) );
		player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Manual (Human)"), COL_BLACK ) );
		player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Goods AI"), COL_BLACK ) );
		player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Passenger AI"), COL_BLACK ) );
		assert(  spieler_t::MAX_AI==4  );
		// when adding new players, add a name here ...
		player_select[i].set_selection(welt->gib_einstellungen()->get_player_type(i) );
		player_select[i].add_listener(this);
		if(  welt->gib_spieler(i)!=NULL  ) {
			player_get_finances[i].setze_text( welt->gib_spieler(i)->gib_name() );
			add_komponente( player_get_finances+i );
		}
		else {
			// init player selection dialoge
			add_komponente( player_select+i );
		}

		add_komponente( ai_income[i] );
	}

	// freeplay mode
	freeplay.init( button_t::square_state, "freeplay mode", koord(4,2+(MAX_PLAYER_COUNT-1)*LINESPACE*2) );
	if(  !welt->gib_einstellungen()->gib_allow_player_change()  ) {
		freeplay.disable();
	}
	else {
		freeplay.add_listener(this);
	}
	freeplay.pressed = welt->gib_einstellungen()->is_freeplay();
	add_komponente( &freeplay );

	setze_fenstergroesse(koord(260, (MAX_PLAYER_COUNT-1)*LINESPACE*2+16+14+4));
}



ki_kontroll_t::~ki_kontroll_t()
{
	for(int i=0; i<MAX_PLAYER_COUNT-1; i++) {
		delete ai_income[i];
	}
}



/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool ki_kontroll_t::action_triggered( gui_action_creator_t *komp,value_t p )
{
	if(  komp==&freeplay  ) {
		welt->gib_einstellungen()->setze_freeplay( !welt->gib_einstellungen()->is_freeplay() );
		freeplay.pressed = welt->gib_einstellungen()->is_freeplay();
		return true;
	}

	for(int i=0; i<MAX_PLAYER_COUNT-1; i++) {
		if(i>=2  &&  komp==(player_active+i-2)) {
			// switch AI on/off
			if(  welt->gib_spieler(i)==NULL  ) {
				welt->new_spieler( i, player_select[i].get_selection() );
				welt->gib_einstellungen()->set_player_type( i, welt->gib_spieler(i)->get_ai_id() );
				remove_komponente( player_select+i );
				add_komponente( player_change_to+i );
				if(  welt->gib_spieler(i)->get_ai_id()==spieler_t::HUMAN  ) {
					remove_komponente( player_active+i-2 );
				}
				player_get_finances[i].setze_text( welt->gib_spieler(i)->gib_name() );
				add_komponente( player_get_finances+i );
				welt->gib_spieler(i)->set_active( true );
			}
			else {
				welt->gib_spieler(i)->set_active( !welt->gib_spieler(i)->is_active() );
			}
			welt->gib_einstellungen()->set_player_active( i, welt->gib_spieler(i)->is_active() );
			break;
		}
		if(komp==(player_get_finances+i)) {
			// get finances
			player_get_finances[i].pressed = false;
			create_win( new money_frame_t(welt->gib_spieler(i)), w_info, (long)welt->gib_spieler(i) );
			break;
		}
		if(komp==(player_change_to+i)) {
			// make active player
			welt->switch_active_player(i);
			break;
		}
		if(komp==(player_select+i)) {
			// make active player
			remove_komponente( player_active+i-2 );
			if(  p.i<spieler_t::MAX_AI  &&  p.i>0  ) {
				add_komponente( player_active+i-2 );
				welt->gib_einstellungen()->set_player_type( i, p.i );
			}
			else {
				player_select[i].set_selection(0);
				welt->gib_einstellungen()->set_player_type( i, 0 );
			}
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
	for(int i=0; i<MAX_PLAYER_COUNT-1; i++) {

		player_change_to[i].pressed = false;

		if(i>=2) {
			player_active[i-2].pressed = welt->gib_spieler(i)!=NULL  &&  welt->gib_spieler(i)->is_active();
		}

		spieler_t *sp = welt->gib_spieler(i);
		if(  sp!=NULL  ) {
			if(i!=1  &&  sp->get_finance_history_year(0, COST_NETWEALTH)<0) {
				ai_income[i]->set_color( MONEY_MINUS );
				tstrncpy(account_str[i], translator::translate("Company bankrupt"), 31 );
			}
			else {
				double account=sp->gib_konto_als_double();
				money_to_string(account_str[i], account );
				ai_income[i]->set_color( account>=0.0 ? MONEY_PLUS : MONEY_MINUS );
			}
		}
		else {
			account_str[i][0] = 0;
		}
	}

	player_change_to[welt->get_active_player_nr()].pressed = true;

	gui_frame_t::zeichnen(pos, gr);
}
