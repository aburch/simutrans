/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string>
#include "../simcity.h"
#include "../simsys.h"
#include "../simwin.h"

#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/tabfile.h"
#include "settings_frame.h"


#include "components/action_listener.h"

using std::string;

settings_frame_t::settings_frame_t(settings_t* const s) :
	gui_frame_t( translator::translate("Setting") ),
	sets(s),
	scrolly_general(&general),
	scrolly_display(&display),
	scrolly_economy(&economy),
	scrolly_routing(&routing),
	scrolly_costs(&costs),
	scrolly_climates(&climates)
{
	revert_to_default.init( button_t::roundbox, "Simuconf.tab", koord( BUTTON1_X, 0) );
	revert_to_default.add_listener( this );
	add_komponente( &revert_to_default );
	revert_to_last_save.init( button_t::roundbox, "Default.sve", koord( BUTTON2_X, 0) );
	revert_to_last_save.add_listener( this );
	add_komponente( &revert_to_last_save );

	general.init( sets );
	display.init( sets );
	economy.init( sets );
	routing.init( sets );
	costs.init( sets );
	climates.init( sets );

	scrolly_general.set_scroll_amount_y(D_BUTTON_HEIGHT/2);
	scrolly_economy.set_scroll_amount_y(D_BUTTON_HEIGHT/2);
	scrolly_routing.set_scroll_amount_y(D_BUTTON_HEIGHT/2);
	scrolly_costs.set_scroll_amount_y(D_BUTTON_HEIGHT/2);
	scrolly_climates.set_scroll_amount_y(D_BUTTON_HEIGHT/2);

	tabs.set_pos(koord(D_MARGIN_LEFT,D_BUTTON_HEIGHT));
	tabs.add_tab(&scrolly_general, translator::translate("General"));
	tabs.add_tab(&scrolly_display, translator::translate("Helligk."));
	tabs.add_tab(&scrolly_economy, translator::translate("Economy"));
	tabs.add_tab(&scrolly_routing, translator::translate("Routing"));
	tabs.add_tab(&scrolly_costs, translator::translate("Costs"));
	tabs.add_tab(&scrolly_climates, translator::translate("Climate Control"));
	add_komponente(&tabs);

	set_fenstergroesse(koord(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+D_BUTTON_HEIGHT+TAB_HEADER_V_SIZE+18*(D_BUTTON_HEIGHT/2)+2+1));
	set_min_windowsize(koord(BUTTON3_X, D_TITLEBAR_HEIGHT+D_BUTTON_HEIGHT+TAB_HEADER_V_SIZE+6*(D_BUTTON_HEIGHT/2)+2+1));

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
	koord groesse = get_fenstergroesse()-koord(D_MARGIN_LEFT,D_TITLEBAR_HEIGHT+D_BUTTON_HEIGHT+D_MARGIN_BOTTOM);
	tabs.set_groesse(groesse);
}



 /* triggered, when button clicked; only single button registered, so the action is clear ... */
bool settings_frame_t::action_triggered( gui_action_creator_t *komp, value_t )
{
	if(  komp==&revert_to_default  ) {
		// reread from simucon.tab(s) the settings and apply them
		tabfile_t simuconf;
		umgebung_t::init();
		*sets = settings_t();
		chdir( umgebung_t::program_dir );
		if(simuconf.open("config/simuconf.tab")) {
			sint16 dummy16;
			string dummy_str;
			sets->parse_simuconf( simuconf, dummy16, dummy16, dummy16, dummy_str );
		}
		stadt_t::cityrules_init(umgebung_t::objfilename);
		chdir( umgebung_t::program_dir );
		chdir( umgebung_t::objfilename.c_str() );
		if(simuconf.open("config/simuconf.tab")) {
			sint16 dummy16;
			string dummy_str;
			sets->parse_simuconf( simuconf, dummy16, dummy16, dummy16, dummy_str );
		}
		chdir(  umgebung_t::user_dir  );
		if(simuconf.open("simuconf.tab")) {
			sint16 dummy16;
			string dummy_str;
			sets->parse_simuconf( simuconf, dummy16, dummy16, dummy16, dummy_str );
		}
		simuconf.close();

		// and update ...
		general.init( sets );
		display.init( sets );
		economy.init( sets );
		routing.init( sets );
		costs.init( sets );
		climates.init( sets );
	}
	else if(  komp==&revert_to_last_save  ) {
		// load settings of last generated map
		loadsave_t file;
		chdir( umgebung_t::user_dir  );
		if(  file.rd_open("default.sve")  ) {
			sets->rdwr(&file);
			file.close();
		}

		// and update ...
		general.init( sets );
		display.init( sets );
		economy.init( sets );
		routing.init( sets );
		costs.init( sets );
		climates.init( sets );
	}
	return true;
}



bool settings_frame_t::infowin_event(const event_t *ev)
{
	if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE  ) {
		general.read( sets );
		display.read( sets );
		routing.read( sets );
		economy.read( sets );
		costs.read( sets );
		climates.read( sets );
	}
	return gui_frame_t::infowin_event(ev);
}
