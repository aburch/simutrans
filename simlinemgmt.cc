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


struct line_details_t {
	simline_t * line;
};


void simlinemgmt_t::init_line_ids()
{
	memset( used_ids, 8192, 0 );
}



int simlinemgmt_t::compare_lines(const void *p1, const void *p2)
{
    line_details_t line1 =*(const line_details_t *)p1;
    line_details_t line2 =*(const line_details_t *)p2;
    int order;

    /***********************************
    * Compare line 1 and line 2
    ***********************************/
    order = strcmp(line1.line->get_name(), line2.line->get_name());
    return order;
}

simlinemgmt_t::simlinemgmt_t(karte_t * welt,spieler_t *sp) :
	all_managed_lines(0)
{
	this->welt = welt;
	this->besitzer = sp;
}

void
simlinemgmt_t::add_line(simline_t * new_line)
{
	const uint16 id = new_line->get_id();
	if(all_managed_lines.is_contained(new_line)) {
		trap();
	}
	all_managed_lines.append_unique(new_line,16);
	used_ids[id/8] |= 1<<(id&7);	// should be registered anyway ...
	sort_lines();
}


simline_t *
simlinemgmt_t::get_line_by_id(uint16 id)
{
	int count = count_lines();
	for(int i = 0; i<count; i++) {
		simline_t *line = all_managed_lines.at(i);
		if (line->get_id()==id) {
			return line;
		}
	}
	dbg->warning("simlinemgmt_t::get_line_by_id()","no line for id=%i",id);
	return NULL;
}

/*
 * this function will try to find a line, which's fahrplan matches the passed fahrplan
 */
simline_t *
simlinemgmt_t::get_line(fahrplan_t *fpl)
{
	int count = count_lines();
	simline_t * line = NULL;
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
	simline_t * line = get_line(iroute);
	if(line) {
		delete_line(line);
	}
}

void
simlinemgmt_t::delete_line(simline_t * line)
{
	if (line != NULL) {
		all_managed_lines.remove(line);
		//destroy line object
		delete (line);
	}
	//DBG_MESSAGE("simlinemgt_t::delete_line()", "line at index %d (%p) deleted", iroute, line);
}


void
simlinemgmt_t::update_line(simline_t * line)
{
	// when a line is updated, all managed convoys must get the new fahrplan!
	int count = line->count_convoys();
	convoi_t *cnv = NULL;
	for (int i = 0; i<count; i++) {
		cnv = line->get_convoy(i);
		cnv->set_line_update_pending(line->get_id());
	}
	// finally de/register all stops
	line->renew_stops();
}


void
simlinemgmt_t::update_line(int iroute)
{
	simline_t * updated_line = get_line(iroute);
	update_line(updated_line);
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
*/
						default:
							line->set_fahrplan(new fahrplan_t(fpl));
							line->set_linetype(simline_t::line);
							break;
					}
				}
				add_line(line);
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
	line_details_t *a = (line_details_t *)alloca(count * sizeof(line_details_t));
#else
	line_details_t a[count];
#endif
        int i;
	for (i = 0; i<count; i++) {
		a[i].line = get_line(i);
	}
	qsort((void *)a, count, sizeof (line_details_t), compare_lines);
	all_managed_lines.clear();
	for (i = 0; i<count; i++) {
		all_managed_lines.append(a[i].line);
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
	register_all_stops();
}


/**
 * Creates a unique line id. (max uint16, but this should be enough anyway)
 * @author prissi
 */
uint16 simlinemgmt_t::get_unique_line_id()
{
	for(uint16 i=1;  i<65530;  i++  ) {
		if( (used_ids[i/8]&(1<<(i&7)))==0 ) {
			used_ids[i/8] |= 1<<(i&7);
DBG_MESSAGE("simlinemgmt_t::get_unique_line_id()","New id %i",i);
			return i;
		}
	}
	dbg->warning("simlinemgmt_t::get_unique_line_id()","No valid id found!");
	// not found
	return UNVALID_LINE_ID;
}

void
simlinemgmt_t::new_month()
{
	for (int i = 0; i<count_lines(); i++) {
		all_managed_lines.at(i)->new_month();
	}
}

simline_t *
simlinemgmt_t::create_line(int ltype)
{
	return create_line(ltype, NULL);
}

simline_t *
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
		default:
		    line = new simline_t(welt, this, new fahrplan_t(fpl));
			DBG_MESSAGE("simlinemgmt_t::create_line()", "default line created");
			break;
		}
	return line;
}

void
simlinemgmt_t::build_line_list(int type, slist_tpl<simline_t *> * list)
{
//DBG_MESSAGE("simlinemgmt_t::build_line_list()","type=%i",type);
	list->clear();
	for( unsigned i=0;  i<count_lines();  i++  ) {
		simline_t * line = all_managed_lines.at(i);

		// Hajo: 11-Apr-04
		// changed code to show generic lines (type == line)
		// in all depots
		//
		// At some time in future, generic lines should be removed
		// from Simutrans because they are unsafe (no checking if stop
		// is set on correct ground type)

		switch (type) {

			case simline_t::line:
				// all lines
				list->append(line);
				break;
			case simline_t::truckline:
				if (line->get_linetype() == simline_t::truckline || line->get_linetype() == simline_t::line) {
					list->append(line);
				}
				break;
			case simline_t::trainline:
				if (line->get_linetype() == simline_t::trainline || line->get_linetype() == simline_t::line) {
					list->append(line);
				}
				break;
			case simline_t::shipline:
				if (line->get_linetype() == simline_t::shipline || line->get_linetype() == simline_t::line) {
					list->append(line);
				}
				break;
			case simline_t::airline:
				if (line->get_linetype() == simline_t::airline || line->get_linetype() == simline_t::line) {
					list->append(line);
				}
				break;
			case simline_t::monorailline:
				if (line->get_linetype() == simline_t::monorailline || line->get_linetype() == simline_t::line) {
					list->append(line);
				}
				break;
		}
	}
}
