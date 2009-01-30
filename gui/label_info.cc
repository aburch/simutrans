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
#include "../simskin.h"
#include "../utils/simstring.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../besch/skin_besch.h"



karte_t *label_info_t::welt = NULL;



label_info_t::label_info_t(karte_t *welt, label_t* l)
	: gui_frame_t("Marker", l->get_besitzer()),
		view(welt, l->get_pos()),
		player_name("")
{

	this->welt = welt;
	this->sp = sp;
	label = l;

	view.set_pos( koord(290 - 64 - 16 , 21) );
	view.set_groesse( koord(64,56) );
	add_komponente( &view );

	grund_t *gr = welt->lookup(l->get_pos());
	if(gr->get_text()) {
		tstrncpy(edit_name, gr->get_text(), lengthof(edit_name));
	}
	else {
		edit_name[0] = '\0';
	}
	// text input
	input.set_pos(koord(11,4));
	input.set_groesse(koord(290-23, 13));
	input.set_text(edit_name, lengthof(edit_name));
	add_komponente(&input);
	input.add_listener(this);

	// text (player name)
	player_name.set_pos (koord(11, 21));
	player_name.set_text (label->get_besitzer()->get_name());
	add_komponente(&player_name);

	set_fenstergroesse(koord(290, 81+16));
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
		gd->set_text(edit_name);
		destroy_win(this);
	}

	return true;
}



void label_info_t::map_rotate90( sint16 new_ysize )
{
	koord3d l = view.get_location();
	l.rotate90( new_ysize );
	view.set_location( l );
}
