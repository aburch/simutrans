/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * Dialog fuer Spieloptionen
 * Niels Roest, Hj. Malthaner, 2000
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simimg.h"
#include "../simwin.h"
#include "../simplay.h"
#include "../simgraph.h"
#include "../simdisplay.h"
#include "optionen.h"
#include "colors.h"
#include "sprachen.h"
#include "player_frame_t.h"
#include "welt.h"
#include "kennfarbe.h"
#include "sound_frame.h"
#include "loadsave_frame.h"

optionen_gui_t::optionen_gui_t(karte_t *welt) :
	gui_frame_t("Einstellungen"),
	txt("Einstellungen aendern")
{
	this->welt = welt;

	// init buttons
	bt_lang.setze_groesse( koord(90, 14) );
	bt_lang.setze_typ(button_t::roundbox);
	bt_lang.setze_pos( koord(11,30) );
	bt_lang.setze_text("Sprache");
	bt_lang.add_listener(this);
	add_komponente( &bt_lang );

	bt_color.setze_groesse( koord(90, 14) );
	bt_color.setze_typ(button_t::roundbox);
	bt_color.setze_pos( koord(11,47) );
	bt_color.setze_text("Farbe");
	bt_color.add_listener(this);
	add_komponente( &bt_color );

	bt_display.setze_groesse( koord(90, 14) );
	bt_display.setze_typ(button_t::roundbox);
	bt_display.setze_pos( koord(11,64) );
	bt_display.setze_text("Helligk.");
	bt_display.add_listener(this);
	add_komponente( &bt_display );

	bt_sound.setze_groesse( koord(90, 14) );
	bt_sound.setze_typ(button_t::roundbox);
	bt_sound.setze_pos( koord(11,81) );
	bt_sound.setze_text("Sound");
	bt_sound.add_listener(this);
	add_komponente( &bt_sound );

	bt_player.setze_groesse( koord(90, 14) );
	bt_player.setze_typ(button_t::roundbox);
	bt_player.setze_pos( koord(11,98) );
	bt_player.setze_text("Spieler(mz)");
	bt_player.add_listener(this);
	add_komponente( &bt_player );

	bt_load.setze_groesse( koord(90, 14) );
	bt_load.setze_typ(button_t::roundbox);
	bt_load.setze_pos( koord(112,30) );
	bt_load.setze_text("Laden");
	bt_load.add_listener(this);
	add_komponente( &bt_load );

	bt_save.setze_groesse( koord(90, 14) );
	bt_save.setze_typ(button_t::roundbox);
	bt_save.setze_pos( koord(112,47) );
	bt_save.setze_text("Speichern");
	bt_save.add_listener(this);
	add_komponente( &bt_save );

	bt_new.setze_groesse( koord(90, 14) );
	bt_new.setze_typ(button_t::roundbox);
	bt_new.setze_pos( koord(112,64) );
	bt_new.setze_text("Neue Karte");
	bt_new.add_listener(this);
	add_komponente( &bt_new );

	// 01-Nov-2001      Markus Weber    Added
	bt_quit.setze_groesse( koord(90, 14) );
	bt_quit.setze_typ(button_t::roundbox);
	bt_quit.setze_pos( koord(112,98) );
	bt_quit.setze_text("Beenden");
	bt_quit.add_listener(this);
	add_komponente( &bt_quit );

	txt.setze_pos( koord(10,10) );
	add_komponente( &txt );

	seperator.setze_pos( koord(112, 81+6) );
	seperator.setze_groesse( koord(90,1) );
	add_komponente( &seperator );

	setze_fenstergroesse( koord(213, 98+7+14+16) );
}



/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool optionen_gui_t::action_triggered(gui_komponente_t *comp,value_t /* */)
{
	if(comp==&bt_lang) {
		create_win(new sprachengui_t(), w_info, magic_sprachengui_t);
	}
	else if(comp==&bt_color) {
		create_win(new farbengui_t(welt->get_active_player()), w_info, magic_farbengui_t);
	}
	else if(comp==&bt_display) {
		create_win(new color_gui_t(welt), w_info, magic_color_gui_t);
	}
	else if(comp==&bt_sound) {
		create_win(new sound_frame_t(), w_info, magic_sound_kontroll_t);
	}
	else if(comp==&bt_player) {
		create_win(new ki_kontroll_t(welt), w_info, magic_ki_kontroll_t);
	}
	else if(comp==&bt_load) {
		destroy_win(this);
		create_win(new loadsave_frame_t(welt, true), w_info, magic_load_t);
	}
	else if(comp==&bt_save) {
		destroy_win(this);
		create_win(new loadsave_frame_t(welt, false), w_info, magic_save_t);
	}
	else if(comp==&bt_new) {
		destroy_all_win();
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
