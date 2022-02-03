/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <algorithm>
#include <limits>

#include "goods_frame.h"
#include "components/gui_scrollpane.h"

#include "../builder/goods_manager.h"
#include "../builder/vehikelbauer.h"
#include "../descriptor/goods_desc.h"
#include "../dataobj/translator.h"

#include "../simcolor.h"
#include "simwin.h"
#include "../world/simworld.h"

sint32 goods_frame_t::selected_speed = 1;

bool goods_frame_t::average_selection = true;

goods_frame_t::sort_mode_t goods_frame_t::sortby = unsortiert;

bool goods_frame_t::sortreverse = false;

const char *goods_frame_t::sort_text[SORT_MODES] = {
	"gl_btn_unsort",
	"gl_btn_sort_name",
	"gl_btn_sort_revenue",
	"gl_btn_sort_bonus",
	"gl_btn_sort_catg"
};

bool goods_frame_t::filter_goods = false;

simline_t::linetype goods_frame_t::last_scheduletype = simline_t::trainline;

goods_frame_t::goods_frame_t() :
	gui_frame_t( translator::translate("gl_title") ),
	speed_text( &speed_message),
	goods_stats(),
	scrolly(&goods_stats)
{
	set_table_layout(1,0);

	// top line: combo, input, text
	add_table(3,1);
	{
		build_linetype_list(filter_goods);
		add_component(&scheduletype);
		scheduletype.add_listener( this );

		speed.init(1, 1, 1, gui_numberinput_t::AUTOLINEAR, false);
		if (average_selection) {
			selected_speed = welt->get_average_speed(simline_t::linetype_to_waytype(last_scheduletype));
		}

		set_linetype(last_scheduletype, selected_speed);
		add_component(&speed);
		speed.add_listener( this );

		gui_label_buf_t *lb = new_component<gui_label_buf_t>(SYSCOL_TEXT_HIGHLIGHT);
		lb->buf().printf(translator::translate("100 km/h = %i tiles/month"), welt->speed_to_tiles_per_month(kmh_to_speed(100)) );
		lb->update();
	}
	end_table();

	add_component(&speed_text);

	filter_goods_toggle.init(button_t::square_state, "Show only used");
	filter_goods_toggle.set_tooltip(translator::translate("Only show goods which are currently handled by factories"));
	filter_goods_toggle.add_listener(this);
	filter_goods_toggle.pressed = filter_goods;
	add_component(&filter_goods_toggle);

	// sort mode
	sort_row = add_table(3,1);
	{
		new_component<gui_label_t>("hl_txt_sort");

		sortedby.set_unsorted(); // do not sort
		for (size_t i = 0; i < lengthof(sort_text); i++) {
			sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
		}
		sortedby.set_selection(sortby);
		sortedby.add_listener(this);
		add_component(&sortedby);

		sorteddir.init(button_t::sortarrow_state, "");
		sorteddir.pressed = sortreverse;
		sorteddir.add_listener(this);
		add_component(&sorteddir);
	}
	end_table();

	add_component(&scrolly);
	scrolly.set_maximize(true);

	sort_list();

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
	set_resizemode(diagonal_resize);
}

void goods_frame_t::build_linetype_list(bool const show_used)
{
	// build combobox list
	scheduletype.enable();
	scheduletype.clear_elements();
	size_t item_count = 0;
	for (int i = 1; i < simline_t::MAX_LINE_TYPE; i++) {
		simline_t::linetype const linetype = (simline_t::linetype)i;
		waytype_t const waytype = simline_t::linetype_to_waytype(linetype);
		sint32 const maximum_speed = vehicle_builder_t::get_fastest_vehicle_speed(waytype, welt->get_current_month(),
			welt->get_settings().get_use_timeline() && show_used, welt->get_settings().get_allow_buying_obsolete_vehicles());

		if (!show_used || maximum_speed > 0) {
			scheduletype.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(simline_t::get_linetype_name(linetype), SYSCOL_TEXT);
			linetype_selection_map[item_count] = linetype;

			item_count+= 1;
		}
	}

	if (item_count == 0) {
		// no vehicles found, eg due to unsupported date or incomplete pakset
		if (show_used) {
			build_linetype_list(false);
		}

		scheduletype.disable();
	}

	// select schedule type
	bool selected = false;
	for (size_t index = 0 ; index < item_count ; index++) {
		if (linetype_selection_map[index] == last_scheduletype) {
			scheduletype.set_selection((int)index);
			selected = true;
			break;
		}
	}
	if (!selected && item_count > 0) {
		scheduletype.set_selection(0);
		set_linetype(linetype_selection_map[0]);
	}
}

bool goods_frame_t::compare_goods(goods_desc_t const* const w1, goods_desc_t const* const w2)
{
	int order = 0;

	switch (sortby) {
		case 0: // sort by number
			order = w1->get_index() - w2->get_index();
			break;
		case 2: // sort by revenue
			{
			    const sint16 relative_speed_change = (100*selected_speed)/welt->get_average_speed(simline_t::linetype_to_waytype(last_scheduletype));
				const sint32 grundwert1281 = w1->get_value() * goods_frame_t::welt->get_settings().get_bonus_basefactor();
				const sint32 grundwert_bonus1 = w1->get_value()*(1000l+(relative_speed_change-100l)*w1->get_speed_bonus());
				const sint32 price1 = (grundwert1281>grundwert_bonus1 ? grundwert1281 : grundwert_bonus1);
				const sint32 grundwert1282 = w2->get_value() * goods_frame_t::welt->get_settings().get_bonus_basefactor();
				const sint32 grundwert_bonus2 = w2->get_value()*(1000l+(relative_speed_change-100l)*w2->get_speed_bonus());
				const sint32 price2 = (grundwert1282>grundwert_bonus2 ? grundwert1282 : grundwert_bonus2);
				order = price1-price2;
			}
			break;
		case 3: // sort by speed bonus
			order = w1->get_speed_bonus()-w2->get_speed_bonus();
			break;
		case 4: // sort by catg_index
			order = w1->get_catg()-w2->get_catg();
			break;
		default: ; // make compiler happy, order will be determined below anyway
	}
	if(  order==0  ) {
		// sort by name if not sorted or not unique
		order = strcmp(translator::translate(w1->get_name()), translator::translate(w2->get_name()));
	}
	return sortreverse ? order > 0 : order < 0;
}

