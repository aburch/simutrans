/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "citylist_stats_t.h"
#include "city_info.h"

#include "../simcity.h"
#include "../simevent.h"
#include "../simworld.h"

#include "../utils/cbuffer_t.h"
#include "components/gui_label.h"

bool citylist_stats_t::sortreverse = false;
bool citylist_stats_t::filter_own_network = false;
uint8 citylist_stats_t::sort_mode = citylist_stats_t::SORT_BY_NAME;
uint8 citylist_stats_t::region_filter = 0;

uint16 citylist_stats_t::name_width = 100;

static karte_ptr_t welt;

citylist_stats_t::citylist_stats_t(stadt_t *c)
{
	city = c;
	set_table_layout(9, 0);

	button_t *b = new_component<button_t>(); // (1,0)
	b->set_typ(button_t::posbutton_automatic);
	b->set_targetpos(city->get_center());

	electricity.set_image(skinverwaltung_t::electricity->get_image_id(0), true);
	electricity.set_rigid(true);
	add_component(&electricity); // (2,0)

	lb_name.set_fixed_width( name_width );
	add_component(&lb_name); // (3,0)

	add_component(&label); // (4,0)

	if (skinverwaltung_t::alerts) {
		alert.set_image(skinverwaltung_t::alerts->get_image_id(2), true);
		alert.set_tooltip("City growth is restrained");
	}
	else {
		alert.set_image(IMG_EMPTY);
	}
	alert.set_rigid(true);
	add_component(&alert); // (5,0)

	fluctuation_city.set_show_border_value(false);
	add_component(&fluctuation_city); // (6,0)

	add_component(&lb_region);

	update_label();

	new_component<gui_fill_t>();
}

void citylist_stats_t::update_label()
{
	electricity.set_visible(city->get_finance_history_month(0, HIST_POWER_RECEIVED));
	if ( (city->get_finance_history_month(0, HIST_POWER_RECEIVED) * 9) < (welt->get_finance_history_month(0, HIST_POWER_NEEDED) / 10) ) {
		electricity.set_transparent(TRANSPARENT25_FLAG);
	}

	const scr_coord_val temp_w = proportional_string_width( city->get_name() );
	if (temp_w > name_width) {
		name_width = temp_w;
	}
	lb_name.buf().printf("%s ", city->get_name());
	lb_name.update();

	label.buf().printf("%8d ", city->get_finance_history_month(0, HIST_CITICENS));
	label.update();
	//label.set_align(gui_label_t::left);

	lb_region.buf().clear();
	if (!welt->get_settings().regions.empty()) {
		lb_region.buf().printf("(%s)", translator::translate(welt->get_region_name(city->get_pos()).c_str()));
	}
	lb_region.update();

	const bool allow_citygrowth = city->get_citygrowth();
	if (!allow_citygrowth) {
		fluctuation_city.set_color(COL_BLUE);
	}
	fluctuation_city.set_value(city->get_finance_history_month(0, HIST_GROWTH));
	alert.set_visible(!allow_citygrowth);

	set_size(size);
}


void citylist_stats_t::set_size(scr_size size)
{
	gui_aligned_container_t::set_size(size);
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

	if (!swallowed  &&  IS_LEFTRELEASE(ev)) {
		//city->open_info_window();
		city->show_info();
		swallowed = true;
	}
	return swallowed;
}


bool citylist_stats_t::compare(const gui_component_t *aa, const gui_component_t *bb)
{
	const citylist_stats_t* a = dynamic_cast<const citylist_stats_t*>(aa);
	const citylist_stats_t* b = dynamic_cast<const citylist_stats_t*>(bb);
	// good luck with mixed lists
	assert(a != NULL && b != NULL);

	if (sortreverse) {
		const citylist_stats_t *temp = a;
		a = b;
		b = temp;
	}
	if (  sort_mode != sort_mode_t::SORT_BY_NAME  ) {
		switch (  sort_mode  ) {
		case SORT_BY_NAME:	// default
			break;
		case SORT_BY_SIZE:
			return a->city->get_city_population() < b->city->get_city_population();
		case SORT_BY_GROWTH:
			return a->city->get_wachstum() < b->city->get_wachstum();
		case SORT_BY_REGION:
			return welt->get_region(a->city->get_pos()) < welt->get_region(b->city->get_pos());
		default: break;
		}
		// default sorting ...
	}
	// first: try to sort by number
	const char *atxt = a->get_text();
	int aint = 0;
	// isdigit produces with UTF8 assertions ...
	if (atxt[0] >= '0'  &&  atxt[0] <= '9') {
		aint = atoi(atxt);
	}
	else if (atxt[0] == '('  &&  atxt[1] >= '0'  &&  atxt[1] <= '9') {
		aint = atoi(atxt + 1);
	}
	const char *btxt = b->get_text();
	int bint = 0;
	if (btxt[0] >= '0'  &&  btxt[0] <= '9') {
		bint = atoi(btxt);
	}
	else if (btxt[0] == '('  &&  btxt[1] >= '0'  &&  btxt[1] <= '9') {
		bint = atoi(btxt + 1);
	}
	if (aint != bint) {
		return (aint - bint) < 0;
	}
	// otherwise: sort by name
	return strcmp(atxt, btxt) < 0;
}
