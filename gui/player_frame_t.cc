/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <string.h>

#include "../simcolor.h"
#include "../simworld.h"
#include "../simmenu.h"
#include "../simtool.h"
#include "../network/network_cmd_ingame.h"
#include "../dataobj/environment.h"
#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"

#include "../gui/simwin.h"
#include "../utils/simstring.h"

#include "money_frame.h" // for the finances
#include "password_frame.h" // for the password
#include "player_frame_t.h"

/* Max Kielland
 * Until gui_label_t has been modified we don't know the size
 * of the fraction part in a localised currency.
 * This is for now hard coded.
 */
#define L_FRACTION_WIDTH (25)
#define L_FINANCE_WIDTH  (max( D_BUTTON_WIDTH, 125 ))
#define L_DIALOG_WIDTH   (300)



ki_kontroll_t::ki_kontroll_t() :
	gui_frame_t( translator::translate("Spielerliste") )
{
	scr_coord cursor = scr_coord ( D_MARGIN_LEFT, D_MARGIN_TOP );

	player_label.set_text("Spieler");
	add_component( &player_label );

	scr_coord_val width = L_FINANCE_WIDTH + D_H_SPACE + D_EDIT_HEIGHT;
	password_label.init("Name/password", cursor, SYSCOL_TEXT, gui_label_t::right);
	password_label.set_size(scr_size(width, D_LABEL_HEIGHT));
	add_component( &password_label );

	access_label.init("Access", cursor);
	add_component( &access_label );

	width = 120;
	cash_label.init("Cash", cursor, SYSCOL_TEXT, gui_label_t::right);
	cash_label.set_size(scr_size(width, D_LABEL_HEIGHT));
	add_component( &cash_label );

	const player_t* const current_player = welt->get_active_player();

	// switching active player allowed?
	bool player_change_allowed = welt->get_settings().get_allow_player_change() || !welt->get_public_player()->is_locked();

	// activate player etc allowed?
	bool player_tools_allowed = true;

	// check also scenario rules
	if (welt->get_scenario()->is_scripted()) {
		player_tools_allowed = welt->get_scenario()->is_tool_allowed(NULL, TOOL_SWITCH_PLAYER | SIMPLE_TOOL);
		player_change_allowed &= player_tools_allowed;
	}

	for(int i=0; i<MAX_PLAYER_COUNT-1; i++) {

		const player_t *const player = welt->get_player(i);

		// AI buttons not available for the two first players (first human and second public)
		if(  i >= 2  ) {
			// AI button (small square)
			player_active[i-2].init(button_t::square_state, "", cursor);
			player_active[i-2].align_to( &player_get_finances[i], ALIGN_CENTER_V );
			player_active[i-2].add_listener(this);
			if(player  &&  player->get_ai_id()!=player_t::HUMAN  &&  player_tools_allowed) {
				add_component( player_active+i-2 );
			}
		}

		// Player select button (arrow)
		player_change_to[i].init(button_t::arrowright_state, "", cursor);
		player_change_to[i].add_listener(this);

		// Allow player change to human and public only (no AI)
		if (player  &&  player_change_allowed) {
			add_component(player_change_to+i);
		}

		// Prepare finances button
		player_get_finances[i].init( button_t::box, "", cursor, scr_size( L_FINANCE_WIDTH, D_EDIT_HEIGHT ) );
		player_get_finances[i].background_color = PLAYER_FLAG | ((player ? player->get_player_color1():i*8)+4);
		player_get_finances[i].add_listener(this);

		// Player type selector, Combobox
		player_select[i].set_focusable( true );

		// Create combobox list data
		player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("slot empty"), SYSCOL_TEXT ) );
		player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Manual (Human)"), SYSCOL_TEXT ) );
		if(  !welt->get_public_player()->is_locked()  ||  !env_t::networkmode  ) {
			player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Goods AI"), SYSCOL_TEXT ) );
			player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Passenger AI"), SYSCOL_TEXT ) );
		}
		assert(  player_t::MAX_AI==4  );

		// When adding new players, activate the interface
		player_select[i].set_selection(welt->get_settings().get_player_type(i));
		player_select[i].add_listener(this);
		if(  player != NULL  ) {
			player_get_finances[i].set_text( player->get_name() );
			add_component( player_get_finances+i );
			player_select[i].set_visible(false);
		}
		else {
			// init player selection dialogue
			if (player_tools_allowed) {
				add_component( player_select+i );
			}
			player_get_finances[i].set_visible(false);
		}

		// password/locked button
		player_lock[i].init(button_t::box, "", cursor, scr_size(D_EDIT_HEIGHT, D_EDIT_HEIGHT));
		player_lock[i].background_color = (player && player->is_locked()) ? (player->is_unlock_pending() ? COL_YELLOW : COL_RED) : COL_GREEN;
		player_lock[i].enable( welt->get_player(i) );
		player_lock[i].add_listener(this);
		if (player_tools_allowed) {
			add_component( player_lock+i );
		}

		// Access buttons
		access_out[i].init(button_t::square_state, "", cursor);
		access_out[i].pressed = player && (!current_player || current_player->allows_access_to(player->get_player_nr()));
		if(access_out[i].pressed && player)
		{
			tooltip_out[i].printf("Withdraw %s's access your ways and stops", player->get_name());
		}
		else if(player)
		{
			tooltip_out[i].printf("Allow %s to access your ways and stops", player->get_name());
		}
		access_out[i].set_tooltip(tooltip_out[i]);
		add_component( access_out+i );
		access_out[i].add_listener(this);
		cursor.x += D_CHECKBOX_WIDTH + 20 + D_H_SPACE;
		access_in[i].init(button_t::square_state, "", cursor);
		access_in[i].pressed = player && player->allows_access_to(current_player->get_player_nr());
		if(access_in[i].pressed && player)
		{
			tooltip_in[i].printf("%s allows you to access its ways and stops", player->get_name());
		}
		else if(player)
		{
			tooltip_in[i].printf("%s does not allow you to access its ways and stops", player->get_name());
		}
		access_in[i].set_tooltip(tooltip_in[i]);
		add_component( access_in+i );

		if(i == welt->get_active_player_nr())
		{
			access_in[i].set_visible(false);
			access_out[i].set_visible(false);
		}

		// Income label
		account_str[i][0] = 0;
		ai_income[i] = new gui_label_t(account_str[i], MONEY_PLUS, gui_label_t::money);
		ai_income[i]->align_to(&player_select[i],ALIGN_CENTER_V);
		add_component( ai_income[i] );

		player_change_to[i].align_to( &player_lock[i], ALIGN_CENTER_V );
		if(  i >= 2  ) {
			player_active[i-2].align_to( &player_lock[i], ALIGN_CENTER_V );
		}
	}

	// freeplay mode
	freeplay.init( button_t::square_state, "freeplay mode", cursor);
	freeplay.add_listener(this);
	if (welt->get_public_player()->is_locked() || !welt->get_settings().get_allow_player_change()  ||  !player_tools_allowed) {
		freeplay.disable();
	}
	freeplay.pressed = welt->get_settings().is_freeplay();
	add_component( &freeplay );

	company_takeovers.set_text(translator::translate("available_company_takeovers:")); // TODO: Set this in Simutranslator
	add_component( &company_takeovers );

	scr_size fincance_size = scr_size(L_FINANCE_WIDTH, D_BUTTON_HEIGHT);

	for (int i = 0; i < MAX_PLAYER_COUNT - 1; i++) {
			take_over_player[i].init(button_t::roundbox, translator::translate("take_over"), cursor, D_BUTTON_SIZE); // TODO: Set this in Simutranslator
			take_over_player[i].add_listener(this);
			take_over_player[i].set_tooltip(translator::translate("take_over_this_company")); // TODO: Set this in Simutranslator
			take_over_player[i].set_visible(false);
			add_component(&take_over_player[i]);

			lb_take_over_player[i].set_visible(false);
			add_component(&lb_take_over_player[i]);

			lb_take_over_cost[i].set_size(fincance_size);
			lb_take_over_cost[i].set_visible(false);
			add_component(&lb_take_over_cost[i]);
	}

	sprintf(text_allow_takeover, "%s", translator::translate("allow_takeover_of_your_company")); // TODO: Set this in Simutranslator
	allow_take_over_of_company.init(button_t::roundbox, text_allow_takeover, cursor, scr_size(display_calc_proportional_string_len_width(text_allow_takeover, -1) + 10 ,D_BUTTON_HEIGHT));
	allow_take_over_of_company.add_listener(this);
	allow_take_over_of_company.set_tooltip(translator::translate("allows_other_players_to_take_over_your_company")); // TODO: Set this in Simutranslator
	if (current_player->get_allow_voluntary_takeover())
	{
		allow_take_over_of_company.disable();
	}
	add_component( &allow_take_over_of_company );

	sprintf(text_cancel_takeover, "%s", translator::translate("cancel"));
	cancel_take_over.init(button_t::roundbox, text_cancel_takeover, cursor, scr_size(display_calc_proportional_string_len_width(text_cancel_takeover, -1) + 10, D_BUTTON_HEIGHT));
	cancel_take_over.add_listener(this);
	cancel_take_over.set_tooltip(translator::translate("cancel_the_takeover_of_your_company")); // TODO: Set this in Simutranslator
	if (!current_player->get_allow_voluntary_takeover())
	{
		cancel_take_over.disable();
	}
	add_component(&cancel_take_over);

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
bool ki_kontroll_t::action_triggered( gui_action_creator_t *comp,value_t p )
{
	static char param[16];

	// Free play button?
	if(  comp==&freeplay  ) {
		welt->call_change_player_tool(karte_t::toggle_freeplay, 255, 0);
		return true;
	}

	// Check the GUI list of buttons
	for(int i=0; i<MAX_PLAYER_COUNT-1; i++) {
		if(i>=2  &&  comp==(player_active+i-2)) {
			// switch AI on/off
			if(  welt->get_player(i)==NULL  ) {
				// create new AI
				welt->call_change_player_tool(karte_t::new_player, i, player_select[i].get_selection());
				player_lock[i].enable( welt->get_player(i) );
			}
			else {
				// Current AI on/off
				sprintf( param, "a,%i,%i", i, !welt->get_player(i)->is_active() );
				tool_t::simple_tool[TOOL_CHANGE_PLAYER]->set_default_param( param );
				welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_PLAYER], welt->get_active_player() );
			}
			break;
		}

		// Finance button pressed
		if(comp==(player_get_finances+i)) {
			// get finances
			player_get_finances[i].pressed = false;
			create_win( new money_frame_t(welt->get_player(i)), w_info, magic_finances_t+welt->get_player(i)->get_player_nr() );
			break;
		}

		// Changed active player
		if(comp==(player_change_to+i)) {
			// make active player (if not in liquidation)
			player_t *const prevplayer = welt->get_active_player();
			welt->switch_active_player(i,false);
			// unlocked public service player can change into any company in multiplayer games
			player_t *const player = welt->get_active_player();
			if (env_t::networkmode  &&  prevplayer == welt->get_public_player() && !prevplayer->is_locked() && player->is_locked()) {
				player->unlock(false, true);

				// send unlock command
				nwc_auth_player_t *nwc = new nwc_auth_player_t();
				nwc->player_nr = player->get_player_nr();
				network_send_server(nwc);

			}
			update_data();
			break;
		}

		// Change player name and/or password
		if(comp==(player_lock+i)  &&  welt->get_player(i)) {
			if (!welt->get_player(i)->is_unlock_pending()) {
				// set password
				create_win( -1, -1, new password_frame_t(welt->get_player(i)), w_info, magic_pwd_t + i );
				player_lock[i].pressed = false;
			}
		}

		// New player assigned in an empty slot
		if(comp==(player_select+i)) {

			// make active player
			remove_component( player_active+i-2 );
			if(  p.i<player_t::MAX_AI  &&  p.i>0  )
			{
				add_component( player_active+i-2 );
				welt->get_settings().set_player_type(i, (uint8)p.i);
			}
			else
			{
				player_select[i].set_selection(0);
				welt->get_settings().set_player_type(i, 0);
			}
			break;
		}

		// Allow access to the selected player
		if(comp == access_out + i)
		{
			access_out[i].pressed =! access_out[i].pressed;
			player_t* player = welt->get_player(i);
			if(access_out[i].pressed && player)
			{
				tooltip_out[i].clear();
				tooltip_out[i].printf("Withdraw %s's access your ways and stops", player->get_name());
			}
			else if(player)
			{
				tooltip_out[i].clear();
				tooltip_out[i].printf("Allow %s to access your ways and stops", player->get_name());
			}

			static char param[16];
			sprintf(param,"g%hu,%i,%i", (uint16)welt->get_active_player_nr(), i, (int)access_out[i].pressed);
			tool_t *tool = create_tool( TOOL_ACCESS_TOOL | SIMPLE_TOOL );
			tool->set_default_param(param);
			welt->set_tool( tool, welt->get_active_player() );
			// since init always returns false, it is save to delete immediately
			delete tool;

			access_out[i].pressed = welt->get_active_player()->allows_access_to(i);
		}

	}

	if (comp == &allow_take_over_of_company)
	{
		static char param[16];
		sprintf(param, "t, %hi, %hi", welt->get_active_player_nr(), true);
		tool_t* tool = create_tool(TOOL_CHANGE_PLAYER | SIMPLE_TOOL);
		tool->set_default_param(param);
		welt->set_tool(tool, welt->get_active_player());
		// since init always returns false, it is save to delete immediately
		delete tool;

		update_data();
	}

	if (comp == &cancel_take_over)
	{
		static char param[16];
		sprintf(param, "t, %hi, %hi", welt->get_active_player_nr(), false);
		tool_t* tool = create_tool(TOOL_CHANGE_PLAYER | SIMPLE_TOOL);
		tool->set_default_param(param);
		welt->set_tool(tool, welt->get_active_player());
		// since init always returns false, it is save to delete immediately
		delete tool;

		update_data();
	}


	for (uint8 i = 0; i < MAX_PLAYER_COUNT - 1; i++) {
		if (comp == take_over_player + i)
		{
			static char param[16];
			sprintf(param, "u, %hu, %hu", (uint16)welt->get_active_player_nr(), (uint16)i);
			tool_t* tool = create_tool(TOOL_CHANGE_PLAYER | SIMPLE_TOOL);
			tool->set_default_param(param);
			welt->set_tool(tool, welt->get_active_player());
			// since init always returns false, it is save to delete immediately
			delete tool;
			take_over_player[i].disable(); // Fail proof, in case the entry stays in the window
		}

	}
	return true;
}


