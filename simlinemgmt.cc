/*
 * part of the Simutrans project
 * @author hsiegeln
 * 01/12/2003
 */

#include <algorithm>
#include "simlinemgmt.h"
#include "simline.h"
#include "simhalt.h"
#include "simtypes.h"
#include "simintr.h"

#include "dataobj/fahrplan.h"

uint8 simlinemgmt_t::used_ids[8192];


void simlinemgmt_t::init_line_ids()
{
	DBG_MESSAGE("simlinemgmt_t::init_line_ids()","done");
	for(int i=0;  i<8192;  i++  ) {
		used_ids[i] = 0;
	}
}


simlinemgmt_t::simlinemgmt_t(karte_t * welt,spieler_t *sp) :
	all_managed_lines(0)
{
	this->welt = welt;
	this->besitzer = sp;
}

void
simlinemgmt_t::add_line(linehandle_t new_line)
{
	uint16 id = new_line->get_line_id();
DBG_MESSAGE("simlinemgmt_t::add_line()","id=%d",new_line.get_id());
	if( (used_ids[id/8] & (1<<(id&7)) ) !=0 ) {
		dbg->error("simlinemgmt_t::add_line()","Line id %i doubled! (0x%X)",id,used_ids[id/8]);
		id += 12345+(7*id)&0x0FFF;
		dbg->message("simlinemgmt_t::add_line()","new line id %i!",id);
		new_line->set_line_id( id );
	}
	used_ids[id/8] |= (1<<(id&7));	// should be registered anyway ...
	all_managed_lines.append(new_line,16);
	sort_lines();
}


linehandle_t
simlinemgmt_t::get_line_by_id(uint16 id)
{
	int count = count_lines();
	for(int i = 0; i<count; i++) {
		linehandle_t line = all_managed_lines[i];
		if (line->get_line_id()==id) {
			return line;
		}
	}
	dbg->warning("simlinemgmt_t::get_line_by_id()","no line for id=%i",id);
	return linehandle_t();
}


bool
simlinemgmt_t::delete_line(linehandle_t line)
{
	if (line.is_bound()) {
		all_managed_lines.remove(line);
		//destroy line object
		simline_t *line_ptr=line.detach();
		delete line_ptr;
		return true;
	}
	return false;
	//DBG_MESSAGE("simlinemgt_t::delete_line()", "line at index %d (%p) deleted", iroute, line);
}


void
simlinemgmt_t::update_line(linehandle_t line)
{
	// when a line is updated, all managed convoys must get the new fahrplan!
	int count = line->count_convoys();
	for (int i = 0; i<count; i++) {
		line->get_convoy(i)->set_line_update_pending(line->get_line_id());
	}
	// finally de/register all stops
	line->renew_stops();
}



void
simlinemgmt_t::rdwr(karte_t * welt, loadsave_t *file)
{
	  if(file->is_saving()) {
		file->wr_obj_id("Linemanagement");
		int count = count_lines();
		file->rdwr_long(count, " ");
		for (vector_tpl<linehandle_t>::iterator i = all_managed_lines.begin(), end = all_managed_lines.end(); i != end; i++) {
			(*i)->rdwr(file);
		}
	}
	else {
		char buf[80];
		file->rd_obj_id(buf, 79);
		all_managed_lines.clear();
		if(strcmp(buf, "Linemanagement")==0) {
			int totalLines = 0;
			file->rdwr_long(totalLines, " ");
DBG_MESSAGE("simlinemgmt_t::rdwr()","number of lines=%i",totalLines);
			for (int i = 0; i<totalLines; i++) {
				simline_t * line;
				simline_t::linetype lt;
				file->rdwr_enum(lt, "\n");
				switch(lt) {
					case simline_t::truckline:    line = new truckline_t(   welt, file); break;
					case simline_t::trainline:    line = new trainline_t(   welt, file); break;
					case simline_t::shipline:     line = new shipline_t(    welt, file); break;
					case simline_t::airline:      line = new airline_t(     welt, file); break;
					case simline_t::monorailline: line = new monorailline_t(welt, file); break;
					case simline_t::tramline:     line = new tramline_t(    welt, file); break;
					default:                      line = new simline_t(     welt, file); break;
				}
				add_line(line->self);
			}
		}
	}
}

void
simlinemgmt_t::destroy_all()
{
	all_managed_lines.clear();
}


static bool compare_lines(const linehandle_t& a, const linehandle_t& b)
{
	return strcmp(a->get_name(), b->get_name()) < 0;
}


