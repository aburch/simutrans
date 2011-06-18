/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simwin.h"
#include "../simworld.h"
#include "../simwerkz.h"

#include "../dataobj/translator.h"
#include "message_frame_t.h"
#include "../simmesg.h"
#include "message_option_t.h"

#include "components/list_button.h"
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



message_frame_t::message_frame_t(karte_t *welt) : gui_frame_t("Mailbox"),
	stats(welt),
	scrolly(&stats)
{
	this->welt = welt;

	// Knightly : add tabs for classifying messages
	tabs.set_pos( koord(0, BUTTON_HEIGHT) );
	tabs.add_tab( &scrolly, translator::translate("All") );
	for(  int i=umgebung_t::networkmode ? 0 : 1;  i<MAX_MESG_TABS;  ++i  ) {
		tabs.add_tab( &scrolly, translator::translate(tab_strings[i]) );
	}
	tabs.add_listener(this);
	add_komponente(&tabs);

	option_bt.init(button_t::roundbox, translator::translate("Optionen"), koord(BUTTON1_X,0), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	option_bt.add_listener(this);
	add_komponente(&option_bt);

	ibuf[0] = 0;
	input.set_text(ibuf, lengthof(ibuf) );
	input.add_listener(this);
	input.set_pos(koord(BUTTON1_X+BUTTON_WIDTH,0));
	if(  umgebung_t::networkmode  ) {
		add_komponente(&input);
		set_focus( &input );
	}

	scrolly.set_show_scroll_x(true);
	scrolly.set_scroll_amount_y(LINESPACE+3);

	set_fenstergroesse(koord(320, 240));
	// a min-size for the window
	set_min_windowsize(koord(320, 80));

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
	koord groesse = get_fenstergroesse()-koord(0,16+BUTTON_HEIGHT);
	input.set_groesse(koord(groesse.x-10-BUTTON1_X-BUTTON_WIDTH, BUTTON_HEIGHT));
	tabs.set_groesse(groesse);
}


/* triggered, when button clicked; only single button registered, so the action is clear ... */
bool message_frame_t::action_triggered( gui_action_creator_t *komp, value_t v )
{
	if(  komp==&option_bt  ) {
		create_win(320, 200, new message_option_t(welt), w_info, magic_none );
	}
	else if(  komp==&input  &&  ibuf[0]!=0  ) {
		// add message via tool!
		werkzeug_t *w = create_tool( WKZ_ADD_MESSAGE_TOOL | SIMPLE_TOOL );
		w->set_default_param( ibuf );
		welt->set_werkzeug( w, welt->get_active_player() );
		// since init always returns false, it is save to delete immediately
		delete w;
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
