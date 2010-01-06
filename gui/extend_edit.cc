/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * Line management
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simcolor.h"

#include "../simworld.h"
#include "../simevent.h"
#include "../simgraph.h"
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

	const sint16 image_width = get_base_tile_raster_width()*2;
	tab_panel_width = ( image_width>COLUMN_WIDTH ? image_width : COLUMN_WIDTH );

	bt_climates.init( button_t::square_state, "ignore climates", koord(tab_panel_width+2*MARGIN, MARGIN) );
	bt_climates.add_listener(this);
	add_komponente(&bt_climates);

	bt_timeline.init( button_t::square_state, "Use timeline start year", koord(tab_panel_width+2*MARGIN, BUTTON_HEIGHT+MARGIN) );
	bt_timeline.pressed = welt->get_einstellungen()->get_use_timeline();
	bt_timeline.add_listener(this);
	add_komponente(&bt_timeline);

	bt_obsolete.init( button_t::square_state, "Show obsolete", koord(tab_panel_width+2*MARGIN, 2*BUTTON_HEIGHT+MARGIN) );
	bt_obsolete.add_listener(this);
	add_komponente(&bt_obsolete);

	offset_of_comp = MARGIN+3*BUTTON_HEIGHT+4;

	// init scrolled list
	scl.set_groesse(koord(tab_panel_width, SCL_HEIGHT-14));
	scl.set_pos(koord(0,1));
	scl.set_highlight_color(sp->get_player_color1()+1);
	scl.set_selection(-1);
	scl.add_listener(this);

	// tab panel
	tabs.set_pos(koord(MARGIN, MARGIN));
	tabs.set_groesse(koord(tab_panel_width, SCL_HEIGHT));
	tabs.add_tab(&scl, translator::translate("Translation"));//land
	tabs.add_tab(&scl, translator::translate("Object"));//city
	tabs.add_listener(this);
	add_komponente(&tabs);

	// item list
	info_text.set_text(buf);
	info_text.set_pos(koord(0,0));
	cont.add_komponente(&info_text);

	scrolly.set_visible(true);
	add_komponente(&scrolly);

	// image placeholder
	for(  sint16 i=3;  i>=0;  i--  ) {
		img[i].set_image(IMG_LEER);
		add_komponente( &img[i] );
	}

	// resize button
	set_min_windowsize(koord(tab_panel_width+COLUMN_WIDTH+3*MARGIN, SCL_HEIGHT+2*get_base_tile_raster_width()+4*MARGIN));
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
		}
	}
	gui_frame_t::infowin_event(ev);
}



bool extend_edit_gui_t::action_triggered( gui_action_creator_t *komp,value_t /* */)           // 28-Dec-01    Markus Weber    Added
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
		change_item_info(scl.get_selection());
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
	koord groesse = get_fenstergroesse()-koord( tab_panel_width+2*MARGIN, offset_of_comp+16 );
	scrolly.set_groesse(groesse);
	scrolly.set_pos( koord( tab_panel_width+2*MARGIN, offset_of_comp ) );
	cont.set_pos( koord( 0, 0 ) );

	// image placeholders
	sint16 rw = get_base_tile_raster_width()/4;
	static const koord img_offsets[4]={ koord(0,0), koord(-2,1), koord(2,1), koord(0,2) };
	for(  sint16 i=0;  i<4;  i++  ) {
		koord pos = koord(((tab_panel_width-get_base_tile_raster_width())/2)+MARGIN, SCL_HEIGHT+3*MARGIN) + img_offsets[i]*rw;
		img[i].set_pos( pos );
	}

}
