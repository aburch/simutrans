/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * Line management
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simcolor.h"

#include "../simworld.h"
#include "../simevent.h"
#include "../simgraph.h"
#include "../simplay.h"
#include "../simskin.h"
#include "../simwerkz.h"
#include "../simwin.h"

#include "../dataobj/translator.h"

#include "components/list_button.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "extend_edit.h"




extend_edit_gui_t::extend_edit_gui_t(spieler_t* sp_,karte_t* welt) :
	gui_frame_t("extend edit tool", sp_),
	sp(sp_),
	buf(2048),
	info_text(buf),
	scrolly(&cont),
	scl(gui_scrolled_list_t::select)
{
	this->welt = welt;

	is_show_trans_name = true;

	bt_climates.init( button_t::square_state, "ignore climates", koord(NAME_COLUMN_WIDTH+11, 10 ) );
	bt_climates.add_listener(this);
	add_komponente(&bt_climates);

	bt_timeline.init( button_t::square_state, "Use timeline start year", koord(NAME_COLUMN_WIDTH+11, 10+BUTTON_HEIGHT ) );
	bt_timeline.pressed = welt->gib_einstellungen()->gib_use_timeline();
	bt_timeline.add_listener(this);
	add_komponente(&bt_timeline);

	bt_obsolete.init( button_t::square_state, "Show obsolete", koord(NAME_COLUMN_WIDTH+11, 10+2*BUTTON_HEIGHT ) );
	bt_obsolete.add_listener(this);
	add_komponente(&bt_obsolete);

	offset_of_comp = 10+3*BUTTON_HEIGHT+4;

	// init scrolled list
	scl.setze_groesse(koord(NAME_COLUMN_WIDTH, SCL_HEIGHT-14));
	scl.setze_pos(koord(0,1));
	scl.setze_highlight_color(sp->get_player_color1()+1);
	scl.request_groesse(scl.gib_groesse());
	scl.setze_selection(-1);
	scl.add_listener(this);

	// tab panel
	tabs.setze_pos(koord(10,10));
	tabs.setze_groesse(koord(NAME_COLUMN_WIDTH, SCL_HEIGHT));
	tabs.add_tab(&scl, translator::translate("Translation"));//land
	tabs.add_tab(&scl, translator::translate("Object"));//city
	tabs.add_listener(this);
	add_komponente(&tabs);

	// item list
	info_text.setze_text(buf);
	info_text.setze_pos(koord(0,0));
	cont.add_komponente(&info_text);

	scrolly.set_visible(true);
	add_komponente(&scrolly);

	// image placeholder
	for(  sint16 i=3;  i>=0;  i--  ) {
		img[i].set_image(IMG_LEER);
		add_komponente( &img[i] );
	}

	// resize button
	set_min_windowsize(koord((short int)(BUTTON_WIDTH*4.5), 300));
	set_resizemode(diagonal_resize);
	resize(koord(0,0));
}



/**
 * Mausklicks werden hiermit an die GUI-Komponenten
 * gemeldet
 */
void extend_edit_gui_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == INFOWIN) {
		if(ev->ev_code == WIN_CLOSE) {
			change_item_info(-1);
			welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
		}
	}
	gui_frame_t::infowin_event(ev);
}



bool extend_edit_gui_t::action_triggered(gui_komponente_t *komp,value_t /* */)           // 28-Dec-01    Markus Weber    Added
{
	if (komp == &tabs) {
		// switch list translation or object name
		if(tabs.get_active_tab_index()==0 && is_show_trans_name==false) {
			// show translation list
			is_show_trans_name = true;
			fill_list( is_show_trans_name );
		}
		else if(tabs.get_active_tab_index()==1  &&   is_show_trans_name==true) {
			// show object list
			is_show_trans_name = false;
			fill_list( is_show_trans_name );
		}
	}
	else if (komp == &scl) {
		// select an item of scroll list ?
		change_item_info(scl.gib_selection());
	}
	else if(  komp==&bt_obsolete  ) {
		bt_obsolete.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(  komp==&bt_climates  ) {
		bt_climates.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(  komp==&bt_timeline  ) {
		bt_timeline.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	return true;
}



/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void extend_edit_gui_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);

	// test region
	koord groesse = gib_fenstergroesse()-koord(NAME_COLUMN_WIDTH+16,offset_of_comp+16);
	scrolly.setze_groesse(groesse);
	scrolly.setze_pos( koord( NAME_COLUMN_WIDTH+16, offset_of_comp ) );
	cont.setze_pos( koord( 0, 0 ) );

	// image placeholders
	sint16 rw = get_tile_raster_width()/(4*get_zoom_factor());
	static const koord img_offsets[4]={ koord(2,-6), koord(0,-5), koord(4,-5), koord(2,-4) };
	for(  sint16 i=0;  i<4;  i++  ) {
		koord pos = koord(4,gib_fenstergroesse().y-4-16) + img_offsets[i]*rw;
		img[i].setze_pos( pos );
	}

}
