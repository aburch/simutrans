/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "simwin.h"
#include "../simworld.h"
#include "../simmenu.h"

#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "message_frame_t.h"
#include "../simmesg.h"
#include "message_option_t.h"
#include "../network/network_cmd_ingame.h"
#include "../player/simplay.h"


#include "components/action_listener.h"


#define MAX_MESG_TABS (8)


static sint32 categories[MAX_MESG_TABS] =
{
	(1 << message_t::chat),
	(1 << message_t::scenario),
	(1 << message_t::problems),
	(1 << message_t::traffic_jams) | (1 << message_t::warnings),
	(1 << message_t::full),
	(1 << message_t::city) | (1 << message_t::industry),
	(1 << message_t::ai),
	(1 << message_t::general) | (1 << message_t::new_vehicle)
};

static char const* const tab_strings[]=
{
	"Chat_msg",
	"Scenario_msg",
	"Problems_msg",
	"Warnings_msg",
	"Station_msg",
	"Town_msg",
	"Company_msg",
	"Game_msg"
};



message_frame_t::message_frame_t() :
	gui_frame_t( translator::translate("Mailbox") ),
	stats(),
	scrolly(&stats)
{
	ibuf[0] = 0;

	set_table_layout(1,0);

	add_table(2,0);
	{
		option_bt.init(button_t::roundbox, translator::translate("Optionen"));
		option_bt.add_listener(this);
		add_component(&option_bt);

		if(  env_t::networkmode  ) {
			input.set_text(ibuf, lengthof(ibuf) );
			input.add_listener(this);
			add_component(&input);
			set_focus( &input );
		}
	}
	end_table();

	scrolly.set_show_scroll_x(true);
	scrolly.set_scroll_amount_y(LINESPACE+1);

	// Knightly : add tabs for classifying messages
	tabs.add_tab( &scrolly, translator::translate("All") );
	tab_categories.append( -1 );

	if (env_t::networkmode) {
		tabs.add_tab( &scrolly, translator::translate(tab_strings[0]) );
		tab_categories.append( categories[0] );
	}
	if (welt->get_scenario()->is_scripted()) {
		tabs.add_tab( &scrolly, translator::translate(tab_strings[1]) );
		tab_categories.append( categories[1] );
	}
	for(  int i=2;  i<MAX_MESG_TABS;  ++i  ) {
		tabs.add_tab( &scrolly, translator::translate(tab_strings[i]) );
		tab_categories.append( categories[i] );
	}
	tabs.add_listener(this);
	add_component(&tabs);

	set_resizemode(diagonal_resize);
	if(  env_t::networkmode  ) {
		set_transparent( env_t::chat_window_transparency, color_idx_to_rgb(COL_WHITE) );
	}

	reset_min_windowsize();
	set_windowsize(scr_size(D_DEFAULT_WIDTH, D_DEFAULT_HEIGHT));
}


/* triggered, when button clicked; only single button registered, so the action is clear ... */
bool message_frame_t::action_triggered( gui_action_creator_t *comp, value_t v )
{
	if(  comp==&option_bt  ) {
		create_win(320, 200, new message_option_t(), w_info, magic_message_options );
	}
	else if(  comp==&input  &&  ibuf[0]!=0  ) {
		// Send chat message to server for distribution
		nwc_chat_t* nwchat = new nwc_chat_t( ibuf, welt->get_active_player()->get_player_nr(), env_t::nickname.c_str() );
		network_send_server( nwchat );

		ibuf[0] = 0;
	}
	else if(  comp==&tabs  ) {
		// Knightly : filter messages by type where necessary
		if(  stats.filter_messages( tab_categories[v.i] )  ) {
			scrolly.set_scroll_position(0, 0);
		}
	}
	return true;
}


void message_frame_t::rdwr(loadsave_t *file)
{
	// window size
	scr_size size = get_windowsize();
	size.rdwr( file );

	scrolly.rdwr(file);
	tabs.rdwr(file);
	file->rdwr_str( ibuf, lengthof(ibuf) );

	if(  file->is_loading()  ) {
		stats.filter_messages( tab_categories[tabs.get_active_tab_index()] );

		reset_min_windowsize();
		set_windowsize(size);
	}
}