void goods_frame_t::set_linetype(simline_t::linetype const linetype, sint32 const speed_value)
{
	waytype_t const waytype = simline_t::linetype_to_waytype(linetype);
	sint32 const average_speed = welt->get_average_speed(waytype);
	sint32 new_value = speed_value;
	if (new_value == 0) {
		double const relative_speed_change = speed.get_value() / (double)welt->get_average_speed(simline_t::linetype_to_waytype(last_scheduletype));
		new_value = (sint32)(average_speed * relative_speed_change + 0.5);
	}

	sint32 const maximum_speed = vehicle_builder_t::get_fastest_vehicle_speed(waytype, welt->get_current_month(),
		welt->get_settings().get_use_timeline(), welt->get_settings().get_allow_buying_obsolete_vehicles());
	if (maximum_speed > 0) {
		speed.enable();
		speed.set_limits(1, maximum_speed);
	}
	else {
		speed.disable();
		speed.set_limits(new_value, new_value);
	}
	speed.set_value(new_value);

	selected_speed = new_value;
	average_selection = new_value == average_speed;

	if (last_scheduletype != linetype) {
		for (size_t index = 0 ; index < simline_t::MAX_LINE_TYPE ; index++) {
			if (linetype_selection_map[index] == linetype) {
				scheduletype.set_selection((int)index);
				break;
			}
		}
	}
	last_scheduletype = linetype;
}

// creates the list and pass it to the child function good_stats, which does the display stuff ...
void goods_frame_t::sort_list()
{
	sint32 const average_speed = welt->get_average_speed(simline_t::linetype_to_waytype(last_scheduletype));

	// update all strings
	speed_message.clear();
	speed_message.printf(translator::translate("Bonus Speed: %i km/h"),
		average_speed
	);
	speed_message.append("\n");
	speed_message.printf(translator::translate("Bonus Multiplier: %i%%"),
		(int)(100.0 * 10.0 * (((double)selected_speed / (double)average_speed) - 1.0))
	);
	speed_message.append("\n");
	if (!speed.enabled()) {
		speed_message.append(translator::translate("No vehicles are available for purchase."));
	}
	if (speed_text.get_min_size().h > speed_text.get_size().h) {
		resize(scr_coord(0,0));
	}

	// update buttons
	sortedby.set_selection(sortby);
	sorteddir.pressed = sortreverse;
	filter_goods_toggle.pressed = filter_goods;
	sort_row->set_size(sort_row->get_min_size());

	// now prepare the sort
	// Fetch the list of goods produced by the factories that exist in the current game
	const vector_tpl<const goods_desc_t*> &goods_in_game = welt->get_goods_list();

	good_list.clear();
	for(unsigned int i=0; i<goods_manager_t::get_count(); i++) {
		const goods_desc_t * wtyp = goods_manager_t::get_info(i);

		// we skip goods that don't generate income
		// this should only be true for the special good 'None'
		if(  wtyp->get_value()!=0  &&  (!filter_goods  ||  goods_in_game.is_contained(wtyp))  ) {
			good_list.insert_ordered( wtyp, compare_goods );
		}
	}

	goods_stats.update_goodslist( good_list, (100 * selected_speed) / average_speed);
}

/**
 * This method is called if an action is triggered
 */
bool goods_frame_t::action_triggered( gui_action_creator_t *comp,value_t p)
{
	if(comp == &sortedby) {
		// sort by what
		sortby = (goods_frame_t::sort_mode_t)p.i;
	}
	else if(comp == &sorteddir) {
		// order
		sortreverse ^= 1;
	}
	else if(comp == &filter_goods_toggle) {
		filter_goods = !filter_goods;
		build_linetype_list(filter_goods);
	}
	else if(comp == &scheduletype) {
		set_linetype(linetype_selection_map[scheduletype.get_selection()]);
	}
	else if(comp == &speed) {
		set_linetype(last_scheduletype, speed.get_value());
	}
	sort_list();

	return true;
}


uint32 goods_frame_t::get_rdwr_id()
{
	return magic_goodslist;
}



void goods_frame_t::rdwr( loadsave_t *file )
{
	// This used to be realitive speed in percentage but is now absolute speed in km/h.
	sint16 saved_speed = (sint16)std::min(selected_speed, (sint32)std::numeric_limits<sint16>::max());
	file->rdwr_short( saved_speed );

	file->rdwr_bool( sortreverse );
	file->rdwr_bool( filter_goods );
	sint16 s = last_scheduletype;
	file->rdwr_short( s );
	sint16 b = sortby;
	file->rdwr_short( b );

	if(  file->is_loading()  ) {
		build_linetype_list(filter_goods);
		if(saved_speed < 1) {
			saved_speed = 1;
		}
		set_linetype((simline_t::linetype)s, saved_speed);
		sortby = (sort_mode_t)b;
		sort_list();
	}
}
