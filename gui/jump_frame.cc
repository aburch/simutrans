/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 *
 * jumps to a position
 */

#include <string.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "jump_frame.h"
#include "components/gui_button.h"
#include "components/list_button.h"


jump_frame_t::jump_frame_t(karte_t *welt) :
	gui_frame_t("Jump to")
{
	this->welt = welt;

	// Input box for new name
	sprintf(buf, "%i,%i", welt->gib_ij_off().x, welt->gib_ij_off().y );
	input.setze_text(buf, 62);
	input.add_listener(this);
	input.setze_pos(koord(10,4));
	input.setze_groesse(koord(BUTTON_WIDTH, 14));
	add_komponente(&input);

	divider1.setze_pos(koord(10,24));
	divider1.setze_groesse(koord(BUTTON_WIDTH,0));
	add_komponente(&divider1);

	jumpbutton.init( button_t::roundbox, "Jump to", koord(10, 28), koord( BUTTON_WIDTH,BUTTON_HEIGHT ) );
	jumpbutton.add_listener(this);
	add_komponente(&jumpbutton);

	setze_opaque(true);
	setze_fenstergroesse(koord(BUTTON_WIDTH+20, 62));
}



/**
 * This method is called if an action is triggered
 * @author V. Meyer
 */
bool jump_frame_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
	if(komp == &input || komp == &jumpbutton) {
		// OK- Button or Enter-Key pressed
		//---------------------------------------
		koord my_pos;
		sscanf(buf, "%hd,%hd", &my_pos.x, &my_pos.y);
		if(welt->ist_in_kartengrenzen(my_pos)) {
			welt->setze_ij_off(koord3d(my_pos,welt->min_hgt(my_pos)));
		}
	}
	return true;
}



