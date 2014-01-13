/*
 * Dialogue to set the signal spacing, when CTRL+clicking a signal on toolbar
 * Used by wkz_roadsign_t
 */

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "components/gui_numberinput.h"

#include "signal_spacing.h"
#include "../simwerkz.h"

#define L_DIALOG_WIDTH (200)

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

	scr_coord cursor(D_MARGIN_LEFT, D_MARGIN_TOP);

	signal_spacing_inp.set_width_by_len(3);
	signal_spacing_inp.set_pos( scr_coord( L_DIALOG_WIDTH - D_MARGIN_RIGHT - signal_spacing_inp.get_size().w, cursor.y ) );
	signal_spacing_inp.add_listener(this);
	signal_spacing_inp.set_limits(1,50);
	signal_spacing_inp.set_value(signal_spacing);
	signal_spacing_inp.set_increment_mode(1);
	add_komponente( &signal_spacing_inp );

	signal_label.align_to( &signal_spacing_inp, ALIGN_CENTER_V, scr_coord( cursor.x, 0 ) );
	signal_label.set_width( signal_spacing_inp.get_pos().x - D_MARGIN_LEFT - D_H_SPACE );
	add_komponente( &signal_label );

	cursor.y += signal_spacing_inp.get_size().h + D_V_SPACE;

	remove_button.init( button_t::square_state, "remove interm. signals", cursor );
	remove_button.set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	remove_button.add_listener(this);
	remove_button.pressed = remove;
	add_komponente( &remove_button );
	cursor.y += remove_button.get_size().h + D_V_SPACE;

	replace_button.init( button_t::square_state, "replace other signals", cursor );
	replace_button.set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	replace_button.add_listener(this);
	replace_button.pressed = replace;
	add_komponente( &replace_button );
	cursor.y += replace_button.get_size().h;

	set_windowsize( scr_size( L_DIALOG_WIDTH, D_TITLEBAR_HEIGHT + cursor.y + D_MARGIN_BOTTOM ) );
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
