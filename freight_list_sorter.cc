
#include "simtypes.h"
#include "utils/cbuffer_t.h"
#include "simware.h"
#include "simhalt.h"
#include "simworld.h"

#include "dataobj/translator.h"

#include "freight_list_sorter.h"


freight_list_sorter_t::sort_mode_t freight_list_sorter_t::sortby=by_name;


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

	int order = 0;
	ware_t ware1 = td1p->ware;
	ware_t ware2 = td2p->ware;

	// first sort according to freight categorie ...
	int index = ware1.gib_catg()-ware2.gib_catg();
	if(index!=0) {
		return index;
	}
	// then according to freight
	index = ware1.gib_typ()->gib_index()-ware2.gib_typ()->gib_index();
	if(index!=0) {
		return index;
	}

	switch (sortby) {
		default:
dbg->error("freight_list_sorter::compare_ware()","illegal sort mode!");
		case by_name: //sort by destination name
			order = strcmp(halt1->gib_name(), halt2->gib_name());
			break;
		case by_via: // sort by via_destination name
			order = strcmp(via_halt1->gib_name(), via_halt2->gib_name());
			// if the destination is different, bit the via_destination the same, sort it by the destination (2nd level sort)
			if (order == 0)	{
				order = strcmp(halt1->gib_name(), halt2->gib_name());
			}
			break;
		case by_via_sum:
		case by_amount: // sort by ware amount
			order = ware2.menge - ware1.menge;
			// if the same amount is transported, we sort by via_destination
			if (order == 0)	{
				order = strcmp(via_halt1->gib_name(), via_halt2->gib_name());
				// if the same amount goes through the same via_destination, sort by destionation
				if (order == 0) {
					order = strcmp(halt1->gib_name(), halt2->gib_name());
				}
			}
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
	if(max) {
		buf.append(sum);
		buf.append("/");
		buf.append(max);
		buf.append(translator::translate(ware->gib_typ()->gib_mass()));
		buf.append(" ");
		if(ware->gib_catg()!=0) {
			buf.append(translator::translate(ware->gib_typ()->gib_catg_name()));
		}
		else {
			// special freight need own name
			buf.append(translator::translate(ware->gib_typ()->gib_name()));
		}
		buf.append(" ");
		buf.append(translator::translate(what_doing));
	}
	else {
		buf.append(sum);
		buf.append(translator::translate(ware->gib_typ()->gib_mass()));
		buf.append(" ");
		buf.append(translator::translate(ware->gib_typ()->gib_name()));
		buf.append(" ");
		buf.append(translator::translate(what_doing));
	}
	buf.append("\n");
}



void
freight_list_sorter_t::sort_freight( karte_t *welt, const slist_tpl <ware_t>*wliste, cbuffer_t &buf, sort_mode_t sort_mode, const slist_tpl <ware_t>*full_list, const char *what_doing )
{
	slist_iterator_tpl<ware_t> iter (wliste);
	sortby = sort_mode;

	slist_tpl <ware_t> dummy;
	slist_iterator_tpl<ware_t> full_iter ( full_list==NULL ? &dummy : full_list );
	bool list_finish=1;

	// hsiegeln
	// added sorting to ware's destination list
	int pos = 0;
#ifdef _MSC_VER
	travel_details *tdlist = (travel_details *)alloca(wliste->count() * sizeof(travel_details *));
#else
	travel_details tdlist [wliste->count()];
#endif

	while(iter.next()) {
		ware_t ware = iter.get_current();
		if(ware.gib_typ()==warenbauer_t::nichts) {
			continue;
		}
//DBG_MESSAGE("freight_list_sorter_t::get_freight_info()","for halt %i",pos);
		tdlist[pos].ware = ware;
		tdlist[pos].destination = haltestelle_t::gib_halt(welt,ware.gib_ziel());
		tdlist[pos].via_destination = haltestelle_t::gib_halt(welt,ware.gib_zwischenziel());
		// for the sorting via the number for the next stop we unify entries
		if(sort_mode==by_via_sum  &&  pos>0) {
//DBG_MESSAGE("freight_list_sorter_t::get_freight_info()","for halt %i check connection",pos);
			// only add it, if there is not another thing waiting with the same via but another destination
			for( int i=0;  i<pos;  i++ ) {
				if(tdlist[i].ware.gib_typ()==tdlist[pos].ware.gib_typ()  &&  tdlist[i].via_destination==tdlist[pos].via_destination  &&  tdlist[i].destination!=tdlist[i].via_destination) {
					tdlist[i].ware.menge += tdlist[pos--].ware.menge;
					break;
				}
			}
		}
//DBG_MESSAGE("freight_list_sorter_t::get_freight_info()","for halt %i added",pos);
		pos++;
	}

	// sort the ware's list
	qsort((void *)tdlist, pos, sizeof (travel_details), compare_ware);

	// print the ware's list to buffer - it should be in sortorder by now!
	int last_ware_index = -1;
	int last_ware_catg = -1;

	for (int j = 0; j<pos; j++) {
		ware_t ware = tdlist[j].ware;

		halthandle_t halt = tdlist[j].destination;
		halthandle_t via_halt = tdlist[j].via_destination;

		const char * name = "Error in Routing";
		if(halt.is_bound()) {
			name = halt->gib_name();
		}

		if(last_ware_index!=ware.gib_typ()->gib_index()  &&  last_ware_catg!=ware.gib_catg()) {
			sint32 sum = 0;
			last_ware_index = ware.gib_typ()->gib_index();
			for(int i=j;  i<pos;  i++  ) {
				ware_t sumware = tdlist[i].ware;
				if(last_ware_index!=sumware.gib_typ()->gib_index()) {
					if(last_ware_catg!=sumware.gib_catg()) {
						break;	// next category reached ...
					}
				}
				sum += sumware.menge;
			}

			// special freight => handle different
			last_ware_catg = (ware.gib_catg()!=0) ? ware.gib_catg() : -1;

			// display all ware
			if(full_list==NULL) {
				add_ware_heading( buf, sum, 0, &ware, what_doing );
			}
			else {
				// ok, we have a list of freights
				while(list_finish  &&  (list_finish=full_iter.next())!=0) {

					ware_t current(full_iter.get_current());
					if(last_ware_index==current.gib_typ()->gib_index()  ||  last_ware_catg==current.gib_catg()) {
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
		buf.append(translator::translate(ware.gib_typ()->gib_mass()));
		buf.append(" ");
		buf.append(translator::translate(ware.gib_typ()->gib_name()));
		buf.append(" > ");
		// the target name is not correct for the via sort
		if(sortby!=by_via_sum  ||  via_halt==halt  ) {
			buf.append(name);
		}

		// for debugging
		const char *via_name = "Error in Routing";
		if(via_halt.is_bound()) {
			via_name = via_halt->gib_name();
		}

		if(via_halt != halt) {
			char tmp [512];
			sprintf(tmp, translator::translate("   via %s\n"), via_name);
			buf.append(tmp);
		}
		else {
			buf.append("\n");
		}
		// debug ende
	}

	// still entires left?
	while(list_finish  &&  full_iter.next()) {
		add_ware_heading( buf, 0, full_iter.get_current().menge, &(full_iter.get_current()), what_doing );
	}
}
