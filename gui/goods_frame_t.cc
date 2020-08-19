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
	sort_label(translator::translate("hl_txt_sort")),
	change_speed_label(speed, COL_WHITE, gui_label_t::right),
	change_distance_label(distance_txt,COL_WHITE,gui_label_t::right),
	change_comfort_label(comfort_txt,COL_WHITE,gui_label_t::right),
	change_catering_label(catering_txt,COL_WHITE,gui_label_t::right),
	change_class_label(class_txt,COL_WHITE,gui_label_t::right),
	goods_stats(),
	scrolly(&goods_stats)
{
	int y=D_BUTTON_HEIGHT+4-D_TITLEBAR_HEIGHT;
	speed[0] = 0;

	change_speed_label.set_text(speed);
	change_speed_label.set_pos(scr_coord(BUTTON4_X + 5, y));
	add_component(&change_speed_label);

	y=D_BUTTON_HEIGHT;

	distance_txt[0] = 0;
	comfort_txt[0] = 0;
	catering_txt[0] = 0;
	class_txt[0] = 0;
	distance_meters = (sint32) 1000 * distance;

	distance_input.set_pos(scr_coord(BUTTON4_X-22, y) );
	distance_input.set_size(scr_size(60, D_BUTTON_HEIGHT));
	distance_input.set_limits( 1, 9999 );
	distance_input.set_value( distance );
	distance_input.wrap_mode( false );
	distance_input.add_listener( this );
	add_component(&distance_input);

	comfort_input.set_pos(scr_coord(BUTTON4_X-22, y+=D_BUTTON_HEIGHT+1) );
	comfort_input.set_size(scr_size(60, D_BUTTON_HEIGHT));
	comfort_input.set_limits( 1, 255 );
	comfort_input.set_value( comfort );
	comfort_input.wrap_mode( false );
	comfort_input.add_listener( this );
	add_component(&comfort_input);

	catering_input.set_pos(scr_coord(BUTTON4_X-22, y+=D_BUTTON_HEIGHT+1) );
	catering_input.set_size(scr_size(60, D_BUTTON_HEIGHT));
	catering_input.set_limits( 0, 5 );
	catering_input.set_value( catering_level );
	catering_input.wrap_mode( false );
	catering_input.add_listener( this );
	add_component(&catering_input);

	speed_input.set_pos(scr_coord(BUTTON4_X - 22, y+=D_BUTTON_HEIGHT+1));
	speed_input.set_size(scr_size(60, D_BUTTON_HEIGHT));
	speed_input.set_limits(19, 9999);
	speed_input.set_value(vehicle_speed);
	speed_input.wrap_mode(false);
	speed_input.add_listener(this);
	add_component(&speed_input);

	class_input.set_pos(scr_coord(BUTTON4_X - 22, y+=D_BUTTON_HEIGHT+1));
	class_input.set_size(scr_size(60, D_BUTTON_HEIGHT));
	class_input.set_limits(0, max(goods_manager_t::passengers->get_number_of_classes() - 1, goods_manager_t::mail->get_number_of_classes() - 1)); // TODO: Extrapolate this to show the class names as well as just the number
	class_input.set_value(g_class);
	class_input.wrap_mode(false);
	class_input.add_listener(this);
	add_component(&class_input);

	y += D_BUTTON_HEIGHT + D_V_SPACE*2;

	sort_label.set_pos(scr_coord(BUTTON1_X, y));
	add_component(&sort_label);

	y += LINESPACE+1;

	sortedby.set_pos(scr_coord(BUTTON1_X, y));
	sortedby.set_size(scr_size(D_BUTTON_WIDTH*1.5, D_BUTTON_HEIGHT));
	sortedby.set_max_size(scr_size(D_BUTTON_WIDTH*1.5, LINESPACE * 4));
	for (int i = 0; i < SORT_MODES; i++) {
		sortedby.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(sort_text[i]), SYSCOL_TEXT));
	}
	sortedby.set_selection(default_sortmode);
	sortedby.add_listener(this);
	add_component(&sortedby);

	// sort ascend/descend button
	sort_asc.init(button_t::arrowup_state, "", scr_coord(BUTTON1_X + D_BUTTON_WIDTH * 1.5 + D_H_SPACE, y + 1), scr_size(D_ARROW_UP_WIDTH, D_ARROW_UP_HEIGHT));
	sort_asc.set_tooltip(translator::translate("hl_btn_sort_asc"));
	sort_asc.add_listener(this);
	sort_asc.pressed = sortreverse;
	add_component(&sort_asc);

	sort_desc.init(button_t::arrowdown_state, "", sort_asc.get_pos() + scr_coord(D_ARROW_UP_WIDTH + 2, 0), scr_size(D_ARROW_DOWN_WIDTH, D_ARROW_DOWN_HEIGHT));
	sort_desc.set_tooltip(translator::translate("hl_btn_sort_desc"));
	sort_desc.add_listener(this);
	sort_desc.pressed = !sortreverse;
	add_component(&sort_desc);


	filter_goods_toggle.init(button_t::square_state, "Show only used", scr_coord(BUTTON2_X + D_BUTTON_WIDTH*1.5 + D_H_SPACE, y));
	filter_goods_toggle.set_tooltip(translator::translate("Only show goods which are currently handled by factories"));
	filter_goods_toggle.add_listener(this);
	filter_goods_toggle.pressed = filter_goods;
	add_component(&filter_goods_toggle);

	y += D_BUTTON_HEIGHT+D_V_SPACE;

	scrolly.set_pos(scr_coord(1, y));
	scrolly.set_scroll_amount_y(LINESPACE+1);
	add_component(&scrolly);

	sort_list();

	int h = (goods_manager_t::get_count()+1)*(LINESPACE+1)+y;
	if(h>450) {
		h = y+27*(LINESPACE+1)+D_TITLEBAR_HEIGHT+1;
	}
	set_windowsize(scr_size(D_DEFAULT_WIDTH, h));
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH,3*(LINESPACE+1)+D_TITLEBAR_HEIGHT+y+1));

	set_resizemode(vertical_resize);
	resize (scr_coord(0,0));
}


