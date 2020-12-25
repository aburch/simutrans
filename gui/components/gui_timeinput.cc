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
	gui_component_t(true),
	null_text(null_text_)
{
	text_width = proportional_string_width("+30 23:55h");

	set_ticks( 0 );
	repeat_steps = 0;

	bt_left.set_typ(button_t::repeatarrowleft );
	bt_left.set_pos( scr_coord(0,0) );

	time_out.set_color( SYSCOL_EDIT_TEXT );
	time_out.set_text_pointer( textbuffer );
	time_out.set_align( gui_label_t::centered );

	bt_right.set_typ(button_t::repeatarrowright );

	b_enabled = true;

	set_size( scr_size(0,0) );
}


void gui_timeinput_t::set_size(scr_size size_par)
{
	bt_left.set_pos( scr_coord(0,(size.h-D_ARROW_LEFT_HEIGHT)/2) );
	time_out.set_size( scr_size(text_width+1,LINESPACE) );
	time_out.align_to( &bt_left, ALIGN_LEFT | ALIGN_EXTERIOR_H | ALIGN_CENTER_V, scr_coord( D_H_SPACE, 0) );
	bt_right.align_to( &bt_left, ALIGN_LEFT | ALIGN_EXTERIOR_H, scr_coord( D_H_SPACE*2+text_width, 0) );

	size_par.w = min( D_ARROW_LEFT_WIDTH + D_ARROW_RIGHT_WIDTH + 2 * D_H_SPACE + text_width, size_par.w );
	size_par.h = min( max(D_ARROW_LEFT_HEIGHT,LINESPACE), size_par.h );

	gui_component_t::set_size(size_par);
}


scr_size gui_timeinput_t::get_max_size() const
{
	return get_min_size();
}


scr_size gui_timeinput_t::get_min_size() const
{
	return scr_size(  text_width + D_ARROW_LEFT_WIDTH + D_ARROW_RIGHT_WIDTH + D_H_SPACE*2,
		max(LINESPACE, max(D_ARROW_LEFT_HEIGHT, D_ARROW_RIGHT_HEIGHT)) );
}


void gui_timeinput_t::set_ticks( uint16 new_ticks )
{
	sint32 disp_ticks = (world()->ticks_per_world_month_shift >= 16) ? (new_ticks << (world()->ticks_per_world_month_shift - 16)) : (new_ticks >> (16 - world()->ticks_per_world_month_shift));

	if(  disp_ticks == 0  &&  null_text  ) {
		tstrncpy( textbuffer, translator::translate(null_text), lengthof(textbuffer) );
	}
	else {
		tstrncpy( textbuffer, difftick_to_string( disp_ticks, false ), lengthof(textbuffer) );
	}

	time_out.set_color( b_enabled ? SYSCOL_EDIT_TEXT : SYSCOL_EDIT_TEXT_DISABLED );
	ticks = new_ticks;
}


bool gui_timeinput_t::infowin_event(const event_t *ev)
{
	if( ev->ev_class == EVENT_RELEASE ) {
		repeat_steps = 0;
	}

	// buttons pressed
	sint32 increment = 0;
	if(  bt_left.getroffen(ev->cx, ev->cy)  &&  ev->ev_code == MOUSE_LEFTBUTTON  ) {
		increment = -1;
	}
	else if(  bt_right.getroffen(ev->cx, ev->cy)  &&  ev->ev_code == MOUSE_LEFTBUTTON  ) {
		increment = 1;
	}

	// something changed?
	if( increment != 0 ) {
		if( ev->ev_class == EVENT_REPEAT ) {
			repeat_steps++;
		}
		if( repeat_steps >= 75 ) {
			increment *= 1000;
		}
		else if( repeat_steps >= 60 ) {
			increment *= 240;
		}
		else if( repeat_steps >= 45 ) {
			increment *= 60;
		}
		else if( repeat_steps >= 30 ) {
			increment *= 15;
		}
		else if( repeat_steps >= 15 ) {
			increment *= 5;
		}
		set_ticks( ticks + world()->ticks_per_world_month + increment );
		value_t v( ticks );
		call_listeners( v );
		return true;
	}

	return false;
}


void gui_timeinput_t::draw(scr_coord offset)
{
	scr_coord new_offset = pos+offset;

	bt_left.draw(new_offset);
	time_out.draw(new_offset);
	bt_right.draw(new_offset);
	
	if(b_enabled  &&  getroffen( get_mouse_x()-offset.x, get_mouse_y()-offset.y )) {
		win_set_tooltip(get_mouse_x() + TOOLTIP_MOUSE_OFFSET_X, new_offset.y + size.h + TOOLTIP_MOUSE_OFFSET_Y, translator::translate("Enter intervall in days, hours minutes" ), this);
	}
}


bool gui_timeinput_t::compare( const gui_component_t *a, const gui_component_t *b )
{
	return dynamic_cast<const gui_timeinput_t *>(a)->get_ticks() < dynamic_cast<const gui_timeinput_t *>(b)->get_ticks();
}



void gui_timeinput_t::rdwr( loadsave_t *file )
{
	file->rdwr_short( ticks );
	set_ticks( ticks );
}
