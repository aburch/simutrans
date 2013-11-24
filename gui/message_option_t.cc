/*
 * Settings for player message options
 *
 * Copyright (c) 2006 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simmesg.h"
#include "../simskin.h"
#include "../simworld.h"

#include "../besch/skin_besch.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "message_option_t.h"

//#define BUTTON_ROW (110+D_MARGIN_LEFT+D_BUTTON_HEIGHT+D_H_SPACE)




message_option_t::message_option_t() :
	gui_frame_t( translator::translate("Mailbox Options") ),
	text_label(&buf),
	legend( skinverwaltung_t::message_options->get_bild_nr(0) )
{
	scr_coord_val button_row = get_windowsize().w - legend.get_size().w - D_MARGIN_RIGHT;

	buf.clear();
	buf.append(translator::translate("MessageOptionsText"));
	text_label.set_pos( scr_coord( D_MARGIN_LEFT + D_CHECKBOX_WIDTH + D_H_SPACE, D_MARGIN_TOP ));
	add_komponente( &text_label );

	legend.set_pos( scr_coord(button_row,0) );
	add_komponente( &legend );

	welt->get_message()->get_message_flags( &ticker_msg, &window_msg, &auto_msg, &ignore_msg );

	for(  int i=0;  i<message_t::MAX_MESSAGE_TYPE;  i++  ) {
		buttons[i*4].set_pos( scr_coord(D_MARGIN_LEFT,D_MARGIN_TOP+(i*2+1)*LINESPACE) );
		buttons[i*4].set_typ(button_t::square_state);
		buttons[i*4].pressed = ((ignore_msg>>i)&1)==0;
		buttons[i*4].add_listener(this);
		add_komponente( buttons+i*4 );

		buttons[i*4+1].set_pos( scr_coord(button_row+10,D_MARGIN_TOP+(i*2+1)*LINESPACE) );
		buttons[i*4+1].set_typ(button_t::square_state);
		buttons[i*4+1].pressed = (ticker_msg>>i)&1;
		buttons[i*4+1].add_listener(this);
		add_komponente( buttons+i*4+1 );

		buttons[i*4+2].set_pos( scr_coord(button_row+30,D_MARGIN_TOP+(i*2+1)*LINESPACE) );
		buttons[i*4+2].set_typ(button_t::square_state);
		buttons[i*4+2].pressed = (auto_msg>>i)&1;
		buttons[i*4+2].add_listener(this);
		add_komponente( buttons+i*4+2 );

		buttons[i*4+3].set_pos( scr_coord(button_row+50,D_MARGIN_TOP+(i*2+1)*LINESPACE) );
		buttons[i*4+3].set_typ(button_t::square_state);
		buttons[i*4+3].pressed = (window_msg>>i)&1;
		buttons[i*4+3].add_listener(this);
		add_komponente( buttons+i*4+3 );
	}
	set_windowsize( scr_size(button_row+70, D_TITLEBAR_HEIGHT+D_MARGIN_TOP+(message_t::MAX_MESSAGE_TYPE-1)*(LINESPACE<<1) + D_MARGIN_BOTTOM ) );
}


bool message_option_t::action_triggered( gui_action_creator_t *komp, value_t )
{
	((button_t*)komp)->pressed ^= 1;
	for(  int i=0;  i<message_t::MAX_MESSAGE_TYPE;  i++  ) {
		if(&buttons[i*4+0]==komp) {
			ignore_msg ^= (1<<i);
		}
		if(&buttons[i*4+1]==komp) {
			ticker_msg ^= (1<<i);
		}
		if(&buttons[i*4+2]==komp) {
			auto_msg ^= (1<<i);
		}
		if(&buttons[i*4+3]==komp) {
			window_msg ^= (1<<i);
		}
	}
	welt->get_message()->set_message_flags( ticker_msg, window_msg, auto_msg, ignore_msg );
	return true;
}
