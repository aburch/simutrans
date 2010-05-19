#include "freight_list_sorter.h"
#include "simhalt.h"
#include "simtypes.h"
#include "simware.h"
#include "simworld.h"

#include "dataobj/translator.h"

#include "tpl/slist_tpl.h"
#include "tpl/vector_tpl.h"

#include "utils/cbuffer_t.h"

freight_list_sorter_t::sort_mode_t freight_list_sorter_t::sortby=by_name;

/*
 * struct hold travel details for wares that travel
 * @author hsiegeln
 */
struct travel_details
{
	ware_t ware;
	halthandle_t destination;
	halthandle_t via_destination;
};


/**
 *  @param td1p, td2p: pointer to travel_details
 *  @return sort order of the two passed elements; used in qsort
 *  @author hsiegeln
 *  @date 2003-11-02
 */
int freight_list_sorter_t::compare_ware(void const* const td1p, void const* const td2p)
{
	travel_details const& td1 = *static_cast<travel_details const*>(td1p);
	travel_details const& td2 = *static_cast<travel_details const*>(td2p);

	// sort according to freight
	int const idx = td1.ware.get_besch()->get_index() - td2.ware.get_besch()->get_index();
	if (idx != 0) {
		return idx;
	}

	switch (sortby) {
		default:
			dbg->error("freight_list_sorter::compare_ware()", "illegal sort mode!");

		case by_via_sum:
		case by_amount: { // sort by ware amount
			int const order = td2.ware.menge - td1.ware.menge;
			if (order != 0) return order;
			/* FALLTHROUGH */
		}

		case by_via: { // sort by via_destination name
			halthandle_t const v1 = td1.via_destination;
			halthandle_t const v2 = td2.via_destination;
			if (v1.is_bound() && v2.is_bound()) {
				int const order = strcmp(v1->get_name(), v2->get_name());
				if (order != 0) return order;
			} else if (v1.is_bound()) {
				return 1;
			} else if (v2.is_bound()) {
				return -1;
			}
			/* FALLTHROUGH */
		}

		case by_name: { // sort by destination name
			halthandle_t const d1 = td1.destination;
			halthandle_t const d2 = td2.destination;
			if (d1.is_bound() && d2.is_bound()) {
				return strcmp(d1->get_name(), d2->get_name());
			} else if (d1.is_bound()) {
				return 1;
			} else if (d2.is_bound()) {
				return -1;
			} else {
				return 0;
			}
		}
	}
}



void
freight_list_sorter_t::add_ware_heading( cbuffer_t &buf, uint32 sum, uint32 max, const ware_t *ware, const char *what_doing )
{
	// buffer full, no need to proceed
	if (buf.is_full()) {
		return;
	}
	// not the first line?
	if(buf.len()>0) {
		buf.append("\n");
	}
	buf.append(" ");
	buf.append(sum);
	if(max) {
		// convois
		buf.append("/");
		buf.append(max);
	}
	buf.append(translator::translate(ware->get_besch()->get_mass()));
	buf.append(" ");
	// special freight (catg==0) need own name
	buf.append( translator::translate( ware->get_catg()!=0 ? ware->get_besch()->get_catg_name() : ware->get_besch()->get_name() ));
	buf.append(" ");
	buf.append(translator::translate(what_doing));
	buf.append("\n");
}


