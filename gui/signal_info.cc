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
#include "../obj/gebaeude.h"

#include "../simmenu.h"
#include "../simworld.h"
#include "../display/viewport.h"

signal_info_t::signal_info_t(signal_t* const s) :
	obj_infowin_t(s),
	sig(s)

{
	koord3d sb = sig->get_signalbox();
	if (sb == koord3d::invalid)
	{
		// No signalbox
	}
	else
	{
		const grund_t* gr = welt->lookup(sb);
		if (gr)
		{
			const gebaeude_t* gb = gr->get_building();
			if (gb)
			{
				signalbox_button.init(button_t::posbutton, NULL, scr_coord(D_MARGIN_LEFT, get_windowsize().h - 26 - (sig->get_textlines() * LINESPACE)));
				signalbox_button.set_tooltip(translator::translate("goto_signalbox"));
				add_component(&signalbox_button);
				signalbox_button.add_listener(this);

			}
			else
			{
				// No signalbox
			}
		}
		else
		{
			// No signalbox
		}
	}
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 *
 * Returns true, if action is done and no more
 * components should be triggered.
 * V.Meyer
   */
bool signal_info_t::action_triggered( gui_action_creator_t *comp, value_t)
{
	if (comp == &signalbox_button)
	{
		koord3d sb = sig->get_signalbox();
		welt->get_viewport()->change_world_position(koord3d(sb));
	}
	return true;
}