/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 * Copyright 2013 Nathanael Nerode, James Petts
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

// For waytype_t
#include "../simtypes.h"

#include "../simcolor.h"
#include "../simworld.h"
#include "../simconvoi.h"

// For revenue stuff
#include "../besch/ware_besch.h"

/**
 * This variable defines the current speed for bonus calculation
 * This is in the "percentage" form in which it runs from -100 to +infinity,
 * where 0 means "standard speed" and +100 means "100% more than standard speed"
 * @author neroden
 */
int goods_frame_t::relative_speed_percentage = 0;

/**
 * This variable contains the selected waytype
 * Relevant for paks with different speedbonuses by waytype
 */
waytype_t goods_frame_t::wtype = road_wt;

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

uint32 goods_frame_t::distance_meters = 1000;
uint16 goods_frame_t::distance = 1;
uint8 goods_frame_t::comfort = 50;
uint8 goods_frame_t::catering_level = 0;

const char *goods_frame_t::sort_text[SORT_MODES] = {
	"gl_btn_unsort",
	"gl_btn_sort_name",
	"gl_btn_sort_revenue",
	"gl_btn_sort_bonus",
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
	change_speed_label(speed_bonus,COL_WHITE,gui_label_t::right),
	change_distance_label(distance_txt,COL_WHITE,gui_label_t::right),
	change_comfort_label(comfort_txt,COL_WHITE,gui_label_t::right),
	change_catering_label(catering_txt,COL_WHITE,gui_label_t::right),
	goods_stats(),
	scrolly(&goods_stats)
{
	int y=D_BUTTON_HEIGHT+4-D_TITLEBAR_HEIGHT;

	speed_bonus[0] = 0;

	change_speed_label.set_text(speed_bonus);
	change_speed_label.set_pos(scr_coord(BUTTON4_X+5, y));
	add_component(&change_speed_label);

	//speed_down.init(button_t::repeatarrowleft, "", koord(BUTTON4_X-20, y), koord(10,D_BUTTON_HEIGHT));
	//speed_down.add_listener(this);
	//add_component(&speed_down);

	//speed_up.init(button_t::repeatarrowright, "", koord(BUTTON4_X+10, y), koord(10,D_BUTTON_HEIGHT));
	//speed_up.add_listener(this);
	//add_component(&speed_up);

	y=D_BUTTON_HEIGHT+4+5*LINESPACE;

	distance_txt[0] = 0;
	comfort_txt[0] = 0;
	catering_txt[0] = 0;
	distance_meters = (sint32) 1000 * distance;


	distance_input.set_pos(scr_coord(BUTTON4_X-22, y) );
	distance_input.set_size(scr_size(60, D_BUTTON_HEIGHT));
	distance_input.set_limits( 1, 9999 );
	distance_input.set_value( distance );
	distance_input.wrap_mode( false );
	distance_input.add_listener( this );
	add_component(&distance_input);

	comfort_input.set_pos(scr_coord(BUTTON4_X-22, y + 12) );
	comfort_input.set_size(scr_size(60, D_BUTTON_HEIGHT));
	comfort_input.set_limits( 1, 255 );
	comfort_input.set_value( comfort );
	comfort_input.wrap_mode( false );
	comfort_input.add_listener( this );
	add_component(&comfort_input);

	catering_input.set_pos(scr_coord(BUTTON4_X-22, y + 24) );
	catering_input.set_size(scr_size(60, D_BUTTON_HEIGHT));
	catering_input.set_limits( 0, 5 );
	catering_input.set_value( catering_level );
	catering_input.wrap_mode( false );
	catering_input.add_listener( this );
	add_component(&catering_input);

	speed_input.set_pos(scr_coord(BUTTON4_X-22, y + 36) );
	speed_input.set_size(scr_size(60, D_BUTTON_HEIGHT));
	speed_input.set_limits( -99, 9999 );
	speed_input.set_value( relative_speed_percentage );
	speed_input.wrap_mode( false );
	speed_input.add_listener( this );
	add_component(&speed_input);

	way_type.add_listener(this);
	add_component(&way_type);
	way_type.clear_elements();
	static const char *txt_wtype[8] = { "Road", "Rail", "Ship", "Monorail", "Maglev", "Tram", "Narrow gauge", "Air" };
	for(uint8 i = 0; i < 8; i++)
	{
		way_type.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(txt_wtype[i]), COL_WHITE ) );
	}
	way_type.set_selection(0);
	way_type.set_pos(scr_coord(BUTTON4_X-22, y + 48));
	way_type.set_size(scr_size(110, 24));
	way_type.set_max_size(scr_size(96, LINESPACE*2+2+16));
	way_type.set_highlight_color(1);
	
	y=D_BUTTON_HEIGHT+6+5*LINESPACE + 25;

