/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Goods list dialog
 */

#include <algorithm>

#include "goods_frame_t.h"
#include "components/gui_scrollpane.h"


#include "../bauer/warenbauer.h"
#include "../besch/ware_besch.h"
#include "../dataobj/translator.h"

#include "../simcolor.h"
#include "../simworld.h"

/**
 * This variable defines the current speed for bonus calculation
 * @author prissi
 */
int goods_frame_t::relative_speed_change=100;

/**
 * This variable defines by which column the table is sorted
 * Values: 0 = Unsorted (passengers and mail first)
 *         1 = Alphabetical
 *         2 = Revenue
 * @author prissi
 */
goods_frame_t::sort_mode_t goods_frame_t::sortby = unsortiert;

/**
 * This variable defines the sort order (ascending or descending)
 * Values: 1 = ascending, 2 = descending)
 * @author Markus Weber
 */
bool goods_frame_t::sortreverse = false;

const char *goods_frame_t::sort_text[SORT_MODES] = {
	"gl_btn_unsort",
	"gl_btn_sort_name",
	"gl_btn_sort_revenue",
	"gl_btn_sort_bonus",
	"gl_btn_sort_catg"
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
	change_speed_label(NULL,SYSCOL_TEXT_HIGHLIGHT,gui_label_t::right),
	goods_stats(),
	scrolly(&goods_stats)
{
	int y=D_BUTTON_HEIGHT+4-D_TITLEBAR_HEIGHT;

	speed_bonus[0] = 0;
	speed_down.init(button_t::repeatarrowleft, "", scr_coord(BUTTON4_X-20, y), scr_size(10,D_BUTTON_HEIGHT));
	speed_down.add_listener(this);
	add_komponente(&speed_down);

	change_speed_label.set_text(speed_bonus);
	change_speed_label.set_width(display_get_char_max_width("-0123456789")*4);
	change_speed_label.align_to(&speed_down, ALIGN_LEFT | ALIGN_EXTERIOR_H | ALIGN_CENTER_V,scr_coord(D_V_SPACE,0));
	add_komponente(&change_speed_label);

	speed_up.init(button_t::repeatarrowright, "",speed_down.get_pos());
	speed_up.align_to(&change_speed_label, ALIGN_LEFT | ALIGN_EXTERIOR_H, scr_coord(D_V_SPACE,0));
	speed_up.add_listener(this);
	add_komponente(&speed_up);
	y=D_BUTTON_HEIGHT+4+5*LINESPACE;

	filter_goods_toggle.init(button_t::square_state, "Show only used", scr_coord(BUTTON1_X, y));
	filter_goods_toggle.set_tooltip(translator::translate("Only show goods which are currently handled by factories"));
	filter_goods_toggle.add_listener(this);
	filter_goods_toggle.pressed = filter_goods;
	add_komponente(&filter_goods_toggle);
	y += LINESPACE+2;

	sort_label.set_pos(scr_coord(BUTTON1_X, y));
	add_komponente(&sort_label);

	y += LINESPACE+1;

	sortedby.init(button_t::roundbox, "", scr_coord(BUTTON1_X, y), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	sortedby.add_listener(this);
	add_komponente(&sortedby);

	sorteddir.init(button_t::roundbox, "", scr_coord(BUTTON2_X, y), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	sorteddir.add_listener(this);
	add_komponente(&sorteddir);

	y += D_BUTTON_HEIGHT+2;

	scrolly.set_pos(scr_coord(1, y));
	scrolly.set_scroll_amount_y(LINESPACE+1);
	add_komponente(&scrolly);

	sort_list();

	int h = (warenbauer_t::get_waren_anzahl()+1)*(LINESPACE+1)+y;
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
	ware_besch_t const* const w1 = warenbauer_t::get_info(a);
	ware_besch_t const* const w2 = warenbauer_t::get_info(b);

	int order = 0;

	switch (sortby) {
		case 0: // sort by number
			order = a - b;
			break;
		case 2: // sort by revenue
			{
				const sint32 grundwert1281 = w1->get_preis() * goods_frame_t::welt->get_settings().get_bonus_basefactor();
				const sint32 grundwert_bonus1 = w1->get_preis()*(1000l+(relative_speed_change-100l)*w1->get_speed_bonus());
				const sint32 price1 = (grundwert1281>grundwert_bonus1 ? grundwert1281 : grundwert_bonus1);
				const sint32 grundwert1282 = w2->get_preis() * goods_frame_t::welt->get_settings().get_bonus_basefactor();
				const sint32 grundwert_bonus2 = w2->get_preis()*(1000l+(relative_speed_change-100l)*w2->get_speed_bonus());
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


// creates the list and pass it to the child function good_stats, which does the display stuff ...
void goods_frame_t::sort_list()
{
	sortedby.set_text(sort_text[sortby]);
	sorteddir.set_text(sortreverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");

	// Fetch the list of goods produced by the factories that exist in the current game
	const vector_tpl<const ware_besch_t*> &goods_in_game = welt->get_goods_list();

	int n=0;
	for(unsigned int i=0; i<warenbauer_t::get_waren_anzahl(); i++) {
		const ware_besch_t * wtyp = warenbauer_t::get_info(i);

		// Hajo: we skip goods that don't generate income
		//       this should only be true for the special good 'None'
		if(  wtyp->get_preis()!=0  &&  (!filter_goods  ||  goods_in_game.is_contained(wtyp))  ) {
			good_list[n++] = i;
		}
	}

	std::sort(good_list, good_list + n, compare_goods);

	goods_stats.update_goodslist( good_list, relative_speed_change, n );
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void goods_frame_t::resize(const scr_coord delta)
{
	gui_frame_t::resize(delta);
	scr_size size = get_windowsize()-scrolly.get_pos()-scr_size(0,D_TITLEBAR_HEIGHT);
	scrolly.set_size(size);
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool goods_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp == &sortedby) {
		// sort by what
		sortby = (sort_mode_t)((int)(sortby+1)%(int)SORT_MODES);
		sort_list();
	}
	else if(komp == &sorteddir) {
		// order
		sortreverse ^= 1;
		sort_list();
	}
	else if(komp == &speed_down) {
		if(relative_speed_change>1) {
			relative_speed_change --;
			sort_list();
		}
	}
	else if(komp == &speed_up) {
		relative_speed_change ++;
		sort_list();
	}
	else if(komp == &filter_goods_toggle) {
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

	sprintf(speed_bonus,"%i",relative_speed_change-100);

	speed_message.clear();
	speed_message.printf(translator::translate("Speedbonus\nroad %i km/h, rail %i km/h\nships %i km/h, planes %i km/h."),
		(welt->get_average_speed(road_wt)*relative_speed_change)/100,
		(welt->get_average_speed(track_wt)*relative_speed_change)/100,
		(welt->get_average_speed(water_wt)*relative_speed_change)/100,
		(welt->get_average_speed(air_wt)*relative_speed_change)/100
	);
	display_multiline_text( pos.x + D_MARGIN_LEFT, pos.y + D_BUTTON_HEIGHT + 4, speed_message, SYSCOL_TEXT_HIGHLIGHT );

	speed_message.clear();
	speed_message.printf(translator::translate("tram %i km/h, monorail %i km/h\nmaglev %i km/h, narrowgauge %i km/h."),
		(welt->get_average_speed(tram_wt)*relative_speed_change)/100,
		(welt->get_average_speed(monorail_wt)*relative_speed_change)/100,
		(welt->get_average_speed(maglev_wt)*relative_speed_change)/100,
		(welt->get_average_speed(narrowgauge_wt)*relative_speed_change)/100
	);
	display_multiline_text( pos.x + D_MARGIN_LEFT, pos.y + D_BUTTON_HEIGHT + 4 + 3 * LINESPACE, speed_message, SYSCOL_TEXT_HIGHLIGHT );

	speed_message.clear();
	speed_message.printf(translator::translate("100 km/h = %i tiles/month"),
		welt->speed_to_tiles_per_month(kmh_to_speed(100))
	);
	display_multiline_text( pos.x + D_MARGIN_LEFT, pos.y + D_BUTTON_HEIGHT + 4 + 5 * LINESPACE, speed_message, SYSCOL_TEXT_HIGHLIGHT );
}
