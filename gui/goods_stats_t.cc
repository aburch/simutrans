/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
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
#include "components/list_button.h"


goods_stats_t::goods_stats_t()
{
	set_groesse(koord(BUTTON4_X+BUTTON_WIDTH+2,-10+(warenbauer_t::get_waren_anzahl()-1)*(LINESPACE+1)));
}


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void goods_stats_t::zeichnen(koord offset)
{
	int yoff = offset.y;
	char money_buf[256];
	cbuffer_t buf;

	for(  uint16 i=0;  i<warenbauer_t::get_waren_anzahl()-1u;  i++  ) 
	{
		const ware_besch_t * wtyp = warenbauer_t::get_info(goodslist[i]);

		display_ddd_box_clip(offset.x + 2, yoff, 8, 8, MN_GREY0, MN_GREY4);
		display_fillbox_wh_clip(offset.x + 3, yoff+1, 6, 6, wtyp->get_color(), true);

		buf.clear();
		buf.printf("%s", translator::translate(wtyp->get_name()));
		display_proportional_clip(offset.x + 14, yoff,	buf, ALIGN_LEFT, COL_BLACK, true);

		// prissi
		// Modified by jamespetts 18 Apr. 2009

		const sint64 base_price = (sint64)wtyp->get_preis();
		const sint64 min_price = base_price / 10ll;
		const sint64 speed_bonus_rating = (sint64)convoi_t::calc_adjusted_speed_bonus(wtyp->get_speed_bonus(), distance, welt);
		const sint64 base_bonus = base_price * (1000ll + ((sint64)bonus - 100ll) * speed_bonus_rating);
		const sint64 revenue = (min_price > base_bonus ? min_price : base_bonus) * (sint64)distance;
		sint64 price = revenue;

		//const uint16 journey_minutes = ((float)distance / (((float)welt->get_average_speed(way_type) * bonus) / 100)) *welt->get_settings().get_meters_per_tile() * 6;
		const uint16 journey_minutes = (((distance * 100) / welt->get_average_speed(way_type)) * welt->get_settings().get_meters_per_tile()) / 1667;

		if(wtyp->get_catg_index() < 1)
		{
			//Passengers care about their comfort
			const uint8 tolerable_comfort = convoi_t::calc_tolerable_comfort(journey_minutes, welt);

			// Comfort matters more the longer the journey.
			// @author: jamespetts, March 2010
			sint64 comfort_modifier;
			if(journey_minutes <=welt->get_settings().get_tolerable_comfort_short_minutes())
			{
				comfort_modifier = 20ll;
			}
			else if(journey_minutes >=welt->get_settings().get_tolerable_comfort_median_long_minutes())
			{
				comfort_modifier = 100ll;
			}
			else
			{
				const uint8 differential = journey_minutes -welt->get_settings().get_tolerable_comfort_short_minutes();
				const uint8 max_differential = welt->get_settings().get_tolerable_comfort_median_long_minutes() -welt->get_settings().get_tolerable_comfort_short_minutes();
				const uint32 proportion = differential * 100ll / max_differential;
				comfort_modifier = (80ll * proportion / 100ll) + 20ll;
			}

			if(comfort > tolerable_comfort)
			{
				// Apply luxury bonus
				const uint8 max_differential = welt->get_settings().get_max_luxury_bonus_differential();
				const uint8 differential = comfort - tolerable_comfort;
				const sint64 multiplier = (welt->get_settings().get_max_luxury_bonus_percent() * comfort_modifier) / 100ll;
				if(differential >= max_differential)
				{
					price += (revenue * multiplier) / 10000ll;
				}
				else
				{
					const sint64 proportion = (differential * 100) / max_differential;
					price += (revenue * (sint64)(multiplier * proportion)) / 10000ll;
				}
			}
			else if(comfort < tolerable_comfort)
			{
				// Apply discomfort penalty
				const uint8 max_differential = welt->get_settings().get_max_discomfort_penalty_differential();
				const uint8 differential = tolerable_comfort - comfort;
				sint64 multiplier = (welt->get_settings().get_max_discomfort_penalty_percent() * comfort_modifier) / 100ll;
				multiplier = multiplier < 95 ? multiplier : 95;
				if(differential >= max_differential)
				{
					price -= (revenue * multiplier) / 10000ll;
				}
				else
				{
					const sint64 proportion = (differential * 100) / max_differential;
					price -= (revenue * (multiplier * proportion)) / 10000ll;
				}
			}
		
			// Do nothing if comfort == tolerable_comfort			
		}
	
		money_to_string( money_buf, price/300000.0 );
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