//	speed_down.init(button_t::repeatarrowleft, "", scr_coord(BUTTON4_X-20, y), scr_size(10,D_BUTTON_HEIGHT));
//	speed_down.add_listener(this);
//	add_component(&speed_down);
//
//	change_speed_label.set_text(speed_bonus);
//	change_speed_label.set_width(display_get_char_max_width("-0123456789")*4);
//	change_speed_label.align_to(&speed_down, ALIGN_LEFT | ALIGN_EXTERIOR_H | ALIGN_CENTER_V,scr_coord(D_V_SPACE,0));
//	add_component(&change_speed_label);
//
//	speed_up.init(button_t::repeatarrowright, "",speed_down.get_pos());
//	speed_up.align_to(&change_speed_label, ALIGN_LEFT | ALIGN_EXTERIOR_H, scr_coord(D_V_SPACE,0));
//	speed_up.add_listener(this);
//	add_component(&speed_up);
//	y=D_BUTTON_HEIGHT+4+5*LINESPACE;


	filter_goods_toggle.init(button_t::square_state, "Show only used", scr_coord(BUTTON1_X, y));
	filter_goods_toggle.set_tooltip(translator::translate("Only show goods which are currently handled by factories"));
	filter_goods_toggle.add_listener(this);
	filter_goods_toggle.pressed = filter_goods;
	add_component(&filter_goods_toggle);
	y += LINESPACE+2;

	sort_label.set_pos(scr_coord(BUTTON1_X, y));
	add_component(&sort_label);

	y += LINESPACE+1;

	sortedby.init(button_t::roundbox, "", scr_coord(BUTTON1_X, y), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	sortedby.add_listener(this);
	add_component(&sortedby);

	sorteddir.init(button_t::roundbox, "", scr_coord(BUTTON2_X, y), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	sorteddir.add_listener(this);
	add_component(&sorteddir);

	y += D_BUTTON_HEIGHT+2;

	scrolly.set_pos(scr_coord(1, y));
	scrolly.set_scroll_amount_y(LINESPACE+1);
	add_component(&scrolly);

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
	const ware_besch_t* w[2];
	w[0] = warenbauer_t::get_info(a);
	w[1] = warenbauer_t::get_info(b);

	int order = 0;

	switch (sortby) 
	{
		case 0: // sort by number
			order = a - b;
			break;
		case 2: // sort by revenue
			{
				sint64 price[2];
				for(uint8 i = 0; i < 2; i ++)
				{
					sint64 relevant_speed = ( welt->get_average_speed(wtype) * (relative_speed_percentage + 100) ) / 100;
					// Roundoff is deliberate here (get two-digit speed)... question this
					if (relevant_speed <= 0) {
						// Negative and zero speeds will be due to roundoff errors
						relevant_speed = 1;
					}
					const uint16 journey_tenths = (uint16) tenths_from_meters_and_kmh(distance_meters, relevant_speed);

					price[i] = w[i]->get_fare_with_comfort_catering_speedbonus(welt,
							comfort, catering_level, journey_tenths, relative_speed_percentage, distance_meters);
				}

				order = price[0] - price[1];
//=======
//			{
//				const sint32 grundwert1281 = w1->get_preis() * goods_frame_t::welt->get_settings().get_bonus_basefactor();
//				const sint32 grundwert_bonus1 = w1->get_preis()*(1000l+(relative_speed_change-100l)*w1->get_speed_bonus());
//				const sint32 price1 = (grundwert1281>grundwert_bonus1 ? grundwert1281 : grundwert_bonus1);
//				const sint32 grundwert1282 = w2->get_preis() * goods_frame_t::welt->get_settings().get_bonus_basefactor();
//				const sint32 grundwert_bonus2 = w2->get_preis()*(1000l+(relative_speed_change-100l)*w2->get_speed_bonus());
//				const sint32 price2 = (grundwert1282>grundwert_bonus2 ? grundwert1282 : grundwert_bonus2);
//				order = price1-price2;
//>>>>>>> v111.3
			}
			break;
		case 3: // sort by speed bonus
			order = w[0]->get_adjusted_speed_bonus(distance_meters) - w[1]->get_adjusted_speed_bonus(distance_meters);
			break;
		case 4: // sort by catg_index
			order = w[1]->get_catg()-w[0]->get_catg();
			break;
		case 5: // sort by weight
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
	sortedby.set_text(sort_text[sortby]);
	sorteddir.set_text(sortreverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");

	// Fetch the list of goods produced by the factories that exist in the current game
	const vector_tpl<const ware_besch_t*> &goods_in_game = welt->get_goods_list();

	int n=0;
	for(unsigned int i=0; i<warenbauer_t::get_waren_anzahl(); i++) {
		const ware_besch_t * wtyp = warenbauer_t::get_info(i);

		// Skip goods not in the game
		// Do not skip goods which don't generate income -- it makes it hard to debug paks
		// Do skip the special good "None"
		if(  (wtyp != warenbauer_t::nichts) && (!filter_goods || goods_in_game.is_contained(wtyp))  ) {
			good_list[n++] = i;
		}
	}

	std::sort(good_list, good_list + n, compare_goods);

	goods_stats.update_goodslist(good_list, relative_speed_percentage, n, goods_frame_t::distance_meters, goods_frame_t::comfort, goods_frame_t::catering_level, wtype);
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void goods_frame_t::resize(const scr_coord delta)
{
	gui_frame_t::resize(delta);
	scr_size size = get_windowsize()-scrolly.get_pos()-scr_size(0,D_TITLEBAR_HEIGHT+25);
	scrolly.set_size(size);
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool goods_frame_t::action_triggered( gui_action_creator_t *comp,value_t v)
{
	if(comp == &sortedby) {
		// sort by what
		sortby = (sort_mode_t)((int)(sortby+1)%(int)SORT_MODES);
		sort_list();
	}
	else if(comp == &sorteddir) {
		// order
		sortreverse ^= 1;
		sort_list();
	}
	//else if(comp == &speed_down) {
	//	if(relative_speed_change>1) {
	//		relative_speed_change --;
	//		sort_list();
	//	}
	//}
	//else if(comp == &speed_up) {
	//	relative_speed_change ++;
	//	sort_list();
	//}

	else if (comp == &speed_input) {
		relative_speed_percentage = v.i;
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
	else if (comp == &way_type)
	{
		switch(way_type.get_selection())
		{
		default:	
		case 0:
			wtype = road_wt;
			break;

		case 1:
			wtype = track_wt;
			break;

		case 2:
			wtype = water_wt;
			break;

		case 3:
			wtype = monorail_wt;
			break;

		case 4:
			wtype = maglev_wt;
			break;

		case 5:
			wtype = tram_wt;
			break;

		case 6:
			wtype = narrowgauge_wt;
			break;

		case 7:
			wtype = air_wt;
		};
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

	// Standard has:
	//sprintf(speed_bonus,"%i",relative_speed_change-100);

	sint16 speed_ratio = relative_speed_percentage + 100;
	speed_message.clear();
	speed_message.printf(translator::translate("Distance\nComfort\nCatering level\nSpeedbonus\nroad %i km/h, rail %i km/h\nships %i km/h, planes %i km/h."),
		(welt->get_average_speed(road_wt)*speed_ratio)/100,
		(welt->get_average_speed(track_wt)*speed_ratio)/100,
		(welt->get_average_speed(water_wt)*speed_ratio)/100,
		(welt->get_average_speed(air_wt)*speed_ratio)/100
	);
	display_multiline_text( pos.x + D_MARGIN_LEFT, pos.y + D_BUTTON_HEIGHT + 4, speed_message, SYSCOL_TEXT_HIGHLIGHT );

	speed_message.clear();
	speed_message.printf(translator::translate("tram %i km/h, monorail %i km/h\nmaglev %i km/h, narrowgauge %i km/h."),
		(welt->get_average_speed(tram_wt)*speed_ratio)/100,
		(welt->get_average_speed(monorail_wt)*speed_ratio)/100,
		(welt->get_average_speed(maglev_wt)*speed_ratio)/100,
		(welt->get_average_speed(narrowgauge_wt)*speed_ratio)/100
	);

	display_multiline_text( pos.x+11, pos.y+D_BUTTON_HEIGHT+4+12+5*LINESPACE, speed_message, SYSCOL_TEXT_HIGHLIGHT);

	speed_message.clear();
	speed_message.printf(translator::translate("\n100 km/h = %i tiles/month"),
		welt->speed_to_tiles_per_month(kmh_to_speed(100))
	);
	display_multiline_text(pos.x+11, pos.y+D_BUTTON_HEIGHT+4+12+6*LINESPACE, speed_message, SYSCOL_TEXT_HIGHLIGHT);
}
