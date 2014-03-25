/*
 * Copyright (c) 2006 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * AI behavior options from AI finance window
 * 2006 prissi
 */

#include <stdio.h>

#include "../player/ai.h"

#include "../besch/skin_besch.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "ai_option_t.h"

#define L_DIALOG_WIDTH (200)

ai_option_t::ai_option_t( spieler_t *sp ) :
	gui_frame_t( translator::translate("Configure AI"), sp ),
	label_cs( "construction speed" )
{
	this->ai = dynamic_cast<ai_t *>(sp);

	scr_coord cursor(D_MARGIN_LEFT,D_MARGIN_TOP);

	label_cs.set_pos( cursor );
	add_komponente( &label_cs );
	cursor.y += LINESPACE;

	construction_speed.init( ai->get_construction_speed(), 25, 1000000, gui_numberinput_t::POWER2, false );
	construction_speed.set_pos( cursor );
	construction_speed.set_size( scr_size( L_DIALOG_WIDTH - D_MARGINS_X, D_EDIT_HEIGHT ) );
	construction_speed.add_listener( this );
	add_komponente( &construction_speed );
	cursor.y += max(D_EDIT_HEIGHT, LINESPACE) + D_V_SPACE;

	// find out if the mode is available and can be activated

	buttons[0].init( button_t::square_state, "road vehicle", cursor, scr_size( L_DIALOG_WIDTH - D_MARGINS_X, D_CHECKBOX_HEIGHT ) );
	buttons[0].pressed = ai->has_road_transport();
	buttons[0].add_listener( this );
	ai->set_road_transport( !buttons[0].pressed );
	if(  buttons[0].pressed==ai->has_road_transport()  ) {
		if(  ai->has_road_transport()  ) {
			buttons[0].disable();
			add_komponente( buttons+0 );
			cursor.y += max(D_CHECKBOX_HEIGHT, LINESPACE) + D_V_SPACE;
		}
	}
	else {
		ai->set_road_transport( buttons[0].pressed );
		add_komponente( buttons+0 );
		cursor.y += max(D_CHECKBOX_HEIGHT, LINESPACE) + D_V_SPACE;
	}

	buttons[1].init( button_t::square_state, "rail car", cursor, scr_size( L_DIALOG_WIDTH - D_MARGINS_X, D_CHECKBOX_HEIGHT ) );
	buttons[1].pressed = ai->has_rail_transport();
	buttons[1].add_listener( this );
	ai->set_rail_transport( !buttons[1].pressed );
	if(  buttons[1].pressed==ai->has_rail_transport()  ) {
		if(  ai->has_rail_transport()  ) {
			buttons[1].disable();
			add_komponente( buttons+1 );
			cursor.y += max(D_CHECKBOX_HEIGHT, LINESPACE) + D_V_SPACE;
		}
	}
	else {
		ai->set_rail_transport( buttons[1].pressed );
		add_komponente( buttons+1 );
		cursor.y += max(D_CHECKBOX_HEIGHT, LINESPACE) + D_V_SPACE;
	}

	buttons[2].init( button_t::square_state, "water vehicle", cursor, scr_size( L_DIALOG_WIDTH - D_MARGINS_X, D_CHECKBOX_HEIGHT ) );
	buttons[2].pressed = ai->has_ship_transport();
	buttons[2].add_listener( this );
	ai->set_ship_transport( !buttons[2].pressed );
	if(  buttons[2].pressed==ai->has_ship_transport()  ) {
		if(  ai->has_ship_transport()  ) {
			buttons[2].disable();
			add_komponente( buttons+2 );
			cursor.y += max(D_CHECKBOX_HEIGHT, LINESPACE) + D_V_SPACE;
		}
	}
	else {
		ai->set_ship_transport( buttons[2].pressed );
		add_komponente( buttons+2 );
		cursor.y += max(D_CHECKBOX_HEIGHT, LINESPACE) + D_V_SPACE;
	}

	buttons[3].init( button_t::square_state, "airplane", cursor, scr_size( L_DIALOG_WIDTH - D_MARGINS_X, D_CHECKBOX_HEIGHT ) );
	buttons[3].pressed = ai->has_air_transport();
	buttons[3].add_listener( this );
	ai->set_air_transport( !buttons[3].pressed );
	if(  buttons[3].pressed==ai->has_air_transport()  ) {
		if(  ai->has_air_transport()  ) {
			buttons[3].disable();
			add_komponente( buttons+3 );
			cursor.y += max(D_CHECKBOX_HEIGHT, LINESPACE) + D_V_SPACE;
		}
	}
	else {
		ai->set_air_transport( buttons[3].pressed );
		add_komponente( buttons+3 );
		cursor.y += max(D_CHECKBOX_HEIGHT, LINESPACE) + D_V_SPACE;
	}

	// Remove one D_V_SPACE so bottom margin is exactly D_MARGIN_BOTTOM
	set_windowsize( scr_size(L_DIALOG_WIDTH, D_TITLEBAR_HEIGHT + cursor.y + D_MARGIN_BOTTOM - D_V_SPACE) );
}


bool ai_option_t::action_triggered( gui_action_creator_t *komp, value_t v )
{
	if(  komp==&construction_speed  ) {
		ai->set_construction_speed( v.i );
	}
	else if(  komp==buttons+0  ) {
		ai->set_road_transport( !buttons[0].pressed );
		buttons[0].pressed = ai->has_road_transport();
	}
	else if(  komp==buttons+1  ) {
		ai->set_rail_transport( !buttons[1].pressed );
		buttons[1].pressed = ai->has_rail_transport();
	}
	else if(  komp==buttons+2  ) {
		ai->set_ship_transport( !buttons[2].pressed );
		buttons[2].pressed = ai->has_ship_transport();
	}
	else if(  komp==buttons+3  ) {
		ai->set_air_transport( !buttons[3].pressed );
		buttons[3].pressed = ai->has_air_transport();
	}
	return true;
}
