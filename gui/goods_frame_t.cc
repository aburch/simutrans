/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "goods_frame_t.h"
#include "components/gui_scrollpane.h"
#include "components/list_button.h"

#include "../bauer/warenbauer.h"
#include "../besch/ware_besch.h"
#include "../dataobj/translator.h"

#include "../simcolor.h"
#include "../simworld.h"

/**
 * This variable defines the current speed for bonus calculation
 * @author prissi
 */
int goods_frame_t::relative_speed_change = 100;

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

uint16 goods_frame_t::distance = 1;
uint8 goods_frame_t::comfort = 50;

const char *goods_frame_t::sort_text[SORT_MODES] = 
{
    "gl_btn_unsort",
    "gl_btn_sort_name",
    "gl_btn_sort_revenue",
    "gl_btn_sort_bonus",
    "gl_btn_sort_catg"
};



goods_frame_t::goods_frame_t(karte_t *wl) :
	gui_frame_t("gl_title"),
	sort_label(translator::translate("hl_txt_sort")),
	change_speed_label(speed_bonus,COL_WHITE,gui_label_t::right),
	change_distance_label(distance_txt,COL_WHITE,gui_label_t::right),
	change_comfort_label(comfort_txt,COL_WHITE,gui_label_t::right),
	scrolly(&goods_stats)
{
	//static karte_t* welt = wl;
	this->goods_frame_t::welt = wl;
	int y=BUTTON_HEIGHT+4-16;

	speed_bonus[0] = 0;
	distance_txt[0] = 0;
	comfort_txt[0] = 0;
	change_speed_label.set_pos(koord(BUTTON4_X+5, y + 24));
	add_komponente(&change_speed_label);

	change_distance_label.set_pos(koord(BUTTON4_X+5, y));
	add_komponente(&change_distance_label);

	change_comfort_label.set_pos(koord(BUTTON4_X+5, y + 12));
	add_komponente(&change_comfort_label);

	speed_down.init(button_t::repeatarrowleft, "", koord(BUTTON4_X-22, y + 24), koord(10,BUTTON_HEIGHT));
	speed_down.add_listener(this);
	add_komponente(&speed_down);

	speed_up.init(button_t::repeatarrowright, "", koord(BUTTON4_X+8, y + 24), koord(10,BUTTON_HEIGHT));
	speed_up.add_listener(this);
	add_komponente(&speed_up);

	distance_down.init(button_t::repeatarrowleft, "", koord(BUTTON4_X-22, y), koord(10,BUTTON_HEIGHT));
	distance_down.add_listener(this);
	add_komponente(&distance_down);

	distance_up.init(button_t::repeatarrowright, "", koord(BUTTON4_X+8, y), koord(10,BUTTON_HEIGHT));
	distance_up.add_listener(this);
	add_komponente(&distance_up);

	comfort_down.init(button_t::repeatarrowleft, "", koord(BUTTON4_X-22, y + 12), koord(10,BUTTON_HEIGHT));
	comfort_down.add_listener(this);
	add_komponente(&comfort_down);

	comfort_up.init(button_t::repeatarrowright, "", koord(BUTTON4_X+8, y + 12), koord(10,BUTTON_HEIGHT));
	comfort_up.add_listener(this);
	add_komponente(&comfort_up);
	
	y=4+5*LINESPACE+6 + 25;	

	sort_label.set_pos(koord(BUTTON1_X, y));
	add_komponente(&sort_label);

	y += LINESPACE;

	sortedby.init(button_t::roundbox, "", koord(BUTTON1_X, y), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sortedby.add_listener(this);
	add_komponente(&sortedby);

	sorteddir.init(button_t::roundbox, "", koord(BUTTON2_X, y), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sorteddir.add_listener(this);
	add_komponente(&sorteddir);

	y += BUTTON_HEIGHT+2;

	scrolly.set_pos(koord(1, y));
	scrolly.set_show_scroll_x(false);
	scrolly.set_groesse(koord(TOTAL_WIDTH-16+25, 191+16+16-y));
	add_komponente(&scrolly);

	int h = (warenbauer_t::get_waren_anzahl()+3)*LINESPACE+y;
	if(h>450) {
		h = y+10*LINESPACE+2;
	}
	set_fenstergroesse(koord(TOTAL_WIDTH, h));
	set_min_windowsize(koord(TOTAL_WIDTH,y+6*LINESPACE+2));
	set_resizemode(vertical_resize);

	sort_list();

	resize (koord(0,0));
}




/**
* @author Markus Weber
*/
int goods_frame_t::compare_goods(const void *p1, const void *p2)
{
	const ware_besch_t* w[2];
	w[0] = warenbauer_t::get_info(*(const unsigned short *)p1);
	w[1] = warenbauer_t::get_info(*(const unsigned short *)p2);

	int order = 0;

	switch (sortby) 
	{
		case 0: // sort by number
			order = *(const unsigned short *)p1 - *(const unsigned short *)p2;
			break;
		case 2: // sort by revenue
			{			
				sint32 price[2];
				for(uint8 i = 0; i < 2; i ++)
				{
					const uint16 base_price = w[i]->get_preis();
					const sint32 min_price = base_price << 7;
					const uint16 speed_bonus_rating = convoi_t::calc_adjusted_speed_bonus(w[i]->get_speed_bonus(), distance, NULL);
					const sint32 base_bonus = base_price * (1000l + (relative_speed_change - 100l) * speed_bonus_rating);
					const sint32 revenue = (min_price > base_bonus ? min_price : base_bonus) * distance;
					price[i] = revenue;

					const uint16 journey_minutes = ((float)distance / (float)((50 * relative_speed_change)/100)) * 0.3 * 60;

					if(w[i]->get_catg_index() < 1)
					{
						//Passengers care about their comfort
						const uint8 tolerable_comfort = 65;

						if(comfort > tolerable_comfort)
						{
							// Apply luxury bonus
							const uint8 max_differential = 55;
							const uint8 differential = comfort - tolerable_comfort;
							const float multiplier = 4;
							if(differential >= max_differential)
							{
								price[i] += (revenue * multiplier);
							}
							else
							{
								const float proportion = (float)differential / (float)max_differential;
								price[i] += revenue * (multiplier * proportion);
							}
						}
						else if(comfort < tolerable_comfort)
						{
							// Apply discomfort penalty
							const uint8 max_differential = 200;
							const uint8 differential = tolerable_comfort - comfort;
							const float multiplier = 9.5;
							if(differential >= max_differential)
							{
								price[i] -= (revenue * multiplier);
							}
							else
							{
								const float proportion = (float)differential / (float)max_differential;
								price[i] -= revenue * (multiplier * proportion);
							}
						}	
						// Do nothing if comfort == tolerable_comfort			
					}
				}

				/*const sint32 grundwert1281 = w[0]->get_preis()<<7;
				const sint32 grundwert_bonus1 = w[0]->get_preis()*(1000l+(relative_speed_change-100l)*w[0]->get_speed_bonus());
				const sint32 price1 = (grundwert1281>grundwert_bonus1 ? grundwert1281 : grundwert_bonus1)  * 10000;
				const sint32 grundwert1282 = w[1]->get_preis()<<7;
				const sint32 grundwert_bonus2 = w[1]->get_preis()*(1000l+(relative_speed_change-100l)*w[1]->get_speed_bonus());
				const sint32 price2 = (grundwert1282>grundwert_bonus2 ? grundwert1282 : grundwert_bonus2) * 10000;
				order = price2-price1;*/

				order = price[0] - price[1];
			}
			break;
		case 3: // sort by speed bonus
			order = w[1]->get_speed_bonus()-w[0]->get_speed_bonus();
			break;
		case 4: // sort by catg_index
			order = w[1]->get_catg()-w[0]->get_catg();
			break;
	}
	if(  order==0  ) {
		// sort by name if not sorted or not unique
		order = strcmp(translator::translate(w[0]->get_name()), translator::translate(w[1]->get_name()));
	}
	return sortreverse ? -order : order;
}


// creates the list and pass it to the child finction good_stats, which does the display stuff ...
void goods_frame_t::sort_list()
{
	sortedby.set_text(sort_text[sortby]);
	sorteddir.set_text(sortreverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");

	int n=0;
	for(unsigned int i=0; i<warenbauer_t::get_waren_anzahl(); i++) {
		const ware_besch_t * wtyp = warenbauer_t::get_info(i);

		// Hajo: we skip goods that don't generate income
		//       this should only be true for the special good 'None'
		if(wtyp->get_preis()!=0) {
			good_list[n++] = i;
		}
	}

	// now sort
	qsort((void *)good_list, n, sizeof(unsigned short), compare_goods);

	goods_stats.update_goodslist(good_list, relative_speed_change, goods_frame_t::distance, goods_frame_t::comfort, goods_frame_t::welt);
}

/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void goods_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);
	koord groesse = get_fenstergroesse()-koord(0,4+6*LINESPACE+4+BUTTON_HEIGHT+2+16+25);
	scrolly.set_groesse(groesse);
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
	else if(komp == &distance_down) 
	{
		if(distance > 1) 
		{
			distance --;
			sort_list();
		}
	}
	else if(komp == &distance_up) 
	{
		distance ++;
		sort_list();
	}
else if(komp == &comfort_down) 
	{
		if(comfort > 1) 
		{
			comfort --;
			sort_list();
		}
	}
	else if(komp == &comfort_up) 
	{
		if(comfort < 255) 
		{
			comfort ++;
			sort_list();
		}
	}
	return true;
}



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void goods_frame_t::zeichnen(koord pos, koord gr)
{
	gui_frame_t::zeichnen(pos, gr);

	sprintf(speed_bonus,"%i",relative_speed_change-100);
	sprintf(distance_txt,"%i",distance);
	sprintf(comfort_txt,"%i",comfort);
	
	sprintf(speed_message,translator::translate("Distance\nComfort\nSpeedbonus\nroad %i km/h, rail %i km/h\nships %i km/h, planes %i km/h."),
		(goods_frame_t::welt->get_average_speed(road_wt)*relative_speed_change)/100,
		(goods_frame_t::welt->get_average_speed(track_wt)*relative_speed_change)/100,
		(goods_frame_t::welt->get_average_speed(water_wt)*relative_speed_change)/100,
		(goods_frame_t::welt->get_average_speed(air_wt)*relative_speed_change)/100
	);
	display_multiline_text(pos.x+11, pos.y+BUTTON_HEIGHT+4, speed_message, COL_WHITE);

	sprintf(speed_message,translator::translate("tram %i km/h, monorail %i km/h\nmaglev %i km/h, narrowgauge %i km/h."),
		(goods_frame_t::welt->get_average_speed(tram_wt)*relative_speed_change)/100,
		(goods_frame_t::welt->get_average_speed(monorail_wt)*relative_speed_change)/100,
		(goods_frame_t::welt->get_average_speed(maglev_wt)*relative_speed_change)/100,
		(goods_frame_t::welt->get_average_speed(narrowgauge_wt)*relative_speed_change)/100
	);
	display_multiline_text(pos.x+11, pos.y+BUTTON_HEIGHT+4+57, speed_message, COL_WHITE);

}
