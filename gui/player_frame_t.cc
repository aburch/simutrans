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



#include "../simcolor.h"
#include "../simworld.h"
#include "../simmenu.h"
#include "../dataobj/network_cmd_ingame.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"

#include "../simwin.h"
#include "../utils/simstring.h"

#include "money_frame.h" // for the finances
#include "password_frame.h" // for the password
#include "player_frame_t.h"


karte_t *ki_kontroll_t::welt = NULL;


ki_kontroll_t::ki_kontroll_t(karte_t *wl) :
	gui_frame_t( translator::translate("Spielerliste") )
{
	this->welt = wl;

	// switching active player allowed?
	bool player_change_allowed = welt->get_settings().get_allow_player_change() || !welt->get_spieler(1)->is_locked();
	// activate player etc allowed?
	bool player_tools_allowed = true;
	// check also scenario rules
	if (welt->get_scenario()->is_scripted()) {
		player_tools_allowed = welt->get_scenario()->is_tool_allowed(NULL, WKZ_SWITCH_PLAYER | SIMPLE_TOOL);
		player_change_allowed &= player_tools_allowed;
	}

	for(int i=0; i<MAX_PLAYER_COUNT-1; i++) {
		const spieler_t *const sp = welt->get_spieler(i);

		player_change_to[i].init(button_t::arrowright_state, " ", koord(16+4,6+i*2*LINESPACE), koord(10,D_BUTTON_HEIGHT));
		player_change_to[i].add_listener(this);

		if(i>=2) {
			player_active[i-2].init(button_t::square_state, "", koord(4,6+i*2*LINESPACE));
			player_active[i-2].add_listener(this);
			if(sp  &&  sp->get_ai_id()!=spieler_t::HUMAN  &&  player_tools_allowed) {
				add_komponente( player_active+i-2 );
			}
		}

		if (sp  &&  player_change_allowed) {
			// allow change to human and public
			add_komponente(player_change_to+i);
		}

		// finances button
		player_get_finances[i].init(button_t::box, "", koord(34,4+i*2*LINESPACE), koord(120,D_BUTTON_HEIGHT));
		player_get_finances[i].background = PLAYER_FLAG|((sp ? sp->get_player_color1():i*8)+4);
		player_get_finances[i].add_listener(this);

		player_select[i].set_pos( koord(34,4+i*2*LINESPACE) );
		player_select[i].set_groesse( koord(120,D_BUTTON_HEIGHT) );
		player_select[i].set_focusable( false );
		player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("slot empty"), COL_BLACK ) );
		player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Manual (Human)"), COL_BLACK ) );
		if(  !welt->get_spieler(1)->is_locked()  ||  !umgebung_t::networkmode  ) {
			player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Goods AI"), COL_BLACK ) );
			player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Passenger AI"), COL_BLACK ) );
		}
		assert(  spieler_t::MAX_AI==4  );

		// when adding new players, add a name here ...
		player_select[i].set_selection(welt->get_settings().get_player_type(i));
		player_select[i].add_listener(this);
		if(  sp!=NULL  ) {
			player_get_finances[i].set_text( sp->get_name() );
			add_komponente( player_get_finances+i );
			player_select[i].set_visible(false);
		}
		else {
			// init player selection dialoge
			if (player_tools_allowed) {
				add_komponente( player_select+i );
			}
			player_get_finances[i].set_visible(false);
		}

		// password/locked button
		player_lock[i].init(button_t::box, "", koord(160+1,4+i*2*LINESPACE+1), koord(D_BUTTON_HEIGHT-2,D_BUTTON_HEIGHT-2));
		player_lock[i].background = sp  &&  sp->is_locked() ? (sp->is_unlock_pending() ? COL_YELLOW : COL_RED) : COL_GREEN;
		player_lock[i].add_listener(this);
		if (player_tools_allowed) {
			add_komponente( player_lock+i );
		}

		// income label
		account_str[i][0] = 0;
		ai_income[i] = new gui_label_t(account_str[i], MONEY_PLUS, gui_label_t::money);
		ai_income[i]->set_pos( koord( 261, 8+i*2*LINESPACE ) );
		add_komponente( ai_income[i] );
	}

	// freeplay mode
	freeplay.init( button_t::square_state, "freeplay mode", koord(4,2+(MAX_PLAYER_COUNT-1)*LINESPACE*2) );
	freeplay.add_listener(this);
	if (welt->get_spieler(1)->is_locked() || !welt->get_settings().get_allow_player_change()  ||  !player_tools_allowed) {
		freeplay.disable();
	}
	freeplay.pressed = welt->get_settings().is_freeplay();
	add_komponente( &freeplay );

	set_fenstergroesse(koord(295, (MAX_PLAYER_COUNT-1)*LINESPACE*2+16+14+4));
	update_data();
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
	static char param[16];

	if(  komp==&freeplay  ) {
		welt->call_change_player_tool(karte_t::toggle_freeplay, 255, 0);
		return true;
	}

	for(int i=0; i<MAX_PLAYER_COUNT-1; i++) {
		if(i>=2  &&  komp==(player_active+i-2)) {
			// switch AI on/off
			if(  welt->get_spieler(i)==NULL  ) {
				// create
				welt->call_change_player_tool(karte_t::new_player, i, player_select[i].get_selection());
			}
			else {
				// activate
				sprintf( param, "a,%i,%i", i, !welt->get_spieler(i)->is_active() );
				werkzeug_t::simple_tool[WKZ_SET_PLAYER_TOOL]->set_default_param( param );
				welt->set_werkzeug( werkzeug_t::simple_tool[WKZ_SET_PLAYER_TOOL], welt->get_active_player() );
			}
			break;
		}
		if(komp==(player_get_finances+i)) {
			// get finances
			player_get_finances[i].pressed = false;
			create_win( new money_frame_t(welt->get_spieler(i)), w_info, magic_finances_t+welt->get_spieler(i)->get_player_nr() );
			break;
		}
		if(komp==(player_change_to+i)) {
			// make active player
			welt->switch_active_player(i,false);
			break;
		}
		if(komp==(player_lock+i)  &&  welt->get_spieler(i)) {
			if (!welt->get_spieler(i)->is_unlock_pending()) {
				// set password
				create_win( -1, -1, new password_frame_t(welt->get_spieler(i)), w_info, magic_pwd_t + i );
				player_lock[i].pressed = false;
			}
		}
		if(komp==(player_select+i)) {
			// make active player
			remove_komponente( player_active+i-2 );
			if(  p.i<spieler_t::MAX_AI  &&  p.i>0  ) {
				add_komponente( player_active+i-2 );
				welt->get_settings().set_player_type(i, (uint8)p.i);
			}
			else {
				player_select[i].set_selection(0);
				welt->get_settings().set_player_type(i, 0);
			}
			break;
		}
	}
	return true;
}


