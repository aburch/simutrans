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
if(sch->is_reserved() || sch->is_reserved_directional() || sch->is_reserved_priority())
	{

		reserving_vehicle_button.init(button_t::posbutton, NULL, scr_coord(D_MARGIN_LEFT, get_windowsize().h - 26 - (sch->get_textlines() * LINESPACE)));
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
	welt->get_viewport()->set_follow_convoi(sch->get_reserved_convoi());
	}
	return true;
}