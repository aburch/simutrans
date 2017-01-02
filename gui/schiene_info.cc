/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
* Track infowindow buttons //Ves
 */

#include "schiene_info.h"
#include "../boden/wege/schiene.h" // The rest of the dialog
#include "../vehicle/simvehicle.h"

#include "../simmenu.h"
#include "../simworld.h"
#include "../display/viewport.h"

schiene_info_t::schiene_info_t(schiene_t* const s) :
	obj_infowin_t(s),
	sch(s)

{
if(reserved.is_bound()) 
	{

		reserving_vehicle_button.init(button_t::posbutton, NULL, scr_coord(D_MARGIN_LEFT, get_windowsize().h - 25 - LINESPACE));
		reserving_vehicle_button.set_tooltip("goto_vehicle");
		add_component(&reserving_vehicle_button);
		reserving_vehicle_button.add_listener(this);

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
bool schiene_info_t::action_triggered( gui_action_creator_t *comp, value_t)
{
	if (comp == &reserving_vehicle_button)
	{
		if (welt->get_viewport()->get_follow_convoi() == cnv) {
			// stop following
			welt->get_viewport()->set_follow_convoi(convoihandle_t());
		}
		else {
			welt->get_viewport()->set_follow_convoi(cnv);
		}
		return true;
	}
	else
	return true;

}