#include <algorithm>

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


/**
 *  @return whether w1 is less than w2
 */
bool freight_list_sorter_t::compare_ware(ware_t const& w1, ware_t const& w2)
{
	// sort according to freight
	int const idx = w1.get_besch()->get_index() - w2.get_besch()->get_index();
	if (idx != 0) {
		return idx < 0;
	}

	switch (sortby) {
		default:
			dbg->error("freight_list_sorter::compare_ware()", "illegal sort mode!");

		case by_via_sum:
		case by_amount: { // sort by ware amount
			int const order = w2.menge - w1.menge;
			if (order != 0) return order < 0;
			/* FALLTHROUGH */
		}

		case by_via: { // sort by via_destination name
			halthandle_t const v1 = w1.get_zwischenziel();
			halthandle_t const v2 = w2.get_zwischenziel();
			if (v1.is_bound() && v2.is_bound()) {
				int const order = strcmp(v1->get_name(), v2->get_name());
				if (order != 0) return order < 0;
			} else if (v1.is_bound()) {
				return false;
			} else if (v2.is_bound()) {
				return true;
			}
			/* FALLTHROUGH */
		}

		case by_name: { // sort by destination name
			halthandle_t const d1 = w1.get_ziel();
			halthandle_t const d2 = w2.get_ziel();
			if (d1.is_bound() && d2.is_bound()) {
				return strcmp(d1->get_name(), d2->get_name()) < 0;
			} else if (d1.is_bound()) {
				return false;
			} else if (d2.is_bound()) {
				return true;
			} else {
				return false;
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
	buf.printf(" %u", sum);
	if (max != 0) {
		// convois
		buf.printf("/%u", max);
	}
	ware_besch_t const& desc = *ware->get_besch();
	char const*  const  unit = translator::translate(desc.get_mass());
	// special freight (catg == 0) needs own name
	char const*  const  name = translator::translate(ware->get_catg() != 0 ? desc.get_catg_name() : desc.get_name());
	char const*  const  what = translator::translate(what_doing);
	buf.printf("%s %s %s\n", unit, name, what);
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
	ALLOCA(ware_t, wlist, warray->get_count());

	for(unsigned i=0;  i<warray->get_count();  i++  ) {
		const ware_t &ware = (*warray)[i];
		if(ware.get_besch()==warenbauer_t::nichts  ||  ware.menge==0) {
			continue;
		}
//DBG_MESSAGE("freight_list_sorter_t::get_freight_info()","for halt %i",pos);
		wlist[pos] = ware;
		// for the sorting via the number for the next stop we unify entries
		if (sort_mode == by_via_sum) {
//DBG_MESSAGE("freight_list_sorter_t::get_freight_info()","for halt %i check connection",pos);
			// only add it, if there is not another thing waiting with the same via but another destination
			for( int i=0;  i<pos;  i++ ) {
				ware_t& wi = wlist[i];
				if (wi.get_index()        == ware.get_index()        &&
						wi.get_zwischenziel() == ware.get_zwischenziel() &&
						wi.get_ziel()         != wi.get_zwischenziel()) {
					wi.menge += ware.menge;
					--pos;
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
		std::sort(wlist, wlist + pos, compare_ware);

		// print the ware's list to buffer - it should be in sortorder by now!
		int last_ware_index = -1;
		int last_ware_catg = -1;

		for (int j = 0; j<pos; j++) {
			halthandle_t const halt     = wlist[j].get_ziel();
			halthandle_t const via_halt = wlist[j].get_zwischenziel();

			const char * name = "Error in Routing";
			if(halt.is_bound()) {
				name = halt->get_name();
			}

			ware_t const& ware = wlist[j];
			if(last_ware_index!=ware.get_index()  &&  last_ware_catg!=ware.get_catg()) {
				sint32 sum = 0;
				last_ware_index = ware.get_index();
				last_ware_catg = (ware.get_catg()!=0) ? ware.get_catg() : -1;
				for(int i=j;  i<pos;  i++  ) {
					ware_t const& sumware = wlist[i];
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
			ware_besch_t const& desc = *ware.get_besch();
			buf.printf("   %u%s %s > ", ware.menge, translator::translate(desc.get_mass()), translator::translate(desc.get_name()));
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
