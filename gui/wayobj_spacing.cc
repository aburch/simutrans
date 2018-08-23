/*
 * Dialogue to set the wayobj spacing, when CTRL+clicking a wayobj on toolbar
 * Used by tool_roadsign_t
 */

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "components/gui_numberinput.h"

#include "wayobj_spacing.h"
#include "../simtool.h"

#define L_DIALOG_WIDTH (200)

uint8 wayobj_spacing_frame_t::spacing = 1;

wayobj_spacing_frame_t::wayobj_spacing_frame_t(player_t *player_, tool_build_wayobj_t* tool_) :
	gui_frame_t( translator::translate("set wayobj spacing") ),
	label("wayobj spacing")
{
	player = player_;
	tool = tool_;
	spacing = tool->get_spacing();

	scr_coord cursor(D_MARGIN_LEFT, D_MARGIN_TOP);

	spacing_inp.set_width_by_len(3);
	spacing_inp.set_pos( scr_coord( L_DIALOG_WIDTH - D_MARGIN_RIGHT - spacing_inp.get_size().w, cursor.y ) );
	spacing_inp.add_listener(this);
	spacing_inp.set_limits(1,50);
	spacing_inp.set_value(spacing);
	spacing_inp.set_increment_mode(1);
	add_component( &spacing_inp );

	label.align_to( &spacing_inp, ALIGN_CENTER_V, scr_coord( cursor.x, 0 ) );
	label.set_width( spacing_inp.get_pos().x - D_MARGIN_LEFT - D_H_SPACE );
	add_component( &label );

	cursor.y += spacing_inp.get_size().h + D_V_SPACE;

	set_windowsize( scr_size( L_DIALOG_WIDTH, D_TITLEBAR_HEIGHT + cursor.y + D_MARGIN_BOTTOM ) );
}

bool wayobj_spacing_frame_t::action_triggered( gui_action_creator_t *komp, value_t)
{
	if( komp == &spacing_inp ) {
		spacing = spacing_inp.get_value();
	}
	tool->set_spacing(spacing);
	return true;
}
