/*
 * simroutemgmt.cc
 * part of the Simutrans project
 * @author hsiegeln
 * 01/12/2003
 */

#include "simlinemgmt.h"
#include "simline.h"
#include "simhalt.h"
#include "simtypes.h"
#include "simintr.h"

#include "dataobj/fahrplan.h"


struct line_details_t {
	simline_t * line;
};

slist_tpl<simline_t *> simlinemgmt_t::all_managed_lines;

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

simlinemgmt_t::simlinemgmt_t(karte_t * welt) :
	line_counter(0),
	welt(welt)
{

}

void
simlinemgmt_t::add_line(simline_t * new_line)
{
	all_managed_lines.append(new_line);
	line_counter++;
        sort_lines();
}


simline_t *
simlinemgmt_t::get_line(int iline)
{
	if ((iline > -1) && (iline < count_lines()))
	{
		return all_managed_lines.at(iline);
	} else {
		return NULL;
	}
}

simline_t *
simlinemgmt_t::get_line_by_id(int id)
{
	int count = count_lines();
	for (int i = 0; i<count; i++)
	{
		if (get_line(i)->get_id() == id)
		{
			return get_line(i);
		}
	}
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

int
simlinemgmt_t::count_lines()
{
	return all_managed_lines.count();
}

int
simlinemgmt_t::get_line_counter()
{
	return line_counter;
}

void
simlinemgmt_t::delete_line(int iroute)
{
	simline_t * line = get_line(iroute);
	delete_line(line);
}

void
simlinemgmt_t::delete_line(simline_t * line)
{
	if (line != NULL)
	{
		//destroy line object
		delete (line);
		all_managed_lines.remove(line);
	}
	//dbg->message("simlinemgt_t::delete_line()", "line at index %d (%p) deleted", iroute, line);
}


void
simlinemgmt_t::update_line(simline_t * line)
{
	// when a line is updated, all managed convoys must get the new fahrplan!
	int count = line->count_convoys();
	convoi_t *cnv = NULL;
	for (int i = 0; i<count; i++)
	{
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
		file->rdwr_int(count, " ");
		for (int i = 0; i < count; i++)
		{
			get_line(i)->rdwr(file);
		}
    } else {
		int totalLines = 0;
		file->rdwr_int(totalLines, " ");
		for (int i = 0; i<totalLines; i++)
		{
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
					default:
						line = new simline_t(welt, this, file);
						break;
					}
			} else {
				// this else-block is for compatibility with old savegames!
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

void
simlinemgmt_t::destroy_all()
{
	all_managed_lines.clear();
}

void
simlinemgmt_t::sort_lines()
{
	int count = count_lines();
	line_details_t a[count];
	for (int i = 0; i<count; i++)
	{
		a[i].line = get_line(i);
	}
	qsort((void *)a, count, sizeof (line_details_t), compare_lines);
	all_managed_lines.clear();
	for (int i = 0; i<count; i++)
	{
		all_managed_lines.append(a[i].line);
	}
}


void
simlinemgmt_t::register_all_stops()
{
	for (int i = 0; i<count_lines(); i++)
	{
		get_line(i)->register_stops();
	}
}


void
simlinemgmt_t::laden_abschliessen()
{
	register_all_stops();
}


/**
 * Creates a unique line id.
 * @author Hj. Malthaner
 */
int simlinemgmt_t::get_unique_line_id()
{
  slist_iterator_tpl<simline_t *> iter (all_managed_lines);

  int low = 9999999;
  int high = -1;

  while( iter.next() ) {
    if(iter.get_current()->get_id() < low) {
      low = iter.get_current()->get_id();
    }

    if(iter.get_current()->get_id() > high) {
      high = iter.get_current()->get_id();
    }
  }

  dbg->message("simlinemgmt_t::get_unique_line_id()", "low=%d, high=%d", low, high);

  return high + 1;
}

void
simlinemgmt_t::new_month()
{
	for (int i = 0; i<count_lines(); i++)
	{
		get_line(i)->new_month();
	}
}

simline_t *
simlinemgmt_t::create_line(int ltype)
{
	create_line(ltype, NULL);
}

simline_t *
simlinemgmt_t::create_line(int ltype, fahrplan_t * fpl)
{
	simline_t * line = NULL;
	dbg->message("simlinemgmt_t::create_line()", "fpl is of type %i", ltype);
	switch (ltype) {
		case simline_t::truckline:
			line = new truckline_t(welt, this, new autofahrplan_t(fpl));
			dbg->message("simlinemgmt_t::create_line()", "truckline created");
			break;
		case simline_t::trainline:
			line = new trainline_t(welt, this, new zugfahrplan_t(fpl));
			dbg->message("simlinemgmt_t::create_line()", "trainline created");
			break;
		case simline_t::shipline:
			line = new shipline_t(welt, this, new schifffahrplan_t(fpl));
			dbg->message("simlinemgmt_t::create_line()", "shipline created");
			break;
		default:
		    line = new simline_t(welt, this, new fahrplan_t(fpl));
			dbg->message("simlinemgmt_t::create_line()", "default line created");
			break;
		}
	return line;
}

void
simlinemgmt_t::build_line_list(int type, slist_tpl<simline_t *> * list)
{
  list->clear();
  slist_iterator_tpl<simline_t *> iter (simlinemgmt_t::all_managed_lines);
  while (iter.next()) {
    simline_t * line = iter.get_current();

    // Hajo: 11-Apr-04
    // changed code to show generic lines (type == line)
    // in all depots
    //
    // At some time in future, generic lines should be removed
    // from Simutrans because they are unsafe (no checking if stop
    // is set on correct ground type)

    switch (type) {

    case simline_t::truckline:
      if (line->get_linetype() == simline_t::truckline ||
	  line->get_linetype() == simline_t::line) {
	list->append(line);
      }
      break;
    case simline_t::trainline:
      if (line->get_linetype() == simline_t::trainline ||
	  line->get_linetype() == simline_t::line) {
	list->append(line);
      }
      break;
    case simline_t::shipline:
      if (line->get_linetype() == simline_t::shipline ||
	  line->get_linetype() == simline_t::line) {
	list->append(line);
      }
      break;
    }
  }
}
