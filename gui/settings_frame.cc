/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifdef _MSC_VER
#include <new.h> // for _set_new_handler
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "../simcity.h"
#include "../simwin.h"

#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/tabfile.h"
#include "settings_frame.h"

#include "components/list_button.h"
#include "components/action_listener.h"


settings_frame_t::settings_frame_t(einstellungen_t *s) : gui_frame_t("Setting"),
	sets(s),
	scrolly_general(&general),
	scrolly_economy(&economy),
	scrolly_routing(&routing),
	scrolly_exp_general(&exp_general),
	scrolly_exp_revenue(&exp_revenue),
	scrolly_costs(&costs)
{
	revert_to_default.init( button_t::roundbox, "Simuconf.tab", koord( 0, 0), koord( BUTTON_WIDTH, BUTTON_HEIGHT ) );
	revert_to_default.add_listener( this );
	add_komponente( &revert_to_default );
	revert_to_last_save.init( button_t::roundbox, "Default.sve", koord( BUTTON_WIDTH, 0), koord( BUTTON_WIDTH, BUTTON_HEIGHT ) );
	revert_to_last_save.add_listener( this );
	add_komponente( &revert_to_last_save );

	general.init( sets );
	economy.init( sets );
	routing.init( sets );
	exp_general.init( sets );
	exp_revenue.init( sets );
	costs.init( sets );

	// tab panel
	tabs.set_pos(koord(0,BUTTON_HEIGHT));
	tabs.set_groesse(koord(320, 240)-koord(11,5));
	tabs.add_tab(&scrolly_general, translator::translate("General"));
	tabs.add_tab(&scrolly_economy, translator::translate("Economy"));
	tabs.add_tab(&scrolly_routing, translator::translate("Routing"));
	tabs.add_tab(&tabs_experimental, translator::translate("Experimental"));
	tabs.add_tab(&scrolly_costs, translator::translate("Costs"));
	add_komponente(&tabs);

	tabs_experimental.add_tab(&scrolly_exp_general, translator::translate("General Exp."));
	tabs_experimental.add_tab(&scrolly_exp_revenue, translator::translate("Passengers"));
	//add_komponente(&tabs_experimental);

	set_fenstergroesse(koord(400, 480));
	// a min-size for the window
	set_min_windowsize(koord(400, 200));

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
	koord groesse = get_fenstergroesse()-koord(0,16+BUTTON_HEIGHT);
	tabs.set_groesse(groesse);
}




 /* triggered, when button clicked; only single button registered, so the action is clear ... */
bool settings_frame_t::action_triggered( gui_action_creator_t *komp, value_t )
{
	if(  komp==&revert_to_default  ) {
		// reread from simucon.tab(s) the settings and apply them
		tabfile_t simuconf;
		umgebung_t::init();
		*sets = einstellungen_t();
		chdir( umgebung_t::program_dir );
		if(simuconf.open("config/simuconf.tab")) {
			sint16 dummy16;
			cstring_t dummy_str;
			sets->parse_simuconf( simuconf, dummy16, dummy16, dummy16, dummy_str );
		}
		stadt_t::cityrules_init(umgebung_t::objfilename);
		chdir( umgebung_t::program_dir );
		chdir( umgebung_t::objfilename );
		if(simuconf.open("config/simuconf.tab")) {
			sint16 dummy16;
			cstring_t dummy_str;
			sets->parse_simuconf( simuconf, dummy16, dummy16, dummy16, dummy_str );
		}
		chdir(  umgebung_t::user_dir  );
		if(simuconf.open("simuconf.tab")) {
			sint16 dummy16;
			cstring_t dummy_str;
			sets->parse_simuconf( simuconf, dummy16, dummy16, dummy16, dummy_str );
		}
		simuconf.close();

		// and update ...
		general.init( sets );
		economy.init( sets );
		routing.init( sets );
		exp_general.init( sets );
		exp_revenue.init( sets );
		costs.init( sets );
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
		economy.init( sets );
		routing.init( sets );
		exp_general.init( sets );
		exp_revenue.init( sets );
		costs.init( sets );
	}
	return true;
}



void settings_frame_t::infowin_event(const event_t *ev)
{
	if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE  ) {
		general.read( sets );
		routing.read( sets );
		economy.read( sets );
		exp_general.read( sets );
		exp_revenue.read( sets );
		costs.read( sets );
	}
	gui_frame_t::infowin_event(ev);
}
