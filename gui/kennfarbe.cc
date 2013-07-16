/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Company colors window
 */
#include "kennfarbe.h"
#include "../simdebug.h"
#include "../simevent.h"
#include "../simimg.h"
#include "../simworld.h"
#include "../besch/skin_besch.h"
#include "../simskin.h"
#include "../dataobj/translator.h"
#include "../player/simplay.h"

farbengui_t::farbengui_t(spieler_t *sp) :
	gui_frame_t( translator::translate("Farbe"), sp ),
	txt(&buf),
	c1( "Your primary color:" ),
	c2( "Your secondary color:" ),
	bild( skinverwaltung_t::color_options->get_bild_nr(0), sp->get_player_nr() )
{
	koord cursor = koord (D_MARGIN_TOP, D_MARGIN_LEFT);

	this->sp = sp;
	buf.clear();
	buf.append(translator::translate("COLOR_CHOOSE\n"));

	// Info text
	txt.set_pos( cursor );
	txt.recalc_size();
	add_komponente( &txt );

	// Picture
	bild.set_pos(cursor);
	bild.enable_offset_removal(true);
	add_komponente( &bild );
	cursor.y += max( txt.get_groesse().y, bild.get_groesse().y );

	// Player's primary color label
	c1.set_pos( cursor );
	add_komponente( &c1 );
	cursor.y += LINESPACE+D_V_SPACE;

	// Get all colors (except the current player's)
	uint32 used_colors1 = 0;
	uint32 used_colors2 = 0;
	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		if(  i!=sp->get_player_nr()  &&  sp->get_welt()->get_spieler(i)  ) {
			used_colors1 |= 1 << (sp->get_welt()->get_spieler(i)->get_player_color1() / 8);
			used_colors2 |= 1 << (sp->get_welt()->get_spieler(i)->get_player_color2() / 8);
		}
	}

	// Primary color buttons
	for(unsigned i=0;  i<28;  i++) {
		player_color_1[i].init( button_t::box_state, (used_colors1 & (1<<(i+1)) ? "X" : ""), koord( cursor.x+(i%14)*(D_BUTTON_HEIGHT+D_H_SPACE), cursor.y+(i/14)*(D_BUTTON_HEIGHT+D_V_SPACE) ) , koord(D_BUTTON_HEIGHT,D_BUTTON_HEIGHT) );
		player_color_1[i].background = i*8+4;
		player_color_1[i].add_listener(this);
		add_komponente( player_color_1+i );
	}
	player_color_1[sp->get_player_color1()/8].pressed = true;
	cursor.y += 2*(D_BUTTON_HEIGHT+D_H_SPACE)+LINESPACE;

	// Player's secondary color label
	c2.set_pos( koord(D_MARGIN_LEFT,cursor.y) );
	add_komponente( &c2 );
	cursor.y += LINESPACE+D_V_SPACE;

	// Secondary color buttons
	for(unsigned i=0;  i<28;  i++) {
		player_color_2[i].init( button_t::box_state, (used_colors2 & (1<<(i+1)) ? "X" : ""), koord( cursor.x+(i%14)*(D_BUTTON_HEIGHT+D_H_SPACE), cursor.y+(i/14)*(D_BUTTON_HEIGHT+D_V_SPACE) ), koord(D_BUTTON_HEIGHT,D_BUTTON_HEIGHT) );
		player_color_2[i].background = i*8+4;
		player_color_2[i].add_listener(this);
		add_komponente( player_color_2+i );
	}
	player_color_2[sp->get_player_color2()/8].pressed = true;
	cursor.y += 2*D_BUTTON_HEIGHT+D_H_SPACE;

	// Put picture in place
	bild.align_to(&player_color_1[13],ALIGN_RIGHT);

	set_fenstergroesse( koord( D_MARGIN_LEFT + 14*D_BUTTON_HEIGHT + 13*D_H_SPACE + D_MARGIN_RIGHT, D_TITLEBAR_HEIGHT + cursor.y + D_MARGIN_BOTTOM ) );
}


/**
 * This method is called if an action is triggered
 * @author V. Meyer
 */
bool farbengui_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	for(unsigned i=0;  i<28;  i++) {

		// new player 1 color?
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
