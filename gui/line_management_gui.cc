/*
 * part of the Simutrans project
 * @author hsiegeln
 * 01/12/2003
 */

#include "../dataobj/schedule.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../gui/karte.h"
#include "../simline.h"
#include "../gui/simwin.h"
#include "../simtool.h"
#include "../simlinemgmt.h"
#include "../utils/cbuffer_t.h"

#include "line_management_gui.h"

line_management_gui_t::line_management_gui_t(linehandle_t line, player_t* player_) :
	schedule_gui_t(line->get_schedule()->copy(), player_, convoihandle_t() )
{
	this->line = line;
	// has this line a single running convoi?
	if(  line->count_convoys() > 0  ) {
		reliefkarte_t::get_karte()->set_current_cnv( line->get_convoy(0) );
	}
	show_line_selector(false);
}


line_management_gui_t::~line_management_gui_t()
{
	delete old_schedule;	// since we pass a *copy* of the line's schedule to the base class
	old_schedule = NULL;
}


bool line_management_gui_t::infowin_event(const event_t *ev)
{
	if(  player!=NULL  ) {
		// not "magic_line_schedule_rdwr_dummy" during loading of UI ...
		if(  !line.is_bound()  ) {
			destroy_win( this );
		}
		else  {
			if(  schedule_gui_t::infowin_event(ev)  ) {
				return true;
			}
			if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE  ) {
				// update line schedule via tool!
				tool_t *tool = create_tool( TOOL_CHANGE_LINE | SIMPLE_TOOL );
				cbuffer_t buf;
				buf.printf( "g,%i,", line.get_id() );
				schedule->sprintf_schedule( buf );
				tool->set_default_param(buf);
				welt->set_tool( tool, line->get_owner() );
				// since init always returns false, it is safe to delete immediately
				delete tool;
			}
		}
	}
	return false;
}



line_management_gui_t::line_management_gui_t() :
	schedule_gui_t()
{
	show_line_selector(false);
}

void line_management_gui_t::rdwr(loadsave_t *file)
{
	// this handles only schedules of bound convois
	// lines are handled by line_management_gui_t
	uint8 player_nr;
	scr_size size = get_windowsize();
	if(  file->is_saving()  ) {
		player_nr = line->get_owner()->get_player_nr();
	}
	else {
		// dummy types
		old_schedule = new truck_schedule_t();
		schedule = new truck_schedule_t();
	}
	size.rdwr( file );
	file->rdwr_byte( player_nr );
	simline_t::rdwr_linehandle_t(file, line);
	old_schedule->rdwr(file);
	schedule->rdwr(file);
	if(  file->is_loading()  ) {
		player_t *player = welt->get_player(player_nr);
		assert(player);	// since it was alive during saving, this should never happen

		if(  line.is_bound()  &&  old_schedule->matches( welt, line->get_schedule() )  ) {
			// now we can open the window ...
			scr_coord const& pos = win_get_pos(this);
			line_management_gui_t *w = new line_management_gui_t( line, player );
			create_win(pos.x, pos.y, w, w_info, (ptrdiff_t)line.get_rep());
			w->set_windowsize( size );
			w->schedule->copy_from( schedule );
		}
		else {
			dbg->error( "line_management_gui_t::rdwr", "Could not restore schedule window for line id %i", line.get_id() );
		}
		player = NULL;
		delete old_schedule;
		delete schedule;
		schedule = old_schedule = NULL;
		line = linehandle_t();
		destroy_win( this );
	}
}
