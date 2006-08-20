/*
 * simroutemgmt.cc
 * part of the Simutrans project
 * @author hsiegeln
 * 01/12/2003
 */

#ifdef _MSC_VER
#include <malloc.h> // for alloca
#endif
#include "simlinemgmt.h"
#include "simline.h"
#include "simhalt.h"
#include "simtypes.h"
#include "simintr.h"

#include "dataobj/fahrplan.h"

uint8 simlinemgmt_t::used_ids[8192];


void simlinemgmt_t::init_line_ids()
{
	memset( used_ids, 8192, 0 );
	used_ids[0] = 1;	// skip line zero
}



int simlinemgmt_t::compare_lines(const void *p1, const void *p2)
{
    linehandle_t line1 =*(const linehandle_t *)p1;
    linehandle_t line2 =*(const linehandle_t *)p2;
    int order;

    /***********************************
    * Compare line 1 and line 2
    ***********************************/
    order = strcmp(line1->get_name(), line2->get_name());
    return order;
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
//DBG_MESSAGE("simlinemgmt_t::add_line()","id=%d",new_line.get_id());
	uint16 id = new_line->get_line_id();
	if(used_ids[id/8]&(1<<(id&7))!=0) {
		dbg->error("simlinemgmt_t::add_line()","Line id %i doubled!",id);
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
		linehandle_t line = all_managed_lines.at(i);
		if (line->get_line_id()==id) {
			return line;
		}
	}
	dbg->warning("simlinemgmt_t::get_line_by_id()","no line for id=%i",id);
	return linehandle_t();
}

/*
 * this function will try to find a line, which's fahrplan matches the passed fahrplan
 */
linehandle_t
simlinemgmt_t::get_line(fahrplan_t *fpl)
{
	int count = count_lines();
	linehandle_t line=linehandle_t();
	for (int i = 0; i<count; i++)
	{
		line = get_line(i);
		if (line->get_fahrplan()->matches(fpl))
			return line;
	}
	return NULL;
}

void
simlinemgmt_t::delete_line(int iroute)
{
	linehandle_t line = get_line(iroute);
	if(line.is_bound()) {
		delete_line(line);
	}
}

void
simlinemgmt_t::delete_line(linehandle_t line)
{
	if (line.is_bound()) {
		all_managed_lines.remove(line);
		//destroy line object
		simline_t *line_ptr=line.detach();
		delete line_ptr;
	}
	//DBG_MESSAGE("simlinemgt_t::delete_line()", "line at index %d (%p) deleted", iroute, line);
}


