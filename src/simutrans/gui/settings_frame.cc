/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include "../world/simcity.h"
#include "../sys/simsys.h"
#include "simwin.h"

#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/tabfile.h"
#include "settings_frame.h"

#include "components/gui_label.h"

#include "../world/simworld.h"


#include "components/action_listener.h"

using std::string;

settings_frame_t::settings_frame_t(settings_t* const s) :
	gui_frame_t( translator::translate("Setting") ),
	sets(s),
	scrolly_general(&general, true, true),
	scrolly_display(&display, true, true),
	scrolly_economy(&economy, true, true),
	scrolly_routing(&routing, true, true),
	scrolly_costs(&costs, true, true),
	scrolly_climates(&climates, true, true)
{
	set_table_layout(1,0);
	add_table(0,1);
	{
		new_component<gui_label_t>("Load settings from");

		revert_to_simuconf.init( button_t::roundbox | button_t::flexible, "Simuconf.tab");
		revert_to_simuconf.add_listener( this );
		add_component( &revert_to_simuconf );

		revert_to_default_sve.init( button_t::roundbox | button_t::flexible, "Default.sve");
		revert_to_default_sve.add_listener( this );
		add_component( &revert_to_default_sve);

		if (world()->type_of_generation != karte_t::AUTO_GENERATED  &&  sets != &welt->get_settings()) {
			revert_to_last_save.init(button_t::roundbox | button_t::flexible, "Current game");
			revert_to_last_save.add_listener(this);
			add_component(&revert_to_last_save);
		}
	}
	end_table();

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

	tabs.add_tab(&scrolly_general, translator::translate("General"));
	tabs.add_tab(&scrolly_display, translator::translate("Display settings"));
	tabs.add_tab(&scrolly_economy, translator::translate("Economy"));
	tabs.add_tab(&scrolly_routing, translator::translate("Routing"));
	tabs.add_tab(&scrolly_costs, translator::translate("Costs"));
	tabs.add_tab(&scrolly_climates, translator::translate("Climate Control"));
	add_component(&tabs);

	reset_min_windowsize();
	set_windowsize(get_min_windowsize() + general.get_min_size());
	set_resizemode(diagonal_resize);
}


 /* triggered, when button clicked; only single button registered, so the action is clear ... */
bool settings_frame_t::action_triggered( gui_action_creator_t *comp, value_t )
{
	// some things must stay the same when loading defaults
	const sint32 old_size_x     = sets->size_x;
	const sint32 old_size_y     = sets->size_y;
	const sint32 old_number     = sets->map_number;
	const uint8  old_rot        = sets->rotation;
	const uint32 old_city_count = sets->city_count;

	std::string old_save = sets->get_filename();

	uint8 old_player_type[MAX_PLAYER_COUNT];
	memcpy(old_player_type, sets->player_type, sizeof(old_player_type));

	// now we can change values
	if(  comp==&revert_to_simuconf  ) {
		// reread from simucon.tab(s) the settings and apply them
		tabfile_t simuconf;
		env_t::init();
		*sets = settings_t();
		dr_chdir( env_t::base_dir );
		if(simuconf.open("config/simuconf.tab")) {
			sets->parse_simuconf( simuconf );
			sets->parse_colours( simuconf );
		}
		stadt_t::cityrules_init();
		dr_chdir( env_t::pak_dir.c_str() );
		if(simuconf.open("config/simuconf.tab")) {
			sets->parse_simuconf( simuconf );
			sets->parse_colours( simuconf );
		}
		dr_chdir(  env_t::user_dir  );
		if(simuconf.open("simuconf.tab")) {
			sets->parse_simuconf( simuconf );
			sets->parse_colours( simuconf );
		}
		simuconf.close();

		// and update ...
		general.init( sets );
		display.init( sets );
		economy.init( sets );
		routing.init( sets );
		costs.init( sets );
		climates.init( sets );
		set_windowsize(get_windowsize());
	}
	else if(  comp==&revert_to_default_sve) {
		// load settings of last generated map
		dr_chdir( env_t::user_dir  );

		loadsave_t file;
		if(  file.rd_open("default.sve") == loadsave_t::FILE_STATUS_OK  ) {
			sets->rdwr(&file);
			sets->reset_after_global_settings_reload();
			file.close();
		}

		// and update ...
		general.init( sets );
		display.init( sets );
		economy.init( sets );
		routing.init( sets );
		costs.init( sets );
		climates.init( sets );
		set_windowsize(get_windowsize());
	}
	else if (comp == &revert_to_last_save) {
		// setting of the current map
		*sets = env_t::default_settings;
		// and update ...
		general.init(sets);
		display.init(sets);
		economy.init(sets);
		routing.init(sets);
		costs.init(sets);
		climates.init(sets);
		set_windowsize(get_windowsize());
	}

	// restore essential values
	sets->set_size(old_size_x, old_size_y);
	sets->map_number = old_number;
	sets->rotation   = old_rot;
	sets->city_count = old_city_count;

	sets->set_filename(old_save.c_str());

	memcpy(sets->player_type, old_player_type, sizeof(old_player_type));
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

		// only the rgb colours have been changed, the colours in system format must be updated
		env_t_rgb_to_system_colors();
	}
	return gui_frame_t::infowin_event(ev);
}
