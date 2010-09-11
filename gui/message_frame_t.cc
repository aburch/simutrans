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
#include "message_stats_t.h"
#include "message_option_t.h"

#include "components/list_button.h"
#include "components/action_listener.h"


karte_t *message_frame_t::welt = NULL;


message_frame_t::message_frame_t(karte_t *welt) : gui_frame_t("Mailbox"),
	stats(welt),
	scrolly(&stats)
{
	this->welt = welt;

	scrolly.set_pos( koord(0,BUTTON_HEIGHT) );
	add_komponente(&scrolly);

	option_bt.init(button_t::roundbox, translator::translate("Optionen"), koord(BUTTON1_X,0), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	option_bt.add_listener(this);
	add_komponente(&option_bt);

	ibuf[0] = 0;
	input.set_text(ibuf, lengthof(ibuf) );
	input.add_listener(this);
	input.set_pos(koord(BUTTON1_X+BUTTON_WIDTH,0));
	add_komponente(&input);

	set_focus( &input );

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
	scrolly.set_groesse(groesse);
}


/* triggered, when button clicked; only single button registered, so the action is clear ... */
bool message_frame_t::action_triggered( gui_action_creator_t *komp, value_t )
{
	if(  komp==&option_bt  ) {
		create_win(320, 200, new message_option_t(welt), w_info, magic_none );
	}
	else if(komp == &input  &&  ibuf[0]>0  ) {
		// add message via tool!
		werkzeug_t *w = create_tool( WKZ_ADD_MESSAGE_TOOL | SIMPLE_TOOL );
		w->set_default_param( ibuf );
		welt->set_werkzeug( w, welt->get_active_player() );
		// since init always returns false, it is save to delete immediately
		delete w;
		ibuf[0] = 0;
		set_focus(&input);
	}
	return true;
}
