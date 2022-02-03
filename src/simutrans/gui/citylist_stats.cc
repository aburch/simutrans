/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "citylist_stats.h"
#include "city_info.h"

#include "../world/simcity.h"
#include "../simevent.h"
#include "../world/simworld.h"

#include "../utils/cbuffer.h"


citylist_stats_t::citylist_stats_t(stadt_t *c)
{
	city = c;
	set_table_layout(3,0);

	button_t *b = new_component<button_t>();
	b->set_typ(button_t::posbutton_automatic);
	b->set_targetpos(city->get_center());

	add_component(&label);
	update_label();

	new_component<gui_fill_t>();
}


void citylist_stats_t::update_label()
{
	cbuffer_t &buf = label.buf();
	buf.printf( "%s: ", city->get_name() );
	buf.append( city->get_einwohner(), 0 );
	buf.append( " (" );
	buf.append( city->get_wachstum()/10.0, 1 );
	buf.append( ")" );
	label.update();
}


void citylist_stats_t::set_size(scr_size size)
{
	gui_aligned_container_t::set_size(size);
	label.set_size(scr_size(get_size().w - label.get_pos().x, label.get_size().h));
}


void citylist_stats_t::draw( scr_coord pos)
{
	update_label();
	gui_aligned_container_t::draw(pos);
}


bool citylist_stats_t::is_valid() const
{
	return world()->get_cities().is_contained(city);
}


bool citylist_stats_t::infowin_event(const event_t *ev)
{
	bool swallowed = gui_aligned_container_t::infowin_event(ev);

	if(  !swallowed  &&  IS_LEFTRELEASE(ev)  ) {
		city->open_info_window();
		swallowed = true;
	}
	return swallowed;
}



citylist_stats_t::sort_mode_t citylist_stats_t::sort_mode = citylist_stats_t::SORT_BY_NAME;
uint8 citylist_stats_t::player_nr = -1;

bool citylist_stats_t::compare(const gui_component_t *aa, const gui_component_t *bb)
{
	bool reverse = citylist_stats_t::sort_mode > citylist_stats_t::SORT_MODES;
	int sort_mode = citylist_stats_t::sort_mode & 0x1F;

	const citylist_stats_t* a = dynamic_cast<const citylist_stats_t*>(aa);
	const citylist_stats_t* b = dynamic_cast<const citylist_stats_t*>(bb);
	// good luck with mixed lists
	assert(a != NULL  &&  b != NULL);

	if(  reverse  ) {
		std::swap(a,b);
	}

	if(  sort_mode != SORT_BY_NAME  ) {
		switch(  sort_mode  ) {
			case SORT_BY_NAME: // default
				break;
			case SORT_BY_SIZE:
				return a->city->get_einwohner() < b->city->get_einwohner();
			case SORT_BY_GROWTH:
				return a->city->get_wachstum() < b->city->get_wachstum();
			default: break;
		}
		// default sorting ...
	}

	// first: try to sort by number
	const char *atxt =a->get_text();
	int aint = 0;
	// isdigit produces with UTF8 assertions ...
	if(  atxt[0]>='0'  &&  atxt[0]<='9'  ) {
		aint = atoi( atxt );
	}
	else if(  atxt[0]=='('  &&  atxt[1]>='0'  &&  atxt[1]<='9'  ) {
		aint = atoi( atxt+1 );
	}
	const char *btxt = b->get_text();
	int bint = 0;
	if(  btxt[0]>='0'  &&  btxt[0]<='9'  ) {
		bint = atoi( btxt );
	}
	else if(  btxt[0]=='('  &&  btxt[1]>='0'  &&  btxt[1]<='9'  ) {
		bint = atoi( btxt+1 );
	}
	if(  aint!=bint  ) {
		return (aint-bint)<0;
	}
	// otherwise: sort by name
	return strcmp(atxt, btxt)<0;
}