void
simlinemgmt_t::update_line(linehandle_t line)
{
	// when a line is updated, all managed convoys must get the new fahrplan!
	int count = line->count_convoys();
	convoihandle_t cnv = convoihandle_t();
	for (int i = 0; i<count; i++) {
		cnv = line->get_convoy(i);
		cnv->set_line_update_pending(line->get_line_id());
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
		for (int i = 0; i < count; i++) {
			get_line(i)->rdwr(file);
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
				if (file->get_version() > 84003) {
					simline_t::linetype lt;
					file->rdwr_enum(lt, "\n");
					switch(lt) {
						case simline_t::truckline:
							line = new truckline_t(welt, this, file);
							break;
						case simline_t::trainline:
							line = new trainline_t(welt, this, file);
							break;
						case simline_t::shipline:
							line = new shipline_t(welt, this, file);
							break;
						case simline_t::airline:
							line = new airline_t(welt, this, file);
							break;
						case simline_t::monorailline:
							line = new monorailline_t(welt, this, file);
							break;
						case simline_t::tramline:
							line = new tramline_t(welt, this, file);
							break;
						default:
							line = new simline_t(welt, this, file);
							break;
					}
				}
				else {
					// this else-block is for compatibility with old savegames! (may be not loadable anyway ... )
					line = new simline_t(welt, this, file);
					// identify linetype by checking type of first stop in fahrplan
					fahrplan_t * fpl = line->get_fahrplan();
					fahrplan_t::fahrplan_type type;
					type = fpl->get_type(welt);
					// set linetype and correct fahrplan type
					switch(type) {
						case fahrplan_t::autofahrplan:
							line->set_fahrplan(new autofahrplan_t(fpl));
							line->set_linetype(simline_t::truckline);
							break;
						case fahrplan_t::zugfahrplan:
							line->set_fahrplan(new zugfahrplan_t(fpl));
							line->set_linetype(simline_t::trainline);
							break;
						case fahrplan_t::schifffahrplan:
							line->set_fahrplan(new schifffahrplan_t(fpl));
							line->set_linetype(simline_t::shipline);
							break;
/* not exitent at that time ...
						case fahrplan_t::airfahrplan:
							line->set_fahrplan(new airfahrplan_t(fpl));
							line->set_linetype(simline_t::airline);
							break;
						case fahrplan_t::monorailfahrplan:
							line->set_fahrplan(new monorailfahrplan_t(fpl));
							line->set_linetype(simline_t::monorailline);
							break;
						case fahrplan_t::tramfahrplan:
							line->set_fahrplan(new tramfahrplan_t(fpl));
							line->set_linetype(simline_t::tramline);
							break;
*/
						default:
							line->set_fahrplan(new fahrplan_t(fpl));
							line->set_linetype(simline_t::line);
							break;
					}
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

void
simlinemgmt_t::sort_lines()
{
	int count = count_lines();
#ifdef _MSC_VER
	linehandle_t *a = (line_details_t *)alloca(count * sizeof(line_details_t));
#else
	linehandle_t a[count];
#endif
        int i;
	for (i = 0; i<count; i++) {
		a[i] = get_line(i);
	}
	qsort((void *)a, count, sizeof (linehandle_t), compare_lines);
	all_managed_lines.clear();
	for (i = 0; i<count; i++) {
		all_managed_lines.append(a[i]);
	}
}


void
simlinemgmt_t::register_all_stops()
{
	for (int i = 0; i<count_lines(); i++) {
		all_managed_lines.at(i)->register_stops();
	}
}


void
simlinemgmt_t::laden_abschliessen()
{
	sort_lines();
	register_all_stops();
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
			for(uint16 id=0;  id<7;  id++ ) {
				if((used_ids[i]&(1<<id))==0) {
					used_ids[i] |= (1<<(id&7));
DBG_MESSAGE("simlinemgmt_t::get_unique_line_id()","New id %i",i*8+id);
					return (i*8)+id;
				}
			}
			break;
		}
	}
	// not found
	dbg->error("simlinemgmt_t::get_unique_line_id()","No valid id found!");
	return UNVALID_LINE_ID;
}

void
simlinemgmt_t::new_month()
{
	for (int i = 0; i<count_lines(); i++) {
		all_managed_lines.at(i)->new_month();
	}
}

linehandle_t
simlinemgmt_t::create_line(int ltype)
{
	return create_line(ltype, NULL);
}

linehandle_t
simlinemgmt_t::create_line(int ltype, fahrplan_t * fpl)
{
	simline_t * line = NULL;
	DBG_MESSAGE("simlinemgmt_t::create_line()", "fpl is of type %i", ltype);
	switch (ltype) {
		case simline_t::truckline:
			line = new truckline_t(welt, this, new autofahrplan_t(fpl));
			DBG_MESSAGE("simlinemgmt_t::create_line()", "truckline created");
			break;
		case simline_t::trainline:
			line = new trainline_t(welt, this, new zugfahrplan_t(fpl));
			DBG_MESSAGE("simlinemgmt_t::create_line()", "trainline created");
			break;
		case simline_t::shipline:
			line = new shipline_t(welt, this, new schifffahrplan_t(fpl));
			DBG_MESSAGE("simlinemgmt_t::create_line()", "shipline created");
			break;
		case simline_t::airline:
			line = new airline_t(welt, this, new airfahrplan_t(fpl));
			DBG_MESSAGE("simlinemgmt_t::create_line()", "airline created");
			break;
		case simline_t::monorailline:
			line = new monorailline_t(welt, this, new monorailfahrplan_t(fpl));
			DBG_MESSAGE("simlinemgmt_t::create_line()", "monorailline created");
			break;
		case simline_t::tramline:
			line = new tramline_t(welt, this, new tramfahrplan_t(fpl));
			DBG_MESSAGE("simlinemgmt_t::create_line()", "tramline created");
			break;
		default:
		    line = new simline_t(welt, this, new fahrplan_t(fpl));
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
		linehandle_t line = all_managed_lines.at(i);

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
