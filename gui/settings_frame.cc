/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simwin.h"

#include "../dataobj/translator.h"
#include "settings_frame.h"

#include "components/list_button.h"
#include "components/action_listener.h"


settings_frame_t::settings_frame_t(einstellungen_t *s) : gui_frame_t("Settings"),
	sets(s),
	scrolly_general(&general),
	scrolly_economy(&economy),
	scrolly_routing(&routing)
{
	general.init( sets );
	economy.init( sets );
	routing.init( sets );

	// tab panel
	tabs.set_pos(koord(0,0));
	tabs.set_groesse(koord(320, 240)-koord(11,5));
	tabs.add_tab(&scrolly_general, translator::translate("General"));
	tabs.add_tab(&scrolly_economy, translator::translate("Economy"));
	tabs.add_tab(&scrolly_routing, translator::translate("Routing"));
//	tabs.add_tab(&scl, translator::translate("Economy"));
	add_komponente(&tabs);


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
void settings_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);
	koord groesse = get_fenstergroesse()-koord(0,16);
	tabs.set_groesse(groesse);
}




 /* triggered, when button clicked; only single button registered, so the action is clear ... */
bool settings_frame_t::action_triggered( gui_action_creator_t *,value_t)
{
	return true;
}



void settings_frame_t::infowin_event(const event_t *ev)
{
	if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE  ) {
		general.read( sets );
		routing.read( sets );
		economy.read( sets );
	}
	gui_frame_t::infowin_event(ev);
}
