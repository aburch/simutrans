/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simwin.h"
#include "../simworld.h"
#include "../simmenu.h"

#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "message_frame_t.h"
#include "../simmesg.h"
#include "message_option_t.h"
#include "../dataobj/network_cmd_ingame.h"
#include "../player/simplay.h"


#include "components/action_listener.h"


#define MAX_MESG_TABS (7)

karte_t *message_frame_t::welt = NULL;

static sint32 categories[MAX_MESG_TABS+1] =
{
	-1,
	(1 << message_t::chat),
	(1 << message_t::problems),
	(1 << message_t::traffic_jams) | (1 << message_t::warnings),
	(1 << message_t::full),
	(1 << message_t::city) | (1 << message_t::industry),
	(1 << message_t::ai),
	(1 << message_t::general) | (1 << message_t::new_vehicle)
};

static const char *tab_strings[]=
{
	"Chat_msg",
	"Problems_msg",
	"Warnings_msg",
	"Station_msg",
	"Town_msg",
	"Company_msg",
	"Game_msg"
};



message_frame_t::message_frame_t(karte_t *welt) :
	gui_frame_t( translator::translate("Mailbox") ),
	stats(welt),
	scrolly(&stats)
{
	this->welt = welt;

	scrolly.set_show_scroll_x(true);
	scrolly.set_scroll_amount_y(LINESPACE+1);

	// Knightly : add tabs for classifying messages
	tabs.set_pos( koord(0, D_BUTTON_HEIGHT) );
	tabs.add_tab( &scrolly, translator::translate("All") );
	for(  int i=umgebung_t::networkmode ? 0 : 1;  i<MAX_MESG_TABS;  ++i  ) {
		tabs.add_tab( &scrolly, translator::translate(tab_strings[i]) );
	}
	tabs.add_listener(this);
	add_komponente(&tabs);

	option_bt.init(button_t::roundbox, translator::translate("Optionen"), koord(BUTTON1_X,0), koord(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	option_bt.add_listener(this);
	add_komponente(&option_bt);

	ibuf[0] = 0;
	input.set_text(ibuf, lengthof(ibuf) );
	input.add_listener(this);
	input.set_pos(koord(BUTTON2_X,0));
	if(  umgebung_t::networkmode  ) {
		set_transparent( umgebung_t::chat_window_transparency, COL_WHITE );
		add_komponente(&input);
		set_focus( &input );
	}

	set_fenstergroesse(koord(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+D_BUTTON_HEIGHT+gui_tab_panel_t::HEADER_VSIZE+2+16*(LINESPACE+1)+scrollbar_t::BAR_SIZE));
	set_min_windowsize(koord(BUTTON3_X, D_TITLEBAR_HEIGHT+D_BUTTON_HEIGHT+gui_tab_panel_t::HEADER_VSIZE+2+3*(LINESPACE+1)+scrollbar_t::BAR_SIZE));

	set_resizemode(diagonal_resize);
	resize(koord(0,0));
}



/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void message_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);
	koord groesse = get_fenstergroesse()-koord(0,D_TITLEBAR_HEIGHT+D_BUTTON_HEIGHT);
	input.set_groesse(koord(groesse.x-scrollbar_t::BAR_SIZE-BUTTON2_X, D_BUTTON_HEIGHT));
	tabs.set_groesse(groesse);
	scrolly.set_groesse(groesse-koord(0,D_BUTTON_HEIGHT+4+1));
}


/* triggered, when button clicked; only single button registered, so the action is clear ... */
bool message_frame_t::action_triggered( gui_action_creator_t *komp, value_t v )
{
	if(  komp==&option_bt  ) {
		create_win(320, 200, new message_option_t(welt), w_info, magic_message_options );
	}
	else if(  komp==&input  &&  ibuf[0]!=0  ) {
		// Send chat message to server for distribution
		nwc_chat_t* nwchat = new nwc_chat_t( ibuf, welt->get_active_player()->get_player_nr(), umgebung_t::nickname.c_str() );
		network_send_server( nwchat );

		ibuf[0] = 0;
		set_focus(&input);
	}
	else if(  komp==&tabs  ) {
		// Knightly : filter messages by type where necessary
		if(  stats.filter_messages( categories[umgebung_t::networkmode ? v.i : (v.i==0 ? 0 : v.i+1)] )  ) {
			scrolly.set_scroll_position(0, 0);
		}
	}
	return true;
}


void message_frame_t::rdwr(loadsave_t *file)
{
	koord gr = get_fenstergroesse();
	sint32 scroll_x = scrolly.get_scroll_x();
	sint32 scroll_y = scrolly.get_scroll_y();
	sint16 tabstate = tabs.get_active_tab_index();

	gr.rdwr( file );
	file->rdwr_str( ibuf, lengthof(ibuf) );
	file->rdwr_short( tabstate );
	file->rdwr_long( scroll_x );
	file->rdwr_long( scroll_y );

	if(  file->is_loading()  ) {
		tabs.set_active_tab_index( tabstate );
		set_fenstergroesse( gr );
		resize( koord(0,0) );
		scrolly.set_scroll_position( scroll_x, scroll_y );
	}
}