void ki_kontroll_t::update_data()
{
	for(int i=0; i<MAX_PLAYER_COUNT-1; i++) {
		if(  spieler_t *sp = welt->get_spieler(i)  ) {
			// active player -> remove selection
			if (player_select[i].is_visible()) {
				player_select[i].set_visible(false);
				player_get_finances[i].set_visible(true);
				add_komponente(player_get_finances+i);
				if (welt->get_settings().get_allow_player_change() || !welt->get_spieler(1)->is_locked()) {
					add_komponente(player_change_to+i);
				}
				player_get_finances[i].set_text(sp->get_name());
			}
			// always update locking status
			player_get_finances[i].background = PLAYER_FLAG | (sp->get_player_color1()+4);
			player_lock[i].background = sp->is_locked() ? (sp->is_unlock_pending() ? COL_YELLOW : COL_RED) : COL_GREEN;
			// human players cannot be deactivated
			if (i>1) {
				remove_komponente( player_active+i-2 );
				if(  sp->get_ai_id()!=spieler_t::HUMAN  ) {
					add_komponente( player_active+i-2 );
				}
			}
		}
		else {
			// inactive player => button needs removal?
			if (player_get_finances[i].is_visible()) {
				player_get_finances[i].set_visible(false);
				remove_komponente(player_get_finances+i);
				remove_komponente(player_change_to+i);
				player_select[i].set_visible(true);
			}
			if (i>1) {
				remove_komponente( player_active+i-2 );
				if(  0<player_select[i].get_selection()  &&  player_select[i].get_selection()<spieler_t::MAX_AI) {
					add_komponente( player_active+i-2 );
				}
			}
			if(  umgebung_t::networkmode  ) {
				// change available selection of AIs
				if(  !welt->get_spieler(1)->is_locked()  ) {
					if(  player_select[i].count_elements()==2  ) {
						player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Goods AI"), COL_BLACK ) );
						player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Passenger AI"), COL_BLACK ) );
					}
				}
				else {
					if(  player_select[i].count_elements()==4  ) {
						player_select[i].clear_elements();
						player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("slot empty"), COL_BLACK ) );
						player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Manual (Human)"), COL_BLACK ) );
					}
				}
			}
		}
	}
}


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void ki_kontroll_t::zeichnen(koord pos, koord gr)
{
	freeplay.pressed = welt->get_settings().is_freeplay();
	if (welt->get_spieler(1)->is_locked() || !welt->get_settings().get_allow_player_change()) {
		freeplay.disable();
	}
	else {
		freeplay.enable();
	}

	for(int i=0; i<MAX_PLAYER_COUNT-1; i++) {

		player_change_to[i].pressed = false;

		if(i>=2) {
			player_active[i-2].pressed = welt->get_spieler(i)!=NULL  &&  welt->get_spieler(i)->is_active();
		}

		spieler_t *sp = welt->get_spieler(i);

		player_lock[i].background = sp  &&  sp->is_locked() ? (sp->is_unlock_pending() ? COL_YELLOW : COL_RED) : COL_GREEN;

		if(  sp!=NULL  ) {
			if (i != 1 && !welt->get_settings().is_freeplay() && sp->get_finance_history_year(0, COST_NETWEALTH) < 0) {
				ai_income[i]->set_color( MONEY_MINUS );
				ai_income[i]->set_pos( koord( gr.x-4, 8+i*2*LINESPACE ) );
				tstrncpy(account_str[i], translator::translate("Company bankrupt"), lengthof(account_str[i]));
			}
			else {
				double account=sp->get_konto_als_double();
				money_to_string(account_str[i], account );
				ai_income[i]->set_color( account>=0.0 ? MONEY_PLUS : MONEY_MINUS );
				ai_income[i]->set_pos( koord( 261, 8+i*2*LINESPACE ) );
			}
		}
		else {
			account_str[i][0] = 0;
		}
	}

	player_change_to[welt->get_active_player_nr()].pressed = true;

	gui_frame_t::zeichnen(pos, gr);
}