void freight_list_sorter_t::sort_freight(const vector_tpl<ware_t>* warray, cbuffer_t& buf, sort_mode_t sort_mode, const slist_tpl<ware_t>* full_list, const char* what_doing)
{
	sortby = sort_mode;

	// if there, give the capacity for each freight
	slist_tpl <ware_t> dummy;
	slist_iterator_tpl<ware_t> full_iter ( full_list==NULL ? &dummy : full_list );
	bool list_finish=1;

	// hsiegeln
	// added sorting to ware's destination list
	int pos = 0;
	ALLOCA(travel_details, tdlist, warray->get_count());

	for(unsigned i=0;  i<warray->get_count();  i++  ) {
		const ware_t &ware = (*warray)[i];
		if(ware.get_besch()==warenbauer_t::nichts  ||  ware.menge==0) {
			continue;
		}
//DBG_MESSAGE("freight_list_sorter_t::get_freight_info()","for halt %i",pos);
		tdlist[pos].ware = ware;
		tdlist[pos].destination = ware.get_ziel();
		tdlist[pos].via_destination = ware.get_zwischenziel();
		// for the sorting via the number for the next stop we unify entries
		if (sort_mode == by_via_sum) {
//DBG_MESSAGE("freight_list_sorter_t::get_freight_info()","for halt %i check connection",pos);
			// only add it, if there is not another thing waiting with the same via but another destination
			for( int i=0;  i<pos;  i++ ) {
				if(tdlist[i].ware.get_index()==tdlist[pos].ware.get_index()  &&  tdlist[i].via_destination==tdlist[pos].via_destination  &&  tdlist[i].destination!=tdlist[i].via_destination) {
					tdlist[i].ware.menge += tdlist[pos--].ware.menge;
					break;
				}
			}
		}
//DBG_MESSAGE("freight_list_sorter_t::get_freight_info()","for halt %i added",pos);
		pos++;
	}

	// at least some capacity added?
	if(pos!=0) {

		// sort the ware's list
		qsort((void *)tdlist, pos, sizeof (travel_details), compare_ware);

		// print the ware's list to buffer - it should be in sortorder by now!
		int last_ware_index = -1;
		int last_ware_catg = -1;

		for (int j = 0; j<pos; j++) {
			halthandle_t halt = tdlist[j].destination;
			halthandle_t via_halt = tdlist[j].via_destination;

			const char * name = "Error in Routing";
			if(halt.is_bound()) {
				name = halt->get_name();
			}

			const ware_t& ware = tdlist[j].ware;
			if(last_ware_index!=ware.get_index()  &&  last_ware_catg!=ware.get_catg()) {
				sint32 sum = 0;
				last_ware_index = ware.get_index();
				last_ware_catg = (ware.get_catg()!=0) ? ware.get_catg() : -1;
				for(int i=j;  i<pos;  i++  ) {
					const ware_t& sumware = tdlist[i].ware;
					if(last_ware_index!=sumware.get_index()) {
						if(last_ware_catg!=sumware.get_catg()) {
							break;	// next category reached ...
						}
					}
					sum += sumware.menge;
				}

				// special freight => handle different
				last_ware_catg = (ware.get_catg()!=0) ? ware.get_catg() : -1;

				// display all ware
				if(full_list==NULL) {
					add_ware_heading( buf, sum, 0, &ware, what_doing );
				}
				else {
					// ok, we have a list of freights
					while(list_finish  &&  (list_finish=full_iter.next())!=0) {

						const ware_t& current = full_iter.get_current();
						if(last_ware_index==current.get_index()  ||  last_ware_catg==current.get_catg()) {
							add_ware_heading( buf, sum, current.menge, &current, what_doing );
							break;
						}
						else {
							add_ware_heading( buf, 0, current.menge, &current, what_doing );
						}
					}
				}
			}
			// detail amount
			buf.append("   ");
			buf.append(ware.menge);
			buf.append(translator::translate(ware.get_besch()->get_mass()));
			buf.append(" ");
			buf.append(translator::translate(ware.get_besch()->get_name()));
			buf.append(" > ");
			// the target name is not correct for the via sort
			if(sortby!=by_via_sum  ||  via_halt==halt  ) {
				buf.append(name);
			}

			// for debugging
			const char *via_name = "Error in Routing";
			if(via_halt.is_bound()) {
				via_name = via_halt->get_name();
			}

			if(via_halt != halt) {
				buf.printf(translator::translate("via %s\n"), via_name);
			}
			else {
				buf.append("\n");
			}
			// debug ende

			// buffer full, no need to proceed
			if (buf.is_full()) {
				break;
			}
		}
	}

	// still entires left?
	while(list_finish  &&  full_iter.next()) {
		add_ware_heading( buf, 0, full_iter.get_current().menge, &(full_iter.get_current()), what_doing );
	}
}
