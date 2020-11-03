/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simwin.h"
#include "minimap.h"

#include "../dataobj/schedule.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../simline.h"
#include "../simtool.h"
#include "../utils/cbuffer_t.h"

#include "line_management_gui.h"

line_management_gui_t::line_management_gui_t( linehandle_t line_, player_t* player_ ) :
	gui_frame_t( translator::translate( "Fahrplan" ), player_ ),
	scd( line_->get_schedule(), player_, convoihandle_t(), line_ )
{
	set_table_layout(1, 0);
	add_component( &scd );
	line = line_;
	if (line.is_bound() ) {
		// has this line a single running convoi?
		if(  line->count_convoys() > 0  ) {
			minimap_t::get_instance()->set_selected_cnv( line->get_convoy(0) );
		}
	}

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
	set_windowsize(get_windowsize());
}


bool line_management_gui_t::infowin_event(const event_t *ev)
{
	if(  !line.is_bound()  ) {
		destroy_win( this );
	}
	else  {
		bool swallowed = gui_frame_t::infowin_event(ev);

		if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE  ) {
			// update line schedule via tool!
			tool_t *tool = create_tool( TOOL_CHANGE_LINE | SIMPLE_TOOL );
			cbuffer_t buf;
			buf.printf( "g,%i,", line.get_id() );
			scd.get_schedule()->sprintf_schedule( buf );
			tool->set_default_param(buf);
			world()->set_tool( tool, line->get_owner() );
			// since init always returns false, it is safe to delete immediately
			delete tool;
			return true;
		}
		return swallowed;
	}

	return gui_frame_t::infowin_event(ev);
}


void line_management_gui_t::rdwr(loadsave_t *file)
{
	scr_size size = get_windowsize();
	size.rdwr( file );
	simline_t::rdwr_linehandle_t(file, line);

	scd.rdwr( file );

	if(  file->is_loading()  ) {
		if(  line.is_bound()  ) {
			set_windowsize(size);
			win_set_magic(this, (ptrdiff_t)line.get_rep());
		}
		else {
			line = linehandle_t();
			destroy_win( this );
			dbg->error( "line_management_gui_t::rdwr", "Could not restore schedule window for line id %i", line.get_id() );
		}
	}
}
