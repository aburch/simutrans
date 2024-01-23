/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_divider.h"
#include "components/action_listener.h"

#include "../world/simworld.h"
#include "../tool/simmenu.h"
#include "simwin.h"
#include "optionen.h"
#include "display_settings.h"
#include "sprachen.h"
#include "player_frame.h"
#include "kennfarbe.h"
#include "sound_frame.h"
#include "scenario_info.h"
#include "banner.h"
#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../sys/simsys.h"

enum BUTTONS {
	BUTTON_LANGUAGE = 0,
	BUTTON_PLAYERS,
	BUTTON_DISPLAY,
	BUTTON_PLAYER_COLORS,
	BUTTON_SOUND,
	BUTTON_SAVE_GAME,
	BUTTON_SCENARIO_INFO,
	BUTTON_MENU,
	BUTTON_QUIT
};

static char const *const option_buttons_text[] =
{
	"Sprache",
	"Spieler(mz)",
	"Display settings",
	"Farbe",
	"Sound",
	"Save",
	"Scenario",
	"Return to menu",
	"Beenden"
};


optionen_gui_t::optionen_gui_t() :
	gui_frame_t( translator::translate("Einstellungen aendern"))
{
	assert(  lengthof(option_buttons)==lengthof(option_buttons_text)  );
	assert(  lengthof(option_buttons)==BUTTON_QUIT+1  );

	set_table_layout(2,0);
	set_force_equal_columns(true);

	for(  uint i=0;  i<lengthof(option_buttons);  i++  ) {

		option_buttons[i].init(button_t::roundbox | button_t::flexible, option_buttons_text[i]);
		option_buttons[i].add_listener(this);

		if (i == BUTTON_SCENARIO_INFO) {
			// Enable/disable scenario button
			if(  !welt->get_scenario()->active()  ) {
				option_buttons[BUTTON_SCENARIO_INFO].disable();
			}
			// Squeeze in divider
			new_component<gui_fill_t>();
			add_component(option_buttons + i);
			new_component_span<gui_divider_t>(2);
		} else add_component(option_buttons + i);
	}

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
	set_resizemode(gui_frame_t::horizontal_resize);
}


/**
 * This method is called if an action is triggered
 */
bool optionen_gui_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(  comp == option_buttons + BUTTON_LANGUAGE  ) {
		create_win(new sprachengui_t(), w_info, magic_sprachengui_t);
	}
	else if(  comp == option_buttons + BUTTON_PLAYER_COLORS  ) {
		create_win(new farbengui_t(welt->get_active_player()), w_info, magic_farbengui_t);
	}
	else if(  comp == option_buttons + BUTTON_DISPLAY  ) {
		create_win(new color_gui_t(), w_info, magic_color_gui_t);
	}
	else if(  comp == option_buttons + BUTTON_SOUND  ) {
		create_win(new sound_frame_t(), w_info, magic_sound_kontroll_t);
	}
	else if(  comp == option_buttons + BUTTON_PLAYERS  ) {
		create_win(new ki_kontroll_t(), w_info, magic_ki_kontroll_t);
	}
	else if(  comp == option_buttons + BUTTON_SCENARIO_INFO  ) {
		create_win(new scenario_info_t(), w_info, magic_scenario_info);
	}
	else if(  comp == option_buttons + BUTTON_SAVE_GAME  ) {
		destroy_win(this);
		tool_t *tmp_tool = create_tool( DIALOG_SAVE | DIALOGE_TOOL );
		welt->set_tool( tmp_tool, welt->get_active_player() );
		// since init always returns false, it is safe to delete immediately
		delete tmp_tool;
	}
	else if (comp == option_buttons + BUTTON_MENU) {
		// return to menu
		banner_t::show_banner();
	}
	else if(  comp == option_buttons + BUTTON_QUIT  ) {
		// we call the proper tool for quitting
		welt->set_tool(tool_t::simple_tool[TOOL_QUIT], NULL);
	}
	else {
		// not our?
		return false;
	}
	return true;
}
