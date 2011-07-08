/*
 * part of the Simutrans project
 * @author hsiegeln
 * 01/12/2003
 */

#include <algorithm>
#include "simlinemgmt.h"
#include "simline.h"
#include "simhalt.h"
#include "simwin.h"
#include "simworld.h"
#include "simtypes.h"
#include "simintr.h"

#include "dataobj/fahrplan.h"
#include "dataobj/loadsave.h"

#include "gui/schedule_list.h"

#include "player/simplay.h"

karte_t *simlinemgmt_t::welt = NULL;


simlinemgmt_t::simlinemgmt_t(karte_t* welt)
{
	this->welt = welt;
	schedule_list_gui = NULL;
}


simlinemgmt_t::~simlinemgmt_t()
{
	destroy_win( (long)this );
	// and delete all lines ...
	while (!all_managed_lines.empty()) {
		linehandle_t line = all_managed_lines.back();
		all_managed_lines.remove_at( all_managed_lines.get_count()-1 );
		delete line.get_rep();	// detaching handled by line itself
	}
}


void simlinemgmt_t::line_management_window(spieler_t *sp)
{
	schedule_list_gui = dynamic_cast<schedule_list_gui_t *>(win_get_magic( magic_line_management_t+sp->get_player_nr() ));
	if(  schedule_list_gui==NULL  ) {
		schedule_list_gui = new schedule_list_gui_t(sp);
		create_win( schedule_list_gui, w_info, magic_line_management_t+sp->get_player_nr() );
	}
}


void simlinemgmt_t::add_line(linehandle_t new_line)
{
	all_managed_lines.append(new_line);
}


void simlinemgmt_t::delete_line(linehandle_t line)
{
	if (line.is_bound()) {
		all_managed_lines.remove(line);
		//destroy line object
		delete line.get_rep();
	}
}


void simlinemgmt_t::update_line(linehandle_t line)
{
	// when a line is updated, all managed convoys must get the new fahrplan!
	int count = line->count_convoys();
	for(int i = 0; i<count; i++) {
		line->get_convoy(i)->set_update_line(line);
	}
	// finally de/register all stops
	line->renew_stops();
}



void simlinemgmt_t::rdwr(karte_t * welt, loadsave_t *file, spieler_t *sp)
{
	xml_tag_t l( file, "simlinemgmt_t" );

	if(file->is_saving()) {
		// on old versions
		if(  file->get_version()<101000  ) {
			file->wr_obj_id("Linemanagement");
		}

		// ensure that lines are saved in the same order on all clients
		sort_lines();

		uint32 count = all_managed_lines.get_count();
		file->rdwr_long(count);
		for (vector_tpl<linehandle_t>::const_iterator i = all_managed_lines.begin(), end = all_managed_lines.end(); i != end; i++) {
			simline_t::linetype lt = (*i)->get_linetype();
			file->rdwr_enum(lt);
			(*i)->rdwr(file);
		}
	}
	else {
		// on old versions
		if(  file->get_version()<101000  ) {
			char buf[80];
			file->rd_obj_id(buf, 79);
			all_managed_lines.clear();
			if(strcmp(buf, "Linemanagement")!=0) {
				dbg->fatal( "simlinemgmt_t::rdwr", "Broken Savegame" );
			}
		}
		sint32 totalLines = 0;
		file->rdwr_long(totalLines);
DBG_MESSAGE("simlinemgmt_t::rdwr()","number of lines=%i",totalLines);

		simline_t *unbound_line = NULL;

		for (int i = 0; i<totalLines; i++) {
			simline_t::linetype lt=simline_t::line;
			file->rdwr_enum(lt);

			if(lt < simline_t::truckline  ||  lt > simline_t::narrowgaugeline) {
					dbg->fatal( "simlinemgmt_t::rdwr()", "Cannot create default line!" );
			}
			simline_t *line = new simline_t(welt, sp, lt, file);
			if (!line->get_handle().is_bound()) {
				// line id was saved as zero ...
				if (unbound_line) {
					dbg->fatal( "simlinemgmt_t::rdwr()", "More than one line with unbound id read" );
				}
				else {
					unbound_line = line;
				}
			}
			else {
				add_line( line->get_handle() );
			}
		}

		if (unbound_line) {
			// linehandle will be corrected in simline_t::laden_abschliessen
			line_with_id_zero = linehandle_t(unbound_line);
			add_line( line_with_id_zero );
		}
	}
}


static bool compare_lines(const linehandle_t& a, const linehandle_t& b)
{
	int diff = 0;
	if(  a->get_name()[0]=='('  &&  b->get_name()[0]=='('  ) {
		diff = atoi(a->get_name()+1)-atoi(b->get_name()+1);
	}
	if(  diff==0  ) {
		diff = strcmp(a->get_name(), b->get_name());
	}
	if(diff==0) {
		diff = a.get_id() - b.get_id();
	}
	return diff < 0;
}


void simlinemgmt_t::sort_lines()
{
	std::sort(all_managed_lines.begin(), all_managed_lines.end(), compare_lines);
}


void simlinemgmt_t::laden_abschliessen()
{
	for (vector_tpl<linehandle_t>::const_iterator i = all_managed_lines.begin(), end = all_managed_lines.end(); i != end; i++) {
		(*i)->laden_abschliessen();
	}
	sort_lines();
}


void simlinemgmt_t::rotate90( sint16 y_size )
{
	for (vector_tpl<linehandle_t>::const_iterator i = all_managed_lines.begin(), end = all_managed_lines.end(); i != end; i++) {
		schedule_t *fpl = (*i)->get_schedule();
		if(fpl) {
			fpl->rotate90( y_size );
		}
	}
}


void simlinemgmt_t::new_month()
{
	for (vector_tpl<linehandle_t>::const_iterator i = all_managed_lines.begin(), end = all_managed_lines.end(); i != end; i++) {
		(*i)->new_month();
	}
}


linehandle_t simlinemgmt_t::create_line(int ltype, spieler_t * sp)
{
	if(ltype < simline_t::truckline  ||  ltype > simline_t::narrowgaugeline) {
			dbg->fatal( "simlinemgmt_t::create_line()", "Cannot create default line!" );
	}

	simline_t * line = new simline_t(welt, sp, (simline_t::linetype)ltype);

	add_line( line->get_handle() );
	sort_lines();
	return line->get_handle();
}



linehandle_t simlinemgmt_t::create_line(int ltype, spieler_t * sp, schedule_t * fpl)
{
	linehandle_t line = create_line( ltype, sp );
	if(fpl) {
		line->get_schedule()->copy_from(fpl);
	}
	return line;
}


void simlinemgmt_t::get_lines(int type, vector_tpl<linehandle_t>* lines) const
{
	lines->clear();
	for (vector_tpl<linehandle_t>::const_iterator i = all_managed_lines.begin(), end = all_managed_lines.end(); i != end; i++) {
		linehandle_t line = *i;
		if (type == simline_t::line || line->get_linetype() == simline_t::line || line->get_linetype() == type) {
			lines->append(line);
		}
	}
}

void simlinemgmt_t::show_lineinfo(spieler_t *sp, linehandle_t line)
{
	line_management_window(sp);
	schedule_list_gui->show_lineinfo(line);
}
