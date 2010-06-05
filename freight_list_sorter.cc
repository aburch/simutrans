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

		case by_origin: // Sort by origin name
		case by_origin_amount: {
			halthandle_t const o1 = w1.get_origin();
			halthandle_t const o2 = w2.get_origin();
			if (o1.is_bound() && o2.is_bound()) {
				int const order = strcmp(o1->get_name(), o2->get_name()) < 0;
				if (order != 0) return order < 0;
			} else if (o1.is_bound()) {
				return false;
			} else if (o2.is_bound()) {
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

	bool delete_check = false;
	if(full_list == NULL)
	{
		full_list = new slist_tpl<ware_t>;
		delete_check = true;
	}
	slist_iterator_tpl<ware_t> full_iter ( full_list );
	bool list_finish = true;
	sint16 count = 0;

	// hsiegeln
	// added sorting to ware's destination list
	int pos = 0;
	ALLOCA(ware_t, wlist, warray->get_count());

	for(unsigned n = 0; n < warray->get_count(); n++)
	{
		const ware_t &ware = (*warray)[n];
		if(ware.get_besch() == warenbauer_t::nichts || ware.menge == 0)
		{
			continue;
		}
//DBG_MESSAGE("freight_list_sorter_t::get_freight_info()","for halt %i",pos);
		wlist[pos] = ware;
		// for the sorting via the number for the next stop we unify entries
		if(sort_mode == by_via_sum && pos > 0) 
		{
//DBG_MESSAGE("freight_list_sorter_t::get_freight_info()","for halt %i check connection",pos);
			// only add it, if there is not another thing waiting with the same via but another destination
			for(int i = 0; i < pos; i++) 
			{
				if(wlist[i].get_index() == wlist[pos].get_index() && 
					wlist[i].get_zwischenziel() == wlist[pos].get_zwischenziel()  &&  
					wlist[i].get_ziel() != wlist[pos].get_ziel())
				{
					wlist[i].menge += wlist[pos--].menge;
					break;
				}
				else if(wlist[i].get_index() == wlist[pos].get_index() && 
					wlist[i].get_ziel() == wlist[pos].get_ziel() &&  
					wlist[i].get_ziel() == wlist[i].get_zwischenziel()) 
				{
					wlist[i].menge += wlist[pos--].menge;
					break;
				}
			}
		}
		if(sort_mode == by_origin_amount && pos > 0) 
		{
			for(int i = 0; i < pos; i++) 
			{
				if(wlist[i].get_index() == wlist[pos].get_index() && wlist[i].get_origin() == wlist[pos].get_origin()) 
				{
					wlist[i].menge += wlist[pos--].menge;
					break;
				}
			}
		}

		if((sort_mode == by_name || sort_mode == by_via || sort_mode == by_amount) && pos > 0) 
		{
			for(int i = 0; i < pos; i++) 
			{
				if(wlist[i].get_index() == wlist[pos].get_index() && wlist[i].get_ziel() == wlist[pos].get_ziel()) 
				{
					wlist[i].menge += wlist[pos--].menge;
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
		count = full_list->get_count();

		for (int j = 0; j < pos; j++)
		{
			halthandle_t const halt			= wlist[j].get_ziel();
			halthandle_t const via_halt		= wlist[j].get_zwischenziel();
			halthandle_t const origin_halt	= wlist[j].get_origin();

			const char * name = "unknown";
			if(halt.is_bound()) 
			{
				name = halt->get_name();
			}

			ware_t const& ware = wlist[j];
			if(last_ware_index != ware.get_index() && last_ware_catg != ware.get_catg()) 
			{
				sint32 sum = 0;
				last_ware_index = ware.get_index();
				last_ware_catg = (ware.get_catg()!=0) ? ware.get_catg() : -1;
				for(int i=j;  i<pos;  i++  ) 
				{
					ware_t const& sumware = wlist[i];
					if(last_ware_index!=sumware.get_index()) 
					{
						if(last_ware_catg!=sumware.get_catg()) 
						{
							break;	// next category reached ...
						}
					}
					sum += sumware.menge;
				}

				// special freight => handle different
				last_ware_catg = (ware.get_catg()!=0) ? ware.get_catg() : -1;

				// display all ware
				if(full_list == NULL || full_list->get_count() == 0) 
				{
					add_ware_heading( buf, sum, 0, &ware, what_doing );
				}
				else
				{
					// ok, we have a list of freight
					while(list_finish && (list_finish = full_iter.next()) != 0) 
					{

						const ware_t& current = full_iter.get_current();
						if(last_ware_index == current.get_index() || last_ware_catg==current.get_catg()) 
						{
							add_ware_heading(buf, sum, current.menge, &current, what_doing);
							break;
						}
						else 
						{
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
			if(sortby != by_origin_amount)
			{
				buf.append(" > ");
			}
			else
			{
				buf.append(" < ");
			}
			// the target name is not correct for the via sort
			/*if((sortby != by_via_sum || via_halt == halt) && sortby != by_origin_amount && sortby != by_name && sortby != by_amount)
			{
				buf.append(name);
			}*/

			if(sortby == by_name || sortby == by_amount || sortby == by_origin || (sortby == by_via_sum && via_halt == halt) || sortby == by_via)
			{
				const char *destination_name = "unknown";
				if(halt.is_bound()) 
				{
					destination_name = halt->get_name();
				}
				buf.printf(destination_name);
			}

			if(sortby == by_origin_amount)
			{
				const char *origin_name = "unknown";
				if(origin_halt.is_bound()) 
				{
					origin_name = origin_halt->get_name();
				}
				buf.printf(origin_name);
			}
			
			if(via_halt != halt && (sortby == by_via || sortby == by_via_sum))
			{
				const char *via_name = "unknown";
				if(via_halt.is_bound()) 
				{
					via_name = via_halt->get_name();
				}
				buf.printf(translator::translate(" via %s"), via_name);
			}
			
			if(sortby == by_origin)
			{
				const char *origin_name = "unknown";
				if(origin_halt.is_bound()) 
				{
					origin_name = origin_halt->get_name();
				}

				buf.printf(translator::translate(" from %s"), origin_name);
			}

			buf.append("\n");
			// debug ende

			// buffer full, no need to proceed
			if (buf.is_full()) {
				break;
			}
		}
	}

	// still entires left?
	while(list_finish  &&  full_iter.next()) 
	{
		add_ware_heading( buf, 0, full_iter.get_current().menge, &(full_iter.get_current()), what_doing );
	}
	if(delete_check)
	{
		delete full_list;
	}
}
