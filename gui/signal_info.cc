/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
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
#include "../simsignalbox.h"


signal_info_t::signal_info_t(signal_t* const s) :
	obj_infowin_t(s),
	sig(s)
{
	bt_goto_signalbox.init(button_t::posbutton, NULL, scr_coord(D_MARGIN_LEFT, get_windowsize().h - 26 - (sig->get_textlines() * LINESPACE)));
	bt_goto_signalbox.set_visible(false);
	bt_goto_signalbox.set_tooltip(translator::translate("goto_signalbox"));
	add_component(&bt_goto_signalbox);
	bt_goto_signalbox.add_listener(this);

	bt_info_signalbox.init(button_t::roundbox, NULL, scr_coord(D_MARGIN_LEFT, bt_goto_signalbox.get_pos().y + LINESPACE), scr_size(bt_goto_signalbox.get_size()));
	bt_info_signalbox.set_visible(false);
	bt_info_signalbox.set_tooltip(translator::translate("open_signalbox_info"));
	add_component(&bt_info_signalbox);
	bt_info_signalbox.add_listener(this);

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
				bt_goto_signalbox.set_visible(true);
				bt_info_signalbox.set_visible(true);
			}
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
bool signal_info_t::action_triggered(gui_action_creator_t *comp, value_t)
{
	if (comp == &bt_goto_signalbox)
	{
		koord3d sb = sig->get_signalbox();
		welt->get_viewport()->change_world_position(koord3d(sb));
	}
	if (comp == &bt_info_signalbox)
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
				gebaeude_t* gb = gr->get_building();
				if (gb)
				{
					gr->get_building()->show_info();
					if (sig->get_player_nr() == welt->get_active_player()->get_player_nr())
					{
						welt->get_active_player()->set_selected_signalbox(static_cast<signalbox_t *>(gb));
					}
				}
			}
			return true;
		}
	}

	return false;
}
