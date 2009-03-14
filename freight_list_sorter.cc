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


/** hsiegeln
 *  @param td1, td2: pointer to travel_details
 *  @return sortorder of the two passed elements; used in qsort
 *  @author hsiegeln
 *  @date 2003-11-02
 */
int freight_list_sorter_t::compare_ware(const void *td1, const void *td2)
{
	const travel_details * const td1p = (const travel_details*)td1;
	const travel_details * const td2p = (const travel_details*)td2;

	halthandle_t halt1 = td1p->destination;
	halthandle_t halt2 = td2p->destination;
	halthandle_t via_halt1 = td1p->via_destination;
	halthandle_t via_halt2 = td2p->via_destination;

	if(!halt1.is_bound()  ||  !via_halt1.is_bound()) {
		return -1;
	}

	if( !halt2.is_bound()  ||    !via_halt2.is_bound() ) {
		return -2;
	}

	const ware_t& ware1 = td1p->ware;
	const ware_t& ware2 = td2p->ware;

	// sort according to freight
	int index = ware1.get_besch()->get_index()-ware2.get_besch()->get_index();
	if(index!=0) {
		return index;
	}

	int order;
	switch (sortby) {
		default:
dbg->error("freight_list_sorter::compare_ware()","illegal sort mode!");

		case by_via_sum:
		case by_amount: // sort by ware amount
			order = ware2.menge - ware1.menge;
			if (order != 0) break;
			/* FALLTHROUGH */

		case by_via: // sort by via_destination name
			order = strcmp(via_halt1->get_name(), via_halt2->get_name());
			if (order != 0) break;
			/* FALLTHROUGH */

		case by_name: // sort by destination name
			order = strcmp(halt1->get_name(), halt2->get_name());
			break;
	}

	return order;
}



void
freight_list_sorter_t::add_ware_heading( cbuffer_t &buf, uint32 sum, uint32 max, const ware_t *ware, const char *what_doing )
{
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
		if(sort_mode==by_via_sum  &&  pos>0) {
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
				char tmp [512];
				sprintf(tmp, translator::translate("via %s\n"), via_name);
				buf.append(tmp);
			}
			else {
				buf.append("\n");
			}
			// debug ende
		}
	}

	// still entires left?
	while(list_finish  &&  full_iter.next()) {
		add_ware_heading( buf, 0, full_iter.get_current().menge, &(full_iter.get_current()), what_doing );
	}
}