void simlinemgmt_t::sort_lines()
{
	std::sort(all_managed_lines.begin(), all_managed_lines.end(), compare_lines);
}


void
simlinemgmt_t::register_all_stops()
{
	for (int i = 0; i<count_lines(); i++) {
		all_managed_lines[i]->register_stops();
	}
}


void
simlinemgmt_t::laden_abschliessen()
{
	sort_lines();
	register_all_stops();
	used_ids[0] |= 1;	// assure, that future ids start at 1 ...
}


/**
 * Creates a unique line id. (max uint16, but this should be enough anyway)
 * @author prissi
 */
uint16 simlinemgmt_t::get_unique_line_id()
{
	for(uint16 i=0;  i<8192;  i++  ) {
		if(used_ids[i]!=255) {
DBG_MESSAGE("simlinemgmt_t::get_unique_line_id()","free id near %i",i*8);
			for(uint16 id=0;  id<8;  id++ ) {
				if((used_ids[i]&(1<<id))==0) {
//					used_ids[i] |= (1<<(id&7));
DBG_MESSAGE("simlinemgmt_t::get_unique_line_id()","New id %i",i*8+id);
					return (i*8)+id;
				}
			}
			break;
		}
	}
	// not found
	dbg->error("simlinemgmt_t::get_unique_line_id()","No valid id found!");
	return INVALID_LINE_ID;
}

void
simlinemgmt_t::new_month()
{
	for (int i = 0; i<count_lines(); i++) {
		all_managed_lines[i]->new_month();
	}
}




linehandle_t
simlinemgmt_t::create_line(int ltype, fahrplan_t * fpl)
{
	simline_t * line = NULL;
	DBG_MESSAGE("simlinemgmt_t::create_line()", "fpl is of type %i", ltype);
	switch (ltype) {
		case simline_t::truckline:
			line = new truckline_t(this, new autofahrplan_t(fpl));
			DBG_MESSAGE("simlinemgmt_t::create_line()", "truckline created");
			break;
		case simline_t::trainline:
			line = new trainline_t(this, new zugfahrplan_t(fpl));
			DBG_MESSAGE("simlinemgmt_t::create_line()", "trainline created");
			break;
		case simline_t::shipline:
			line = new shipline_t(this, new schifffahrplan_t(fpl));
			DBG_MESSAGE("simlinemgmt_t::create_line()", "shipline created");
			break;
		case simline_t::airline:
			line = new airline_t(this, new airfahrplan_t(fpl));
			DBG_MESSAGE("simlinemgmt_t::create_line()", "airline created");
			break;
		case simline_t::monorailline:
			line = new monorailline_t(this, new monorailfahrplan_t(fpl));
			DBG_MESSAGE("simlinemgmt_t::create_line()", "monorailline created");
			break;
		case simline_t::tramline:
			line = new tramline_t(this, new tramfahrplan_t(fpl));
			DBG_MESSAGE("simlinemgmt_t::create_line()", "tramline created");
			break;
		default:
		    line = new simline_t(this, new fahrplan_t(fpl));
			DBG_MESSAGE("simlinemgmt_t::create_line()", "default line created");
			break;
	}
	return line->self;
}



/*
 * return a list with all lines of a certain type;
 * type==simline_t::line will return all lines
 */
void
simlinemgmt_t::build_line_list(int type, slist_tpl<linehandle_t> * list)
{
//DBG_MESSAGE("simlinemgmt_t::build_line_list()","type=%i",type);
	list->clear();

	for( int i=0;  i<count_lines();  i++  ) {
		linehandle_t line = all_managed_lines[i];

		if(type==simline_t::line  ||  line->get_linetype()==simline_t::line  ||  line->get_linetype()==type) {
#if 0
			// insert sorted (these lists are not too long, so we will refrain from quicksorts)
			for(unsigned i=0;  i<list->count()  &&  line.is_bound();  i++  ) {
				if(strcmp(line->get_name(),list->at(i)->get_name())<=0) {
DBG_MESSAGE("simlinemgmt_t::build_line_list()","insert id=%i at %i",line.get_id(),i);
					list->insert( line, i );
					line = linehandle_t();
				}
			}
			if(line.is_bound()) {
DBG_MESSAGE("simlinemgmt_t::build_line_list()","append id=%i",line.get_id());
				list->append(line);
			}
#else
			list->append(line);
#endif
		}

	}
}
