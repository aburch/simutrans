/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "label_info.h"
#include "../simworld.h"
#include "../simcolor.h"
#include "../simwin.h"
#include "../simmenu.h"
#include "../besch/skin_besch.h"
#include "../dataobj/translator.h"
#include "../dings/label.h"
#include "../player/simplay.h"
#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"



karte_t *label_info_t::welt = NULL;



label_info_t::label_info_t(karte_t *welt, label_t* l) :
	gui_frame_t( translator::translate("Marker"), l->get_besitzer()),
	player_name(""),
	view(welt, l->get_pos(), koord( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width()*7)/8) ))
{

	this->welt = welt;
	this->sp = sp;
	label = l;

	const char *const p_name = label->get_besitzer()->get_name();
	const int min_width = max(290, display_calc_proportional_string_len_width(p_name, strlen(p_name)) + view.get_groesse().x + 30);

	view.set_pos( koord(min_width - view.get_groesse().x - 10 , 21) );
	add_komponente( &view );

	grund_t *gr = welt->lookup(l->get_pos());
	if(gr->get_text()) {
		tstrncpy(edit_name, gr->get_text(), lengthof(edit_name));
	}
	else {
		edit_name[0] = '\0';
	}
	// text input
	input.set_pos(koord(10,4));
	input.set_groesse(koord(min_width-20, 13));
	input.set_text(edit_name, lengthof(edit_name));
	add_komponente(&input);
	input.add_listener(this);

	// text (player name)
	player_name.set_pos(koord(10, 21));
	player_name.set_text(p_name);
	add_komponente(&player_name);

	set_focus(&input);
	set_fenstergroesse(koord(min_width, view.get_groesse().y+47));
}



/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool label_info_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp == &input  &&  welt->get_active_player()==label->get_besitzer()) {
		// check owner to change text
		grund_t *gd = welt->lookup(label->get_pos());
		if(  strcmp(gd->get_text(),edit_name)  ) {
			// text changed => call tool
			cbuffer_t buf;
			buf.printf( "m%s,%s", label->get_pos().get_str(), edit_name );
			werkzeug_t *w = create_tool( WKZ_RENAME_TOOL | SIMPLE_TOOL );
			w->set_default_param( buf );
			welt->set_werkzeug( w, label->get_besitzer() );
			// since init always returns false, it is safe to delete immediately
			delete w;
		}
		destroy_win(this);
	}

	return true;
}



void label_info_t::map_rotate90( sint16 new_ysize )
{
	view.map_rotate90(new_ysize);
}
