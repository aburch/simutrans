/*
 * part of the Simutrans project
 * @author hsiegeln
 * 01/12/2003
 */

#include "../dataobj/fahrplan.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../gui/karte.h"
#include "../simline.h"
#include "../simwin.h"
#include "../simwerkz.h"
#include "../simlinemgmt.h"
#include "../utils/cbuffer_t.h"

#include "line_management_gui.h"

line_management_gui_t::line_management_gui_t(linehandle_t line, spieler_t* sp) :
	fahrplan_gui_t(line->get_schedule()->copy(), sp, convoihandle_t() )
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
	delete old_fpl;	// since we pass a *copy* of the line's schedule to the base class
	old_fpl = NULL;
}



const char *line_management_gui_t::get_name() const
{
	return translator::translate("Line Management");
}


bool line_management_gui_t::infowin_event(const event_t *ev)
{
	if(  sp!=NULL  ) {
		// not "magic_line_schedule_rdwr_dummy" during loading of UI ...
		if(  !line.is_bound()  ) {
			destroy_win( this );
		}
		else  {
			if(  fahrplan_gui_t::infowin_event(ev)  ) {
				return true;
			}
			if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE  ) {
				// update line schedule via tool!
				werkzeug_t *w = create_tool( WKZ_LINE_TOOL | SIMPLE_TOOL );
				cbuffer_t buf;
				buf.printf( "g,%i,", line.get_id() );
				fpl->sprintf_schedule( buf );
				w->set_default_param(buf);
				line->get_besitzer()->get_welt()->set_werkzeug( w, line->get_besitzer() );
				// since init always returns false, it is save to delete immediately
				delete w;
			}
		}
	}
	return false;
}



line_management_gui_t::line_management_gui_t(karte_t *welt) :
	fahrplan_gui_t( welt )
{
	show_line_selector(false);
}

void line_management_gui_t::rdwr(loadsave_t *file)
{
	// this handles only schedules of bound convois
	// lines are handled by line_management_gui_t
	uint8 player_nr;
	koord gr = get_fenstergroesse();
	if(  file->is_saving()  ) {
		player_nr = line->get_besitzer()->get_player_nr();
	}
	else {
		// dummy types
		old_fpl = new autofahrplan_t();
		fpl = new autofahrplan_t();
	}
	gr.rdwr( file );
	file->rdwr_byte( player_nr );
	simline_t::rdwr_linehandle_t(file, line);
	old_fpl->rdwr(file);
	fpl->rdwr(file);
	if(  file->is_loading()  ) {
		spieler_t *sp = welt->get_spieler(player_nr);
		assert(sp);	// since it was alive during saving, this shoudl never happen

		if(  line.is_bound()  &&  old_fpl->matches( welt, line->get_schedule() )  ) {
			// now we can open the window ...
			koord const& pos = win_get_pos(this);
			line_management_gui_t *w = new line_management_gui_t( line, sp );
			create_win(pos.x, pos.y, w, w_info, (ptrdiff_t)line.get_rep());
			w->set_fenstergroesse( gr );
			w->fpl->copy_from( fpl );
		}
		else {
			dbg->error( "line_management_gui_t::rdwr", "Could not restore schedule window for line id %i", line.get_id() );
		}
		sp = NULL;
		delete old_fpl;
		delete fpl;
		fpl = old_fpl = NULL;
		line = linehandle_t();
		destroy_win( this );
	}
}
