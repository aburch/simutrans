/*
 * Dialogue to set the signal spacing.
 * Used by wkz_roadsign_t
 */

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "components/gui_numberinput.h"

#include "signal_spacing.h"
#include "../simwerkz.h"

uint8 signal_spacing_frame_t::signal_spacing = 2;
bool signal_spacing_frame_t::remove = true;
bool signal_spacing_frame_t::replace = true;

signal_spacing_frame_t::signal_spacing_frame_t(spieler_t *sp_, wkz_roadsign_t* tool_) :
	gui_frame_t( translator::translate("set signal spacing") ),
	signal_label("signal spacing")
{
	sp = sp_;
	tool = tool_;
	tool->get_values(sp, signal_spacing, remove, replace);

	int intTopOfButton = 12;

	signal_label.set_pos( koord(10, intTopOfButton+2) );
	signal_label.set_groesse(koord(10, 50));
	add_komponente( &signal_label );

	signal_spacing_inp.set_pos( koord(140,intTopOfButton) );
	signal_spacing_inp.set_groesse(koord(50,12));
	signal_spacing_inp.add_listener(this);
	signal_spacing_inp.set_limits(1,50);
	signal_spacing_inp.set_value(signal_spacing);
	signal_spacing_inp.set_increment_mode(1);
	add_komponente( &signal_spacing_inp );
	intTopOfButton += 12+4;

	remove_button.init(button_t::square_state,"remove interm. signals", koord(10,intTopOfButton), koord(100,10));
	remove_button.add_listener(this);
	remove_button.pressed = remove;
	add_komponente( &remove_button );
	intTopOfButton += 12;

	replace_button.init(button_t::square_state,"replace other signals", koord(10,intTopOfButton), koord(100,10));
	replace_button.add_listener(this);
	replace_button.pressed = replace;
	add_komponente( &replace_button );
	intTopOfButton += 12+4;

	set_fenstergroesse( koord(110+80+10, intTopOfButton+38) );
}

bool signal_spacing_frame_t::action_triggered( gui_action_creator_t *komp, value_t)
{
	if( komp == &signal_spacing_inp ) {
		signal_spacing = signal_spacing_inp.get_value();
	}
	else if( komp == &remove_button ) {
		remove = !remove;
		remove_button.pressed = remove;
	}
	else if( komp == &replace_button ) {
		replace = !replace;
		replace_button.pressed = replace;
	}
	tool->set_values(sp, signal_spacing, remove, replace);
	return true;
}
