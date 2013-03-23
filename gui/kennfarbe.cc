/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simdebug.h"
#include "../simevent.h"
#include "../simimg.h"
#include "../simworld.h"
#include "../besch/skin_besch.h"
#include "../simskin.h"
#include "../dataobj/translator.h"
#include "kennfarbe.h"
#include "../player/simplay.h"



farbengui_t::farbengui_t(spieler_t *sp) :
	gui_frame_t( translator::translate("Farbe"), sp ),
	txt(&buf),
	c1( "Your primary color:" ),
	c2( "Your secondary color:" ),
	bild( skinverwaltung_t::color_options->get_bild_nr(0), sp->get_player_nr() )
{
	this->sp = sp;
	buf.clear();
	buf.append(translator::translate("COLOR_CHOOSE\n"));

	txt.set_pos( koord(D_MARGIN_LEFT,D_MARGIN_TOP) );
	txt.recalc_size();
	add_komponente( &txt );

	bild.set_pos( koord( (D_MARGIN_LEFT + 14*D_BUTTON_HEIGHT + 13*D_H_SPACE)-bild.get_groesse().x, D_MARGIN_TOP ) );
	add_komponente( &bild );

	KOORD_VAL y = D_MARGIN_TOP+D_V_SPACE + max( txt.get_groesse().y, bild.get_groesse().y )-LINESPACE;

	// player color 1
	c1.set_pos( koord(D_MARGIN_LEFT,y) );
	add_komponente( &c1 );
	y += LINESPACE+D_V_SPACE;
	// now get all colors (but the one of the current player)
	uint32 used_colors1 = 0;
	uint32 used_colors2 = 0;
	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		if(  i!=sp->get_player_nr()  &&  sp->get_welt()->get_spieler(i)  ) {
			used_colors1 |= 1 << (sp->get_welt()->get_spieler(i)->get_player_color1() / 8);
			used_colors2 |= 1 << (sp->get_welt()->get_spieler(i)->get_player_color2() / 8);
		}
	}
	// color buttons
	for(unsigned i=0;  i<28;  i++) {
		player_color_1[i].init( button_t::box_state, (used_colors1 & (1<<i) ? "X" : ""), koord( D_MARGIN_LEFT+(i%14)*(D_BUTTON_HEIGHT+D_H_SPACE), y+(i/14)*(D_BUTTON_HEIGHT+D_H_SPACE) ), koord(D_BUTTON_HEIGHT,D_BUTTON_HEIGHT) );
		player_color_1[i].background = i*8+4;
		player_color_1[i].add_listener(this);
		add_komponente( player_color_1+i );
	}
	player_color_1[sp->get_player_color1()/8].pressed = true;
	y += 2*D_BUTTON_HEIGHT+D_H_SPACE+D_V_SPACE+LINESPACE;

	// player color 2
	c2.set_pos( koord(D_MARGIN_LEFT,y) );
	add_komponente( &c2 );
	y += LINESPACE+D_V_SPACE;
	// second color buttons
	for(unsigned i=0;  i<28;  i++) {
		player_color_2[i].init( button_t::box_state, (used_colors2 & (1<<i) ? "X" : ""), koord( D_MARGIN_LEFT+(i%14)*(D_BUTTON_HEIGHT+D_H_SPACE), y+(i/14)*(D_BUTTON_HEIGHT+D_H_SPACE) ), koord(D_BUTTON_HEIGHT,D_BUTTON_HEIGHT) );
		player_color_2[i].background = i*8+4;
		player_color_2[i].add_listener(this);
		add_komponente( player_color_2+i );
	}
	player_color_2[sp->get_player_color2()/8].pressed = true;
	y += 2*D_BUTTON_HEIGHT+D_H_SPACE+D_MARGIN_BOTTOM;

	set_fenstergroesse( koord( D_MARGIN_LEFT + 14*D_BUTTON_HEIGHT + 13*D_H_SPACE + D_MARGIN_RIGHT, y+D_TITLEBAR_HEIGHT ) );
}



/**
 * This method is called if an action is triggered
 * @author V. Meyer
 */
bool farbengui_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	for(unsigned i=0;  i<28;  i++) {
		// new player 1 color ?
		if(komp==player_color_1+i) {
			for(unsigned j=0;  j<28;  j++) {
				player_color_1[j].pressed = false;
			}
			player_color_1[i].pressed = true;
			sp->set_player_color( i*8, sp->get_player_color2() );
			return true;
		}
		// new player color 2?
		if(komp==player_color_2+i) {
			for(unsigned j=0;  j<28;  j++) {
				player_color_2[j].pressed = false;
			}
			player_color_2[i].pressed = true;
			sp->set_player_color( sp->get_player_color1(), i*8 );
			return true;
		}
	}
	return false;
}
