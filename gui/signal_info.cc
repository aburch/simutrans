/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
* signal infowindow buttons //Ves
 */

#include "signal_info.h"
#include "../obj/signal.h" // The rest of the dialog

#include "../simmenu.h"
#include "../simworld.h"
#include "../display/viewport.h"

signal_info_t::signal_info_t(roadsign_t* s) :
	obj_infowin_t(s),
	ampel(s)
{

	scr_coord dummy(D_MARGIN_LEFT, D_MARGIN_TOP);

	signalbox_button.init(button_t::roundbox, "Signalbox", dummy, D_BUTTON_SIZE);
	signalbox_button.set_tooltip("Automatically replace this convoy.");
	add_component(&signalbox_button);
	signalbox_button.add_listener(this);
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 *
 * Returns true, if action is done and no more
 * components should be triggered.
 * V.Meyer
   */
bool signal_info_t::action_triggered( gui_action_creator_t *comp, value_t v)
{
	signal_t* sig = (signal_t*)this;
	koord3d sb = sig->get_signalbox();
	koord signalbox_position = sb->get_world_position();
	if (sb != koord3d::invalid)
	{
		const grund_t* gr = welt->lookup(sb);
		if (gr)
		{
			const gebaeude_t* gb = gr->get_building();
			if (gb)
			{
				if (comp == &signalbox_button) {
					welt->get_viewport()->change_world_position(koord3d(welt->max_hgt(signalbox_position)));
				}
			}
		}
	}
}

	/*
// notify for an external update
void signal_info_t::update_data()
{
	ns.set_value( ampel->get_ticks_ns() );
	ow.set_value( ampel->get_ticks_ow() );
	offset.set_value( ampel->get_ticks_offset() );
}
*/