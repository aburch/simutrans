/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simmesg.h"
#include "../simskin.h"
#include "../world/simworld.h"

#include "../descriptor/skin_desc.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "message_option.h"
#include "components/gui_image.h"


message_option_t::message_option_t() :
	gui_frame_t( translator::translate("Mailbox Options") )
{
	set_table_layout(5,0);

	// first row images
	new_component_span<gui_empty_t>(2);
	if (skinverwaltung_t::message_options->get_count() >=3 ) {
		// three single images
		for(int i=0; i<3; i++) {
			new_component<gui_image_t>(skinverwaltung_t::message_options->get_image_id(i), 0, 0, true);
		}
	}
	else {
		// one monolithic image
		new_component_span<gui_image_t>(skinverwaltung_t::message_options->get_image_id(0), 0, 0, true, 3);
	}

	// The text is unfortunately a single text, which we have to chop into pieces.
	const unsigned char *p = (const unsigned char *)translator::translate( "MessageOptionsText" );
	welt->get_message()->get_message_flags( &ticker_msg, &window_msg, &auto_msg, &ignore_msg );

	for(  int i=0;  i<message_t::MAX_MESSAGE_TYPE;  i++  ) {

		buttons[i*4].set_typ(button_t::square_state);
		buttons[i*4].pressed = ((ignore_msg>>i)&1)==0;
		buttons[i*4].add_listener(this);
		add_component( buttons+i*4 );

		// copy the next line of the option text
		while(  *p < ' '  &&  *p  ) {
			p++;
		}
		for(  int j=0;   *p>=' '; p++  ) {
			if(  j < MAX_MESSAGE_OPTION_TEXTLEN-1  ) {
				option_texts[i][j++] = *p;
				option_texts[i][j] = 0;
			}
		}
		text_lbl[i].set_text( option_texts[i] );
		add_component( text_lbl+i );

		buttons[i*4+1].set_typ(button_t::square_state);
		buttons[i*4+1].pressed = (ticker_msg>>i)&1;
		buttons[i*4+1].add_listener(this);
		add_component( buttons+i*4+1 );

		buttons[i*4+2].set_typ(button_t::square_state);
		buttons[i*4+2].pressed = (auto_msg>>i)&1;
		buttons[i*4+2].add_listener(this);
		add_component( buttons+i*4+2 );

		buttons[i*4+3].set_typ(button_t::square_state);
		buttons[i*4+3].pressed = (window_msg>>i)&1;
		buttons[i*4+3].add_listener(this);
		add_component( buttons+i*4+3 );
	}

	reset_min_windowsize();
}


bool message_option_t::action_triggered( gui_action_creator_t *comp, value_t )
{
	((button_t*)comp)->pressed ^= 1;
	for(  int i=0;  i<message_t::MAX_MESSAGE_TYPE;  i++  ) {
		if(&buttons[i*4+0]==comp) {
			ignore_msg ^= (1<<i);
		}
		if(&buttons[i*4+1]==comp) {
			ticker_msg ^= (1<<i);
		}
		if(&buttons[i*4+2]==comp) {
			auto_msg ^= (1<<i);
		}
		if(&buttons[i*4+3]==comp) {
			window_msg ^= (1<<i);
		}
	}
	welt->get_message()->set_message_flags( ticker_msg, window_msg, auto_msg, ignore_msg );
	return true;
}
