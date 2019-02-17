/*
 * Dialogue to set a value, when CTRL+clicking an icon on toolbar
 * @author THLeaderH
 */

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "components/gui_numberinput.h"

#include "simple_number_input.h"
#include "../simtool.h"

#define L_DIALOG_WIDTH (200)

simple_number_input_frame_t::simple_number_input_frame_t (const char* frame_title, const char* label_text, uint8 min_val, uint8 max_val, uint8 default_val) :
 	gui_frame_t( translator::translate(frame_title) ),
	label(label_text)
{
	val = default_val;
	scr_coord cursor(D_MARGIN_LEFT, D_MARGIN_TOP);
	
	//input.set_width_by_len(3);
	input.set_pos( scr_coord( L_DIALOG_WIDTH - D_MARGIN_RIGHT - input.get_size().w, cursor.y ) );
	input.add_listener(this);
	input.set_limits(min_val,max_val);
	input.set_value(val);
	input.set_increment_mode(1);
	add_component( &input );

	label.align_to( &input, ALIGN_CENTER_V, scr_coord( cursor.x, 0 ) );
	label.set_width( input.get_pos().x - D_MARGIN_LEFT - D_H_SPACE );
	add_component( &label );

	cursor.y += input.get_size().h + D_V_SPACE;

	set_windowsize( scr_size( L_DIALOG_WIDTH, D_TITLEBAR_HEIGHT + cursor.y + D_MARGIN_BOTTOM ) );
}

bool simple_number_input_frame_t::action_triggered( gui_action_creator_t *komp, value_t)
{
	if( komp == &input ) {
		register_val(input.get_value());
	}
	return true;
}

wayobj_spacing_frame_t::wayobj_spacing_frame_t(player_t *player_, tool_build_wayobj_t* tool_) :
	simple_number_input_frame_t( translator::translate("set wayobj spacing"), translator::translate("wayobj spacing"), 1, 50, tool_->get_spacing() )
{
	tool = tool_;
}

void wayobj_spacing_frame_t::register_val(uint8 v) {
	tool->set_spacing(v);
}

height_offset_frame_t::height_offset_frame_t(player_t *player_, tool_build_way_t* tool_) :
	simple_number_input_frame_t( translator::translate("set height offset"), translator::translate("height offset"), 0, 32, tool_->get_height_offset() )
{
	tool = tool_;
}

void height_offset_frame_t::register_val(uint8 v) {
	tool->set_height_offset(v);
}
