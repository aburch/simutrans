/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include "../simdebug.h"
#include "../simwerkz.h"
#include "../simworld.h"

#include "../utils/simstring.h"

#include "components/list_button.h"
#include "send_message_frame.h"

#define DIALOG_WIDTH (360)


send_message_frame_t::send_message_frame_t( spieler_t *sp ) :
	gui_frame_t("add message",sp)
{
	this->sp = sp;

	ibuf[0] = 0;
	input.set_text(ibuf, lengthof(ibuf) );
	input.add_listener(this);
	input.set_pos(koord(4,8));
	input.set_groesse(koord(DIALOG_WIDTH-4-10-10, 14));
	add_komponente(&input);

	set_focus( &input );

	set_fenstergroesse(koord(DIALOG_WIDTH, 40));
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool send_message_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp == &input  &&  ibuf[0]>0  ) {
		// add message via tool!
		werkzeug_t *w = create_tool( WKZ_ADD_MESSAGE_TOOL | SIMPLE_TOOL );
		w->set_default_param( ibuf );
		sp->get_welt()->set_werkzeug( w, sp );
		// since init always returns false, it is save to delete immediately
		delete w;
		ibuf[0] = 0;
	}
	return true;
}
