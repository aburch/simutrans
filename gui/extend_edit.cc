/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * Base class for the map editing windows (in files *_edit.cc)
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simevent.h"

#include "../dataobj/translator.h"

#include "../player/simplay.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "extend_edit.h"




extend_edit_gui_t::extend_edit_gui_t(const char *name, spieler_t* sp_) :
	gui_frame_t( name, sp_ ),
	sp(sp_),
	info_text(&buf, COLUMN_WIDTH),
	scrolly(&cont),
	scl(gui_scrolled_list_t::listskin)
{

	is_show_trans_name = true;

	const sint16 image_width = get_base_tile_raster_width()*2;
	tab_panel_width = ( image_width>COLUMN_WIDTH ? image_width : COLUMN_WIDTH );

	// init scrolled list
	scl.set_size(scr_size(tab_panel_width, SCL_HEIGHT-14));
	scl.set_pos(scr_coord(0,1));
	scl.set_highlight_color(sp->get_player_color1()+1);
	scl.set_selection(-1);
	scl.add_listener(this);

	// tab panel
	tabs.set_pos(scr_coord(11,5));
	tabs.set_size(scr_size(tab_panel_width, SCL_HEIGHT));
	tabs.add_tab(&scl, translator::translate("Translation"));//land
	tabs.add_tab(&scl, translator::translate("Object"));//city
	tabs.add_listener(this);
	add_komponente(&tabs);

	bt_climates.init( button_t::square_state, "ignore climates", scr_coord(tab_panel_width+2*MARGIN, MARGIN) );
	bt_climates.add_listener(this);
	add_komponente(&bt_climates);

	bt_timeline.init( button_t::square_state, "Use timeline start year", scr_coord(tab_panel_width+2*MARGIN, D_BUTTON_HEIGHT+MARGIN) );
	bt_timeline.pressed = welt->get_settings().get_use_timeline();
	bt_timeline.add_listener(this);
	add_komponente(&bt_timeline);

	bt_obsolete.init( button_t::square_state, "Show obsolete", scr_coord(tab_panel_width+2*MARGIN, 2*D_BUTTON_HEIGHT+MARGIN) );
	bt_obsolete.add_listener(this);
	add_komponente(&bt_obsolete);

	offset_of_comp = MARGIN+3*D_BUTTON_HEIGHT+4;

	// item list
	info_text.set_pos(scr_coord(0, 10));
	cont.add_komponente(&info_text);
	cont.set_pos( scr_coord( 0, 0 ) );

	scrolly.set_visible(true);
	add_komponente(&scrolly);

	// image placeholder
	for(  sint16 i=3;  i>=0;  i--  ) {
		img[i].set_image(IMG_LEER);
		add_komponente( &img[i] );
	}

	// resize button
	set_min_windowsize(scr_size(tab_panel_width+COLUMN_WIDTH+3*MARGIN, D_TITLEBAR_HEIGHT+SCL_HEIGHT+(get_base_tile_raster_width()*3)/2+5*MARGIN));
	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));
}



/**
 * Mouse click are hereby reported to its GUI-Components
 */
bool extend_edit_gui_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE) {
		change_item_info(-1);
	}
	return gui_frame_t::infowin_event(ev);
}



bool extend_edit_gui_t::action_triggered( gui_action_creator_t *komp,value_t /* */)           // 28-Dec-01    Markus Weber    Added
{
	if (komp == &tabs) {
		// switch list translation or object name
		if (tabs.get_active_tab_index() == 0 && !is_show_trans_name) {
			// show translation list
			is_show_trans_name = true;
			fill_list( is_show_trans_name );
		} else if (tabs.get_active_tab_index() == 1 && is_show_trans_name) {
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
void extend_edit_gui_t::resize(const scr_coord delta)
{
	gui_frame_t::resize(delta);

	// text region
	scr_size size = get_windowsize()-scr_coord( tab_panel_width+2*MARGIN, offset_of_comp+16 );
	info_text.set_width(size.w - 20);
	info_text.recalc_size();
	cont.set_size( info_text.get_size() + scr_coord(0, 20) );
	scrolly.set_size(size);
	scrolly.set_pos( scr_coord( tab_panel_width+2*MARGIN, offset_of_comp ) );

	// image placeholders
	sint16 rw = get_base_tile_raster_width()/4;
	static const scr_coord img_offsets[4]={ scr_coord(0,0), scr_coord(-2,1), scr_coord(2,1), scr_coord(0,2) };
	for(  sint16 i=0;  i<4;  i++  ) {
		scr_coord pos = scr_coord(((tab_panel_width-get_base_tile_raster_width())/2)+MARGIN, SCL_HEIGHT+3*MARGIN) + img_offsets[i]*rw;
		img[i].set_pos( pos );
	}

}
