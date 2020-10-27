/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <algorithm>

#include "goods_frame_t.h"
#include "components/gui_scrollpane.h"


#include "../bauer/goods_manager.h"
#include "../descriptor/goods_desc.h"
#include "../dataobj/translator.h"

// For waytype_t
#include "../simtypes.h"

#include "../simcolor.h"
#include "simwin.h"
#include "../simworld.h"
#include "../simconvoi.h"

// For revenue stuff
#include "../descriptor/goods_desc.h"

/**
 * This variable defines the current speed for the purposes of calculating
 * journey time, which in turn affects comfort. Adaopted from the old speed
 * bonus code, which was put into its present form by Neroden circa 2013.
 */
uint32 goods_frame_t::vehicle_speed = 50;

/**
 * This variable defines by which column the table is sorted
 * Values: 0 = Unsorted (passengers and mail first)
 *         1 = Alphabetical
 *         2 = Revenue
 * @author prissi
 */
goods_frame_t::sort_mode_t goods_frame_t::sortby = by_number;
static uint8 default_sortmode = 0;

/**
 * This variable defines the sort order (ascending or descending)
 * Values: 1 = ascending, 2 = descending)
 * @author Markus Weber
 */
bool goods_frame_t::sortreverse = false;

uint32 goods_frame_t::distance_meters = 1000;
uint16 goods_frame_t::distance = 1;
uint8 goods_frame_t::comfort = 50;
uint8 goods_frame_t::catering_level = 0;
uint8 goods_frame_t::g_class = 0;

const char *goods_frame_t::sort_text[SORT_MODES] = {
	"gl_btn_unsort",
	"gl_btn_sort_name",
	"gl_btn_sort_revenue",
	"gl_btn_sort_catg",
	"gl_btn_sort_weight"
};

/**
 * This variable controls whether all goods are displayed, or
 * just the ones relevant to the current game
 * Values: false = all goods shown, true = relevant goods shown
 * @author falconne
 */
bool goods_frame_t::filter_goods = false;

goods_frame_t::goods_frame_t() :
	gui_frame_t( translator::translate("gl_title") ),
	goods_stats(),
	scrolly(&goods_stats)
{
	set_table_layout(1, 0);

	add_table(2, 5);
	{
		speed[0] = 0;

		new_component<gui_label_t>("distance");

		distance_txt[0] = 0;
		comfort_txt[0] = 0;
		catering_txt[0] = 0;
		class_txt[0] = 0;
		distance_meters = (sint32) 1000 * distance;

		distance_input.set_limits( 1, 9999 );
		distance_input.set_value( distance );
		distance_input.wrap_mode( false );
		distance_input.add_listener( this );
		add_component(&distance_input);

		new_component<gui_label_t>("Comfort");
		comfort_input.set_limits( 1, 255 );
		comfort_input.set_value( comfort );
		comfort_input.wrap_mode( false );
		comfort_input.add_listener( this );
		add_component(&comfort_input);

		new_component<gui_label_t>("Catering level");
		catering_input.set_limits( 0, 5 );
		catering_input.set_value( catering_level );
		catering_input.wrap_mode( false );
		catering_input.add_listener( this );
		add_component(&catering_input);

		new_component<gui_label_t>("Average speed");
		speed_input.set_limits(19, 9999);
		speed_input.set_value(vehicle_speed);
		speed_input.wrap_mode(false);
		speed_input.add_listener(this);
		add_component(&speed_input);

		new_component<gui_label_t>("Class");
		class_input.set_limits(0, max(goods_manager_t::passengers->get_number_of_classes() - 1, goods_manager_t::mail->get_number_of_classes() - 1)); // TODO: Extrapolate this to show the class names as well as just the number
		class_input.set_value(g_class);
		class_input.wrap_mode(false);
		class_input.add_listener(this);
		add_component(&class_input);
	}
	end_table();

	new_component<gui_label_t>("hl_txt_sort");

	// sort mode
	sort_row = add_table(5, 1);
	{
		for (int i = 0; i < SORT_MODES; i++) {
			sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
		}
		sortedby.set_selection(default_sortmode);
		sortedby.add_listener(this);
		add_component(&sortedby); // (1,1)

		// sort ascend/descend button
		sort_asc.init(button_t::arrowup_state, "");
		sort_asc.set_tooltip(translator::translate("hl_btn_sort_asc"));
		sort_asc.add_listener(this);
		sort_asc.pressed = sortreverse;
		add_component(&sort_asc); // (2,1)

		sort_desc.init(button_t::arrowdown_state, "");
		sort_desc.set_tooltip(translator::translate("hl_btn_sort_desc"));
		sort_desc.add_listener(this);
		sort_desc.pressed = !sortreverse;
		add_component(&sort_desc); // (3,1)
		new_component<gui_margin_t>(LINESPACE); // (4,1)

		filter_goods_toggle.init(button_t::square_state, "Show only used");
		filter_goods_toggle.set_tooltip(translator::translate("Only show goods which are currently handled by factories"));
		filter_goods_toggle.add_listener(this);
		filter_goods_toggle.pressed = filter_goods;
		add_component(&filter_goods_toggle); // (5,1)
	}
	end_table();

	add_component(&scrolly);
	scrolly.set_maximize(true);

	sort_list();

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
	set_resizemode(diagonal_resize);
}


