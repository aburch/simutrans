/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_timeinput.h"
#include "../gui_frame.h"
#include "../simwin.h"
#include "../../display/simgraph.h"
#include "../../macros.h"
#include "../../dataobj/translator.h"
#include "../../utils/simstring.h"
#include "../../simintr.h"
#include "../../simworld.h"


scr_coord_val gui_timeinput_t::text_width;


gui_timeinput_t::gui_timeinput_t(const char *null_text_) :
	null_text(null_text_)
{
	text_width = proportional_string_width("+30 23:55h");

	set_ticks( 0 );
	repeat_steps = 0;

	gui_numberinput_t::init( 65535, 0, 65535, 2114, true );

	time_out.set_color( SYSCOL_EDIT_TEXT );
	time_out.set_text_pointer( textbuffer );
	time_out.set_align( gui_label_t::left );
	time_out.set_pos( scr_coord( gui_numberinput_t::get_min_size().w+D_H_SPACE, (gui_numberinput_t::get_min_size().h-LINESPACE)/2 ) );

	time_out.set_size( scr_size( text_width, gui_numberinput_t::get_size().h ) );

	b_enabled = true;
}


scr_size gui_timeinput_t::get_max_size() const
{
	return get_min_size();
}


scr_size gui_timeinput_t::get_min_size() const
{
	scr_size sz = gui_numberinput_t::get_min_size();
	return sz+scr_size( text_width+D_H_SPACE, 0 );
}


void  gui_timeinput_t::set_size( scr_size sz )
{
	gui_numberinput_t::set_size( sz - scr_size( text_width, 0 ) );
}


scr_size gui_timeinput_t::get_size() const
{
	return gui_numberinput_t::get_size() + scr_size( text_width, 0 );
}


void gui_timeinput_t::draw(scr_coord offset)
{
	gui_numberinput_t::draw( offset );

	uint16 new_ticks = gui_numberinput_t::get_value();
	sint32 disp_ticks = (world()->ticks_per_world_month_shift >= 16) ? (new_ticks << (world()->ticks_per_world_month_shift - 16)) : (new_ticks >> (16 - world()->ticks_per_world_month_shift));
	if(  disp_ticks == 0  &&  null_text  ) {
		tstrncpy( textbuffer, translator::translate(null_text), lengthof(textbuffer) );
	}
	else {
		tstrncpy( textbuffer, difftick_to_string( disp_ticks, false ), lengthof(textbuffer) );
	}
	time_out.set_color( b_enabled ? SYSCOL_EDIT_TEXT : SYSCOL_EDIT_TEXT_DISABLED );
	time_out.draw( pos+offset );

	if(b_enabled  &&  gui_numberinput_t::getroffen( get_mouse_x()-offset.x, get_mouse_y()-offset.y )) {
		win_set_tooltip(get_mouse_x() + TOOLTIP_MOUSE_OFFSET_X,get_mouse_y() + TOOLTIP_MOUSE_OFFSET_Y, translator::translate("Enter intervall in days, hours minutes" ), this);
	}
}


void gui_timeinput_t::rdwr( loadsave_t *file )
{
	uint16 ticks = get_value();
	file->rdwr_short( ticks );
	set_value( ticks );
}
