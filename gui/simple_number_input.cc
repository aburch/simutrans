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
  
	set_table_layout(1,0);
  add_table(2,1);
  {
  	input.add_listener(this);
  	input.set_limits(min_val,max_val);
  	input.set_value(val);
  	input.set_increment_mode(1);
    add_component( &label );
  	add_component( &input );
  }
  end_table();
  
	reset_min_windowsize();
	set_windowsize(get_min_windowsize() );
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
