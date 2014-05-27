/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Dialog for game options
 * Niels Roest, Hj. Malthaner, 2000
 */

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_divider.h"
#include "components/action_listener.h"

#include "../simworld.h"
#include "../gui/simwin.h"
#include "optionen.h"
#include "display_settings.h"
#include "sprachen.h"
#include "player_frame_t.h"
#include "kennfarbe.h"
#include "sound_frame.h"
#include "loadsave_frame.h"
#include "scenario_frame.h"
#include "scenario_info.h"
#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"

enum BUTTONS {
	BUTTON_LANGUAGE = 0,
	BUTTON_PLAYERS,
	BUTTON_PLAYER_COLORS,
	BUTTON_DISPLAY,
	BUTTON_SOUND,
	BUTTON_NEW_GAME,
	BUTTON_LOAD_GAME,
	BUTTON_SAVE_GAME,
	BUTTON_LOAD_SCENARIO,
	BUTTON_SCENARIO_INFO,
	BUTTON_QUIT
};

static char const *const option_buttons_text[] =
{
	"Sprache", "Spieler(mz)", "Farbe", "Helligk.", "Sound",
	"Neue Karte", "Load game", "Speichern", "Load scenario", "Scenario", "Beenden"
};


optionen_gui_t::optionen_gui_t() :
	gui_frame_t( translator::translate("Einstellungen aendern"))
{
	assert(  lengthof(option_buttons)==lengthof(option_buttons_text)  );
	assert(  lengthof(option_buttons)==BUTTON_QUIT+1  );

	scr_coord cursor = scr_coord( D_MARGIN_LEFT, D_MARGIN_TOP );


	for(  uint i=0;  i<lengthof(option_buttons);  i++  ) {

		switch(i) {

			// Enable/disable scenario button
			case BUTTON_SCENARIO_INFO:
				if(  !welt->get_scenario()->active()  ) {
					option_buttons[BUTTON_SCENARIO_INFO].disable();
				}
				break;

			// Move cursor to the second column
			case BUTTON_NEW_GAME:
				cursor = scr_coord ( cursor.x+D_BUTTON_WIDTH+D_H_SPACE,D_MARGIN_TOP);
				break;

			// Squeeze in divider
			case BUTTON_QUIT:
				cursor.y -= D_V_SPACE;
				divider.init( scr_coord(D_MARGIN_LEFT, cursor.y), cursor.x - D_MARGIN_LEFT + D_BUTTON_WIDTH );
				add_komponente( &divider );
				cursor.y += divider.get_size().h; //+D_V_SPACE;
				break;

		}

		// Add button at cursor
		option_buttons[i].init( button_t::roundbox, option_buttons_text[i], cursor, scr_size( D_BUTTON_WIDTH, D_BUTTON_HEIGHT ) );
		option_buttons[i].add_listener(this);
		add_komponente( option_buttons+i );
		cursor.y += D_BUTTON_HEIGHT + D_V_SPACE;
	}

	set_windowsize( scr_size( D_BUTTON_WIDTH + D_MARGIN_RIGHT, D_TITLEBAR_HEIGHT + D_MARGIN_BOTTOM - D_V_SPACE ) + cursor );
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
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
	else if(  comp == option_buttons + BUTTON_LOAD_GAME  ) {
		destroy_win(this);
		create_win(new loadsave_frame_t(true), w_info, magic_load_t);
	}
	else if(  comp == option_buttons + BUTTON_LOAD_SCENARIO  ) {
		destroy_win(this);
		destroy_all_win(true);
		create_win( new scenario_frame_t(), w_info, magic_load_t );
	}
	else if(  comp == option_buttons + BUTTON_SAVE_GAME  ) {
		destroy_win(this);
		create_win(new loadsave_frame_t(false), w_info, magic_save_t);
	}
	else if(  comp == option_buttons + BUTTON_NEW_GAME  ) {
		destroy_all_win( true );
		welt->stop(false);
	}
	else if(  comp == option_buttons + BUTTON_QUIT  ) {
		welt->stop(true);
	}
	else {
		// not our?
		return false;
	}
	return true;
}
