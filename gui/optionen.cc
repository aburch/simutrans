/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Dialog fuer Spieloptionen
 * Niels Roest, Hj. Malthaner, 2000
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simimg.h"
#include "../simwin.h"
#include "../simgraph.h"
#include "../simsys.h"
#include "optionen.h"
#include "display_settings.h"
#include "sprachen.h"
#include "player_frame_t.h"
#include "welt.h"
#include "kennfarbe.h"
#include "sound_frame.h"
#include "loadsave_frame.h"
#include "scenario_frame.h"
#include "scenario_info.h"
#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"

#ifdef _MSC_VER
#include <direct.h>
#endif


const char *option_buttons_text[6] =
{
	"Sprache", "Farbe", "Helligk.", "Sound", "Spieler(mz)", "Scenario"
};

// currently not used yet
const char *option_buttons_tooltip[6] =
{
	"Sprache", "Farbe", "Helligk. u. Farben", "Sound settings", "Spielerliste", "Scenario information"
};




optionen_gui_t::optionen_gui_t(karte_t *welt) :
	gui_frame_t( translator::translate("Einstellungen aendern"))
{
	this->welt = welt;

	// init buttons
	KOORD_VAL row_x = D_MARGIN_LEFT;
	KOORD_VAL y = D_MARGIN_TOP;
	for(  int i=0;  i<6;  i++  ) {
		option_buttons[i].init( button_t::roundbox, option_buttons_text[i], koord( row_x, y ), koord( D_BUTTON_WIDTH, D_BUTTON_HEIGHT ) );
		option_buttons[i].add_listener(this);
		add_komponente( option_buttons+i );
		y += D_BUTTON_HEIGHT+D_V_SPACE;
	}
	// only activen when scenarios are active
	if(  !welt->get_scenario()->active()  ) {
		option_buttons[5].disable();
	}

	const KOORD_VAL y_max = y-D_V_SPACE+D_MARGIN_BOTTOM;

	// secon row of buttons
	y = D_MARGIN_TOP;
	row_x += D_BUTTON_WIDTH+D_H_SPACE;

	bt_load.init( button_t::roundbox, "Load game", koord( row_x, y ), koord( D_BUTTON_WIDTH, D_BUTTON_HEIGHT ) );
	bt_load.add_listener(this);
	add_komponente( &bt_load );
	y += D_BUTTON_HEIGHT+D_V_SPACE;

	bt_load_scenario.init( button_t::roundbox, "Load scenario", koord( row_x, y ), koord( D_BUTTON_WIDTH, D_BUTTON_HEIGHT ) );
	bt_load_scenario.add_listener(this);
	add_komponente( &bt_load_scenario );
	y += D_BUTTON_HEIGHT+D_V_SPACE;

	bt_save.init( button_t::roundbox, "Speichern", koord( row_x, y ), koord( D_BUTTON_WIDTH, D_BUTTON_HEIGHT ) );
	bt_save.add_listener(this);
	add_komponente( &bt_save );
	y += D_BUTTON_HEIGHT+D_V_SPACE;

	seperator.init( koord( row_x, y+D_BUTTON_HEIGHT/2 ), D_BUTTON_WIDTH );
	add_komponente( &seperator );
	y += D_BUTTON_HEIGHT+D_V_SPACE;

	bt_new.init( button_t::roundbox, "Neue Karte", koord( row_x, y ), koord( D_BUTTON_WIDTH, D_BUTTON_HEIGHT ) );
	bt_new.add_listener(this);
	add_komponente( &bt_new );
	y += D_BUTTON_HEIGHT+D_V_SPACE;

	bt_quit.init( button_t::roundbox, "Beenden", koord( row_x, y ), koord( D_BUTTON_WIDTH, D_BUTTON_HEIGHT ) );
	bt_quit.add_listener(this);
	add_komponente( &bt_quit );

	set_fenstergroesse( koord( D_MARGIN_LEFT+2*D_BUTTON_WIDTH+D_H_SPACE+D_MARGIN_RIGHT, y_max+D_TITLEBAR_HEIGHT ) );
}



/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool optionen_gui_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(comp==option_buttons+0) {
		create_win(new sprachengui_t(), w_info, magic_sprachengui_t);
	}
	else if(comp==option_buttons+1) {
		create_win(new farbengui_t(welt->get_active_player()), w_info, magic_farbengui_t);
	}
	else if(comp==option_buttons+2) {
		create_win(new color_gui_t(welt), w_info, magic_color_gui_t);
	}
	else if(comp==option_buttons+3) {
		create_win(new sound_frame_t(), w_info, magic_sound_kontroll_t);
	}
	else if(comp==option_buttons+4) {
		create_win(new ki_kontroll_t(welt), w_info, magic_ki_kontroll_t);
	}
	else if(comp==option_buttons+5) {
		create_win(new scenario_info_t(welt), w_info, magic_scenario_info);
	}
	else if(comp==&bt_load) {
		destroy_win(this);
		create_win(new loadsave_frame_t(welt, true), w_info, magic_load_t);
	}
	else if(comp==&bt_load_scenario) {
		destroy_win(this);
		destroy_all_win(true);
		char path[1024];
		sprintf( path, "%s%sscenario/", umgebung_t::program_dir, umgebung_t::objfilename.c_str() );
		chdir( path );
		create_win( new scenario_frame_t(welt), w_info, magic_load_t );
		chdir( umgebung_t::user_dir );
	}
	else if(comp==&bt_save) {
		destroy_win(this);
		create_win(new loadsave_frame_t(welt, false), w_info, magic_save_t);
	}
	else if(comp==&bt_new) {
		destroy_all_win( true );
		welt->beenden(false);
	}
	else if(comp==&bt_quit) {
		welt->beenden(true);
	}
	else {
		// not our?
		return false;
	}
	return true;
}
