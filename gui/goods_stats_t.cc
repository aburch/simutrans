/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 * Copyright 2013 Nathanael Nerode, James Petts
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Display information about each configured good
 * as a list like display
 * @author Hj. Malthaner
 */

#include "goods_stats_t.h"

#include "../simgraph.h"
#include "../simcolor.h"
#include "../simworld.h"
#include "../simconvoi.h"

#include "../bauer/warenbauer.h"
#include "../besch/ware_besch.h"

#include "../dataobj/translator.h"
#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "../besch/ware_besch.h"
#include "gui_frame.h"


karte_t *goods_stats_t::welt = NULL;

goods_stats_t::goods_stats_t( karte_t *wl )
{
	welt = wl;
	set_groesse( koord(BUTTON4_X + D_BUTTON_WIDTH + 2, warenbauer_t::get_waren_anzahl() * (LINESPACE+1) ) );
}


void goods_stats_t::update_goodslist( uint16 *g, int b, int l, uint16 d, uint8 c, uint8 ct, waytype_t wt)
{
	goodslist = g;
	relative_speed_percentage = b;
	distance = d;
	comfort = c;
	catering_level = ct;
	way_type = wt;
	listed_goods = l;
	set_groesse( koord(BUTTON4_X + D_BUTTON_WIDTH + 2, listed_goods * (LINESPACE+1) ) );
}


/**
 * Draw the component
 * @author Hj. Malthaner
 */
void goods_stats_t::zeichnen(koord offset)
{
	int yoff = offset.y;
	char money_buf[256];
	cbuffer_t buf;

	// Pre-111.1 in case current does not work.
	/*for(  uint16 i=0;  i<warenbauer_t::get_waren_anzahl()-1u;  i++  )*/

	for(  uint16 i=0;  i<listed_goods;  i++  ) 
	{
		const ware_besch_t * wtyp = warenbauer_t::get_info(goodslist[i]);

		display_ddd_box_clip(offset.x + 2, yoff, 8, 8, MN_GREY0, MN_GREY4);
		display_fillbox_wh_clip(offset.x + 3, yoff+1, 6, 6, wtyp->get_color(), true);

		buf.clear();
		buf.printf("%s", translator::translate(wtyp->get_name()));
		display_proportional_clip(offset.x + 14, yoff,	buf, ALIGN_LEFT, COL_BLACK, true);

		// Massively cleaned up by neroden, June 2013
		sint64 revenue = wtyp->get_fare_with_speedbonus(welt, relative_speed_percentage, distance);
		sint64 price = revenue;

		sint64 relevant_speed = ( welt->get_average_speed(way_type) * (relative_speed_percentage + 100) ) / 100;
		// Roundoff is deliberate here (get two-digit speed)... question this
		if (relevant_speed <= 0) {
			// Negative and zero speeds will be due to roundoff errors
			relevant_speed = 1;
		}
		sint64 distance_meters = (sint64)distance * welt->get_settings().get_meters_per_tile();
		const uint16 journey_tenths = (uint16) tenths_from_meters_and_kmh(distance_meters, relevant_speed);
		const uint16 journey_minutes = (uint16) minutes_from_meters_and_kmh(distance_meters, relevant_speed);

		if(wtyp->get_catg_index() < 1)
		{
			//Passengers care about their comfort
			const uint8 tolerable_comfort = convoi_t::calc_tolerable_comfort(journey_minutes, welt);

			// Comfort matters more the longer the journey.
			// @author: jamespetts, March 2010
			sint64 comfort_modifier;
			if(journey_minutes <= welt->get_settings().get_tolerable_comfort_short_minutes())
			{
				comfort_modifier = 20ll;
			}
			else if(journey_minutes >= welt->get_settings().get_tolerable_comfort_median_long_minutes())
			{
				comfort_modifier = 100ll;
			}
			else
			{
				const uint16 differential = journey_minutes - welt->get_settings().get_tolerable_comfort_short_minutes();
				const uint16 max_differential = welt->get_settings().get_tolerable_comfort_median_long_minutes() -welt->get_settings().get_tolerable_comfort_short_minutes();
				const uint32 proportion = differential * 100 / max_differential;
				comfort_modifier = (80ll * (sint64)proportion / 100ll) + 20ll;
			}

			if(comfort > tolerable_comfort)
			{
				// Apply luxury bonus
				const uint8 max_differential = welt->get_settings().get_max_luxury_bonus_differential();
				const uint8 differential = comfort - tolerable_comfort;
				const sint64 multiplier = ((sint64)welt->get_settings().get_max_luxury_bonus_percent() * comfort_modifier) / 100ll;
				if(differential >= max_differential)
				{
					price += (revenue * (sint64)multiplier) / 100ll;
				}
				else
				{
					const sint64 proportion = ((sint64)differential * 100ll) / (sint64)max_differential;
					price += (revenue * (sint64)(multiplier * proportion)) / 10000ll;
				}
			}
			else if(comfort < tolerable_comfort)
			{
				// Apply discomfort penalty
				const uint8 max_differential = welt->get_settings().get_max_discomfort_penalty_differential();
				const uint8 differential = tolerable_comfort - comfort;
				sint64 multiplier = ((sint64)welt->get_settings().get_max_discomfort_penalty_percent() * comfort_modifier) / 100ll;
				multiplier = multiplier < 95ll ? multiplier : 95ll;
				if(differential >= max_differential)
				{
					price -= (revenue * (sint64)multiplier) / 100ll;
				}
				else
				{
					const sint64 proportion = ((sint64)differential * 100ll) / (sint64)max_differential;
					price -= (revenue * (multiplier * proportion)) / 10000ll;
				}
			}
		
			// Do nothing if comfort == tolerable_comfort			
		}

		// Add catering or TPO revenue
		if(catering_level > 0)
		{
			if(wtyp->get_catg_index() == 1)
			{
				// Mail
				if(journey_minutes >=welt->get_settings().get_tpo_min_minutes())
				{
					price += (sint64)(welt->get_settings().get_tpo_revenue() * 1000);
				}
			}
			else if(wtyp->get_catg_index() == 0)
			{
				// Passengers
				if (catering_level > 5) {
					// This isn't supposed to happen
					catering_level = 5;
				}
				catering_table_t& catering_revenue = welt->get_settings().catering_revenues[catering_level];
				price += catering_revenue(journey_tenths);
				sint64 proportion = 0;
			}
		}
	
		money_to_string( money_buf, (double)price/300000.0 );
		buf.clear();
		buf.printf(money_buf);
		display_proportional_clip(offset.x + 170, yoff, buf, 	ALIGN_RIGHT, 	COL_BLACK, true);

		buf.clear();
		buf.printf("%d%%", wtyp->get_speed_bonus());
		display_proportional_clip(offset.x + 205, yoff, buf, ALIGN_RIGHT, COL_BLACK, true);

		buf.clear();
		buf.printf( "%s",	translator::translate(wtyp->get_catg_name()));
		display_proportional_clip(offset.x + 220, yoff, buf, 	ALIGN_LEFT, COL_BLACK, 	true);

		buf.clear();
		buf.printf("%dKg", wtyp->get_weight_per_unit());
		display_proportional_clip(offset.x + 360, yoff, buf, ALIGN_RIGHT, COL_BLACK, true);

		yoff += LINESPACE+1;
	}
}