void ki_kontroll_t::update_data()
{
	// ------- Layouting the window ------- //
	scr_coord cursor = scr_coord(D_MARGIN_LEFT, D_MARGIN_TOP);
	scr_coord_val width = L_FINANCE_WIDTH + D_H_SPACE + D_EDIT_HEIGHT;

	player_label.set_pos(cursor);
	cursor.x += D_CHECKBOX_WIDTH + D_H_SPACE;
	cursor.x += D_ARROW_RIGHT_WIDTH + D_H_SPACE;

	password_label.set_pos(cursor);

	cursor.x += width + 10;

	access_label.set_pos(cursor);
	cursor.x += D_CHECKBOX_WIDTH + 20 + D_H_SPACE;
	cursor.x += D_CHECKBOX_WIDTH + D_H_SPACE;

	width = 120;
	cash_label.set_pos(cursor);

	const scr_coord_val window_width = cursor.x + width + D_MARGIN_RIGHT;

	for (int i = 0; i < MAX_PLAYER_COUNT - 1; i++) {
		cursor.y += D_EDIT_HEIGHT + D_V_SPACE;
		cursor.x = D_MARGIN_LEFT;

		if (i >= 2) {
			player_active[i - 2].set_pos(cursor);
		}
		cursor.x += D_CHECKBOX_WIDTH + D_H_SPACE;

		player_change_to[i].set_pos(cursor);
		cursor.x += D_ARROW_RIGHT_WIDTH + D_H_SPACE;

		player_get_finances[i].set_pos(cursor);
		player_select[i].set_pos( cursor );
		player_select[i].set_size(scr_size(L_FINANCE_WIDTH, D_EDIT_HEIGHT));

		cursor.x += L_FINANCE_WIDTH + D_H_SPACE;

		player_lock[i].set_pos(cursor);
		cursor.x += D_EDIT_HEIGHT + 10;


		access_out[i].set_pos(cursor);
		cursor.x += D_CHECKBOX_WIDTH + 20 + D_H_SPACE;

		access_in[i].set_pos(cursor);
		cursor.x += D_CHECKBOX_WIDTH + D_H_SPACE;

		ai_income[i]->set_pos(cursor);
		ai_income[i]->align_to(&player_select[i], ALIGN_CENTER_V);
		player_change_to[i].align_to(&player_lock[i], ALIGN_CENTER_V);
		if (i >= 2) {
			player_active[i - 2].align_to(&player_lock[i], ALIGN_CENTER_V);
		}
	}
	cursor.y += D_EDIT_HEIGHT + D_V_SPACE;
	cursor.x = D_MARGIN_LEFT;

	freeplay.set_pos(cursor);
	cursor.y += max(D_CHECKBOX_HEIGHT, LINESPACE);
	cursor.y += D_BUTTON_HEIGHT * 2;

	company_takeovers.set_pos(cursor);
	cursor.y += D_BUTTON_HEIGHT;

	for (int i = 0; i < MAX_PLAYER_COUNT - 1; i++) {
		const player_t* const player = welt->get_player(i);
		take_over_player[i].set_visible(false);
		lb_take_over_player[i].set_visible(false);
		lb_take_over_cost[i].set_visible(false);

		if (player && player->available_for_takeover())
		{
			if (welt->get_active_player()->is_locked())
			{
				take_over_player[i].disable();
			}
			else
			{
				take_over_player[i].enable();
			}

			take_over_player[i].set_pos(cursor);
			take_over_player[i].set_visible(true);
			cursor.x += D_BUTTON_WIDTH + 10;

			lb_take_over_player[i].set_text(player->get_name());
			lb_take_over_player[i].set_pos(cursor);
			lb_take_over_player[i].set_size(scr_size(L_FINANCE_WIDTH, D_EDIT_HEIGHT));
			lb_take_over_player[i].set_visible(true);
			cursor.x += L_FINANCE_WIDTH + 10;

			lb_take_over_cost[i].set_text(text_take_over_cost[i]);
			lb_take_over_cost[i].set_pos(cursor);
			lb_take_over_cost[i].set_visible(true);

			// Disable our own entry
			if (player == welt->get_active_player()) {
				take_over_player[i].disable();
			}

			// If this company is available to be taken over, disable the take over buttons for the others
			const bool public_player = welt->get_active_player() == welt->get_public_player();
			const bool available_for_takeover = welt->get_active_player()->available_for_takeover();
			if (available_for_takeover || (!public_player && (!welt->get_active_player()->can_afford(player->calc_takeover_cost()))))
			{
				take_over_player[i].disable();
			}

			cursor.y += D_EDIT_HEIGHT + D_V_SPACE;
			cursor.x = D_MARGIN_LEFT;
		}
	}
	cursor.y += D_BUTTON_HEIGHT;

	allow_take_over_of_company.set_pos(cursor);
	cursor.x += allow_take_over_of_company.get_size().w + 5;

	cancel_take_over.set_pos(cursor);
	cursor.y += D_BUTTON_HEIGHT;
	cursor.x = D_MARGIN_LEFT;

	if (!welt->get_active_player()->is_locked())
	{
		allow_take_over_of_company.enable();
	}
	else
	{
		allow_take_over_of_company.disable();
	}
	cancel_take_over.set_visible(true);
	cancel_take_over.disable();
	if (welt->get_active_player()->get_allow_voluntary_takeover())
	{
		allow_take_over_of_company.disable();
		if (!welt->get_active_player()->is_locked())
		{
			cancel_take_over.enable();
		}
	}
	if (welt->get_active_player()->is_public_service())
	{
		allow_take_over_of_company.disable();
		cancel_take_over.set_visible(false);
	}

	set_windowsize( scr_size( window_width, D_TITLEBAR_HEIGHT + cursor.y + D_MARGIN_BOTTOM ) );
	// ------- Layouting done ------- //


	// Now update the upper Player buttons
	for(int i=0; i<MAX_PLAYER_COUNT-1; i++) {

		if(  player_t *player = welt->get_player(i)  ) {

			// active player -> remove selection
			if (player_select[i].is_visible())
			{
				player_select[i].set_visible(false);
				player_get_finances[i].set_visible(true);
				add_component(player_get_finances+i);
				if (welt->get_settings().get_allow_player_change() || !welt->get_public_player()->is_locked())
				{
					add_component(player_change_to+i);
				}
			}
			player_get_finances[i].set_text(player->get_name());

			access_in[i].pressed = player && player->allows_access_to(welt->get_active_player_nr());
			if(access_in[i].pressed && player)
			{
				tooltip_in[i].clear();
				tooltip_in[i].printf("%s allows you to access its ways and stops", player->get_name());
			}
			else if(player)
			{
				tooltip_in[i].clear();
				tooltip_in[i].printf("%s does not allow you to access its ways and stops", player->get_name());
			}
			access_out[i].pressed = player && welt->get_active_player()->allows_access_to(player->get_player_nr());
			if(access_out[i].pressed && player)
			{
				tooltip_out[i].clear();
				tooltip_out[i].printf("Withdraw %s's access your ways and stops", player->get_name());
			}
			else if(player)
			{
				tooltip_out[i].clear();
				tooltip_out[i].printf("%s does not allow you to access its ways and stops", player->get_name());
			}

			if(welt->get_active_player_nr() == i)
			{
				access_in[i].set_visible(false);
				access_out[i].set_visible(false);
			}
			else
			{
				access_in[i].set_visible(true);
				access_out[i].set_visible(true);
			}

			// always update locking status
			player_get_finances[i].background_color = PLAYER_FLAG | (player->get_player_color1()+4);
			player_lock[i].background_color = player->is_locked() ? (player->is_unlock_pending() ? COL_YELLOW : COL_RED) : COL_GREEN;

			// human players cannot be deactivated
			if (i>1)
			{
				remove_component( player_active+i-2 );
				if(  player->get_ai_id()!=player_t::HUMAN  )
				{
					add_component( player_active+i-2 );
				}
			}
		}
		else {

			// inactive player => button needs removal?
			access_in[i].set_visible(false);
			access_out[i].set_visible(false);
			if (player_get_finances[i].is_visible())
			{
				player_get_finances[i].set_visible(false);
				remove_component(player_get_finances+i);
				remove_component(player_change_to+i);
				player_select[i].set_visible(true);
			}

			if (i>1) {
				remove_component( player_active+i-2 );
				if(  0<player_select[i].get_selection()  &&  player_select[i].get_selection()<player_t::MAX_AI)
				{
					add_component( player_active+i-2 );
				}
			}

			if(  env_t::networkmode  ) {

				// change available selection of AIs
				if(  !welt->get_public_player()->is_locked()  )
				{
					if(  player_select[i].count_elements()==2  )
					{
						player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Goods AI"), SYSCOL_TEXT ) );
						player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Passenger AI"), SYSCOL_TEXT ) );
					}
				}
				else
				{
					if(  player_select[i].count_elements()==4  )
					{
						player_select[i].clear_elements();
						player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("slot empty"), SYSCOL_TEXT ) );
						player_select[i].append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("Manual (Human)"), SYSCOL_TEXT ) );
					}
				}

			}
		}
	}
}