bool goods_frame_t::compare_goods(uint16 const a, uint16 const b)
{
	const goods_desc_t* w[2];
	w[0] = goods_manager_t::get_info(a);
	w[1] = goods_manager_t::get_info(b);

	int order = 0;

	switch (sortby)
	{
		case by_number:
			order = a - b;
			break;
		case by_revenue:
			{
				sint64 price[2];
				for(uint8 i = 0; i < 2; i ++)
				{
					const uint16 journey_tenths = (uint16)tenths_from_meters_and_kmh(distance_meters, vehicle_speed);

					price[i] = w[i]->get_total_fare(distance_meters, 0, comfort, catering_level, min(g_class, w[i]->get_number_of_classes() - 1), journey_tenths);
				}

				order = price[0] - price[1];
			}
			break;
		case by_category:
			order = w[1]->get_catg()-w[0]->get_catg();
			break;
		case by_weight:
			order = w[0]->get_weight_per_unit() - w[1]->get_weight_per_unit();
		default: ; // make compiler happy, order will be determined below anyway
	}
	if(  order==0  ) {
		// sort by name if not sorted or not unique
		order = strcmp(translator::translate(w[0]->get_name()), translator::translate(w[1]->get_name()));
	}
	return sortreverse ? order > 0 : order < 0;
}


// creates the list and pass it to the child function good_stats, which does the display stuff ...
void goods_frame_t::sort_list()
{
	// Fetch the list of goods produced by the factories that exist in the current game
	const vector_tpl<const goods_desc_t*> &goods_in_game = welt->get_goods_list();

	int n=0;
	for(unsigned int i=0; i<goods_manager_t::get_count(); i++) {
		const goods_desc_t * wtyp = goods_manager_t::get_info(i);

		// Skip goods not in the game
		// Do not skip goods which don't generate income -- it makes it hard to debug paks
		// Do skip the special good "None"
		if(  (wtyp != goods_manager_t::none) && (!filter_goods || goods_in_game.is_contained(wtyp))  ) {
			good_list[n++] = i;
		}
	}

	std::sort(good_list, good_list + n, compare_goods);

	goods_stats.update_goodslist(good_list, vehicle_speed, n, goods_frame_t::distance_meters, goods_frame_t::comfort, goods_frame_t::catering_level, g_class);
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void goods_frame_t::resize(const scr_coord delta)
{
	gui_frame_t::resize(delta);
	scr_size size = get_windowsize()-scrolly.get_pos()-scr_size(0,D_TITLEBAR_HEIGHT+2);
	scrolly.set_size(size);
	sortedby.set_max_size(scr_size(D_BUTTON_WIDTH*1.5, scrolly.get_size().h));
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
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

	descriptive_text.clear();
	pos.y += D_BUTTON_HEIGHT;
	// TODO: Add translation entry for these
	descriptive_text.printf("%s:", translator::translate("Distance"));
	display_multiline_text(pos.x + D_MARGIN_LEFT, pos.y += D_BUTTON_HEIGHT + 1, descriptive_text, SYSCOL_TEXT_HIGHLIGHT);
	descriptive_text.clear();

	descriptive_text.printf("%s:", translator::translate("Comfort"));
	display_multiline_text(pos.x + D_MARGIN_LEFT, pos.y += D_BUTTON_HEIGHT + 1, descriptive_text, SYSCOL_TEXT_HIGHLIGHT);
	descriptive_text.clear();

	descriptive_text.printf("%s:", translator::translate("Catering level"));
	display_multiline_text(pos.x + D_MARGIN_LEFT, pos.y += D_BUTTON_HEIGHT + 1, descriptive_text, SYSCOL_TEXT_HIGHLIGHT);
	descriptive_text.clear();

	descriptive_text.printf("%s:", translator::translate("Average speed"));
	display_multiline_text(pos.x + D_MARGIN_LEFT, pos.y += D_BUTTON_HEIGHT + 1, descriptive_text, SYSCOL_TEXT_HIGHLIGHT);
	descriptive_text.clear();

	descriptive_text.printf("%s:", translator::translate("Class"));
	display_multiline_text(pos.x + D_MARGIN_LEFT, pos.y += D_BUTTON_HEIGHT + 1, descriptive_text, SYSCOL_TEXT_HIGHLIGHT);
	descriptive_text.clear();
}