bool goods_frame_t::compare_goods(goods_desc_t const* const w1, goods_desc_t const* const w2)
{
	int order = 0;

	switch (sortby)
	{
		case by_number:
			order = w1->get_index() - w2->get_index();
			break;
		case by_revenue:
			{
				sint64 price[2];
				const uint16 journey_tenths = (uint16)tenths_from_meters_and_kmh(distance_meters, vehicle_speed);
				price[0] = w1->get_total_fare(distance_meters, 0, comfort, catering_level, min(g_class, w1->get_number_of_classes() - 1), journey_tenths);
				price[1] = w2->get_total_fare(distance_meters, 0, comfort, catering_level, min(g_class, w2->get_number_of_classes() - 1), journey_tenths);

				order = price[0] - price[1];
			}
			break;
		case by_category:
			order = w1->get_catg() - w2->get_catg();
			break;
		case by_weight:
			order = w1->get_weight_per_unit() - w2->get_weight_per_unit();
		default: ; // make compiler happy, order will be determined below anyway
	}
	if(  order==0  ) {
		// sort by name if not sorted or not unique
		order = strcmp(translator::translate(w1->get_name()), translator::translate(w2->get_name()));
	}
	return sortreverse ? order > 0 : order < 0;
}


// creates the list and pass it to the child function good_stats, which does the display stuff ...
void goods_frame_t::sort_list()
{
	// Fetch the list of goods produced by the factories that exist in the current game
	const vector_tpl<const goods_desc_t*> &goods_in_game = welt->get_goods_list();

	good_list.clear();
	for(unsigned int i=0; i<goods_manager_t::get_count(); i++) {
		const goods_desc_t * wtyp = goods_manager_t::get_info(i);

		// Skip goods not in the game
		// Do not skip goods which don't generate income -- it makes it hard to debug paks
		// Do skip the special good "None"
		if(  (wtyp != goods_manager_t::none) && (!filter_goods || goods_in_game.is_contained(wtyp))  ) {
			good_list.insert_ordered(wtyp, compare_goods);
		}
	}

	goods_stats.update_goodslist(good_list, vehicle_speed, goods_frame_t::distance_meters, goods_frame_t::comfort, goods_frame_t::catering_level, g_class);
}


/**
 * This method is called if an action is triggered
 */
bool goods_frame_t::action_triggered( gui_action_creator_t *comp,value_t v)
{
	if(comp == &sortedby) {
		// sort by what
		int tmp = sortedby.get_selection();
		if (tmp >= 0 && tmp < sortedby.count_elements())
		{
			sortedby.set_selection(tmp);
			sortby =(goods_frame_t::sort_mode_t)tmp;
		}
		else {
			sortedby.set_selection(0);
			sortby = goods_frame_t::by_number;
		}
		default_sortmode = (uint8)tmp;
		sort_list();
	}
	else if (comp == &sort_asc || comp == &sort_desc) {
		// order
		sortreverse ^= 1;
		sort_list();
		sort_asc.pressed = sortreverse;
		sort_desc.pressed = !sortreverse;
	}
	else if (comp == &speed_input) {
		vehicle_speed = v.i;
		sort_list();
	}
	else if (comp == &distance_input) {
		distance = v.i;
		distance_meters = (sint32) 1000 * distance;
		sort_list();
	}
	else if (comp == &comfort_input) {
		comfort = v.i;
		sort_list();
	}
	else if (comp == &catering_input) {
		catering_level = v.i;
		sort_list();
	}
	else if (comp == &class_input) {
		g_class = v.i;
		sort_list();
	}
	else if(comp == &filter_goods_toggle) {
		filter_goods = !filter_goods;
		filter_goods_toggle.pressed = filter_goods;
		sort_list();
	}

	return true;
}


/**
 * Draw the component
 * @author Hj. Malthaner
 */
void goods_frame_t::draw(scr_coord pos, scr_size size)
{
	gui_frame_t::draw(pos, size);
}


uint32 goods_frame_t::get_rdwr_id()
{
	return magic_goodslist;
}


void goods_frame_t::rdwr(loadsave_t *file)
{
	file->rdwr_short(distance);
	file->rdwr_byte(comfort);
	file->rdwr_byte(catering_level);
	file->rdwr_long(vehicle_speed);
	file->rdwr_byte(g_class);
	file->rdwr_bool(sort_asc.pressed);
	file->rdwr_bool(filter_goods_toggle.pressed);
	uint8 s = default_sortmode;
	file->rdwr_byte(s);
	sint16 b = sortby;
	file->rdwr_short(b);

	if (file->is_loading()) {
		sortedby.set_selection(s);
		sortby = (sort_mode_t)b;
		sortreverse = sort_asc.pressed;
		sort_desc.pressed = !sort_asc.pressed;
		sort_list();
		distance_input.set_value(distance);
		comfort_input.set_value(comfort);
		catering_input.set_value(catering_level);
		speed_input.set_value(vehicle_speed);
		class_input.set_value(g_class);
		filter_goods = filter_goods_toggle.pressed;
		default_sortmode = s;
	}
}