/**
 * Draw the component
 * @author Hj. Malthaner
 */
void ki_kontroll_t::draw(scr_coord pos, scr_size size)
{
	// Update free play
	freeplay.pressed = welt->get_settings().is_freeplay();
	if (welt->get_public_player()->is_locked() || !welt->get_settings().get_allow_player_change()) {
		freeplay.disable();
	}
	else {
		freeplay.enable();
	}

	// Update finance
	for(int i=0; i<MAX_PLAYER_COUNT-1; i++) {

		player_change_to[i].pressed = false;
		if(i>=2) {
			player_active[i-2].pressed = welt->get_player(i) !=NULL  &&  welt->get_player(i)->is_active();
		}

		player_t *player = welt->get_player(i);
		player_lock[i].background_color = player  &&  player->is_locked() ? (player->is_unlock_pending() ? COL_YELLOW : COL_RED) : COL_GREEN;


		if(player != NULL)
		{
			const player_t::solvency_status ss = player->check_solvency();
			if (ss == player_t::in_liquidation)
			{
				ai_income[i]->set_color(COL_DARK_RED);
				tstrncpy(account_str[i], translator::translate("in_liquidation"), lengthof(account_str[i]));
			}
			else if (ss == player_t::in_administration)
			{
				ai_income[i]->set_color(COL_DARK_BLUE);
				tstrncpy(account_str[i], translator::translate("in_administration"), lengthof(account_str[i]));
			}
			else
			{
				double account=player->get_account_balance_as_double();
				money_to_string(account_str[i], account );
				ai_income[i]->set_color( account>=0.0 ? MONEY_PLUS : MONEY_MINUS );
			}
			ai_income[i]->set_pos( scr_coord( size.w-D_MARGIN_RIGHT-L_FRACTION_WIDTH, ai_income[i]->get_pos().y ) );

			access_out[i].pressed = welt->get_active_player()->allows_access_to(i);
			if(access_out[i].pressed && player)
			{
				tooltip_out[i].clear();
				tooltip_out[i].printf("Withdraw %s's access your ways and stops", player->get_name());
			}
			else if(player)
			{
				tooltip_out[i].clear();
				tooltip_out[i].printf("Allow %s to access your ways and stops", player->get_name());
			}

		}
		else {
			account_str[i][0] = 0;
		}
	}

	player_change_to[welt->get_active_player_nr()].pressed = true;

	// Update take over money entry
	for (int i = 0; i < MAX_PLAYER_COUNT - 1; i++) {
		player_t* player = welt->get_player(i);
		if (player && player->available_for_takeover()) {
			money_to_string(text_take_over_cost[i], player->calc_takeover_cost() / 100, true);
		}
	}

	// All controls updated, draw them...
	gui_frame_t::draw(pos, size);
}
