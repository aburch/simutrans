/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * jumps to a position
 */

#include <string.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simwerkz.h"
#include "station_building_select.h"
#include "components/gui_button.h"
#include "components/list_button.h"


static const char label_text[4][64] = {
	"sued",
	"ost",
	"nord",
	"west"
};


karte_t *station_building_select_t::welt = NULL;
char station_building_select_t::default_str[260];
wkz_station_t station_building_select_t::wkz=wkz_station_t();


station_building_select_t::station_building_select_t(karte_t *welt, const haus_besch_t *besch) :
	gui_frame_t("Choose direction"),
	txt("")
{
	this->welt = welt;
	this->besch = besch;

	int layout = besch->get_all_layouts();
	int row = (layout > 2) ? 1 : 0;
	sint16 rw = get_base_tile_raster_width()/4;
	int y_diff = (besch->get_b(0)==1) ? ((besch->get_h(0)==1) ? rw*4 : rw*5) : ((besch->get_h(0)==1) ? rw*5 : rw*6) ;
	const koord img_offsets[4]={ koord(rw*2,0), koord(0,rw), koord(rw*4,rw), koord(rw*2,rw*2) };
	const koord base_offsets[6]={ koord(0,0), koord(rw*8+10,0), koord(0,y_diff+30),  koord(rw*8+10,y_diff+30), koord(rw*8+10,0), koord(0,0) };

	// image placeholder
	for( sint16 i=0;  i<layout;  i++ ) {
		for( sint16 j=0;  j<4;  j++ ) {
			koord pos = img_offsets[j]+base_offsets[i+row*2];
			img[i*4+j].set_pos( pos );
			img[i*4+j].set_image(IMG_LEER);
			img[i*4+j].set_pos( pos );
			add_komponente( &img[i*4+j] );
		}
	}
	// now the images (maximum is 2x2 size)
	// since these may be affected by rotation, we do this every time ...
	for(int i=0;  i<layout;  i++  ) {
		uint8 rot = i;
		for(int j=0;  j<4;  j++  ) {
			img[i*4].set_image( besch->get_tile(rot,0,0)->get_hintergrund(0,0,0) );

			if(besch->get_h(rot)>1) {
				img[i*4+1].set_image( besch->get_tile(rot,0,1)->get_hintergrund(0,0,0) );
			}
			if(besch->get_b(rot)==1) {
				continue;
			}
			else {
				img[i*4+2].set_image( besch->get_tile(rot,1,0)->get_hintergrund(0,0,0) );
			}
			if(besch->get_h(rot)==1) {
				continue;
			}
			img[i*4+3].set_image( besch->get_tile(rot,1,1)->get_hintergrund(0,0,0) );
		}
	}

	// text
	sprintf(buf, "X=%i,Y=%i", besch->get_b(0), besch->get_h(0) );
	txt.set_text(buf);
	txt.set_pos( koord(10,0) );
	add_komponente( &txt );

	// button
	for(int i=0; i<layout; i++) {
		actionbutton[i].init( button_t::roundbox, translator::translate(label_text[i]), base_offsets[i+row*2]+koord(rw*4-BUTTON_WIDTH/2, y_diff), koord( BUTTON_WIDTH,BUTTON_HEIGHT ) );
		actionbutton[i].add_listener(this);
		add_komponente(&actionbutton[i]);
	}
	set_fenstergroesse(koord(rw*16+10, (y_diff+30)*(row+1)+10));
}



/**
 * This method is called if an action is triggered
 * @author V. Meyer
 */
bool station_building_select_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	for(int i=0; i<4; i++) {
		if(komp == &actionbutton[i]) {
			static char default_str[1024];
			sprintf( default_str, "%s,%i", besch->get_name(), i );
			wkz.default_param = default_str;
			welt->set_werkzeug( &wkz );
			destroy_win(this);
		}
	}
	return true;
}
