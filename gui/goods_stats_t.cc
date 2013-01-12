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

#include "../besch/ware_besch.h"

goods_stats_t::goods_stats_t()
{
	set_groesse(koord(BUTTON4_X+BUTTON_WIDTH+2,(warenbauer_t::get_waren_anzahl()-1)*(LINESPACE+1)));
}


void goods_stats_t::update_goodslist( uint16 *g, int b, int l, uint16 d, uint8 c, uint8 ct, karte_t* w, waytype_t wt)
{
	goodslist = g;
	bonus = b;
	distance = d;
	comfort = c;
	catering_level = ct;
	welt = w;
	way_type = wt;
	listed_goods = l;
	set_groesse(koord(BUTTON4_X+BUTTON_WIDTH+2,max(2,listed_goods-2)*(LINESPACE+1)));
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

		// prissi
		// Modified by jamespetts 18 Apr. 2009

		const sint64 base_fare = wtyp->get_fare(distance);
		const sint64 min_fare = (base_fare * 1000ll) / 4ll;
		const sint64 max_fare = base_fare * 4000ll;
		const sint64 speed_bonus_rating = (sint64)convoi_t::calc_adjusted_speed_bonus(wtyp->get_speed_bonus(), distance, welt);
		const sint64 base_bonus = base_fare * (1000ll + (((sint64)bonus - 100ll) * speed_bonus_rating));
		const sint64 revenue = min(max_fare, max(min_fare, base_bonus));
		sint64 price = revenue;


		//const uint16 journey_minutes = ((float)distance / (((float)welt->get_average_speed(way_type) * bonus) / 100)) *welt->get_settings().get_meters_per_tile() * 6;
		const uint16 divider = (welt->get_average_speed(way_type) * bonus) / 100;
		const uint16 journey_minutes = (((distance * 100) / (divider > 0 ? divider : 1)) * welt->get_settings().get_meters_per_tile()) / 1667;

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
				sint64 proportion = 0;
				// Knightly : Reorganised the switch cases to get rid of goto statements
				switch(catering_level)
				{

				default:
				case 5:
					if(journey_minutes >= welt->get_settings().get_catering_level4_minutes())
					{
						if(journey_minutes > welt->get_settings().get_catering_level5_minutes())
						{
							price += (welt->get_settings().get_catering_level5_max_revenue() * 1000);
							break;
						}
					
						proportion = (sint64)((journey_minutes - welt->get_settings().get_catering_level4_minutes()) * 1000) / (welt->get_settings().get_catering_level5_minutes() -welt->get_settings().get_catering_level4_minutes());
						price += max((sint64)(proportion * (welt->get_settings().get_catering_level5_max_revenue())), ((sint64)(welt->get_settings().get_catering_level4_max_revenue() * 1000) + 4000));
						break;
					}

				case 4:
					if(journey_minutes >= welt->get_settings().get_catering_level3_minutes())
					{
						if(journey_minutes > welt->get_settings().get_catering_level4_minutes())
						{
							price += (sint64)(welt->get_settings().get_catering_level4_max_revenue() * 1000);
							break;
						}
					
						proportion = ((journey_minutes -welt->get_settings().get_catering_level3_minutes()) * 1000) / (welt->get_settings().get_catering_level4_minutes() - welt->get_settings().get_catering_level3_minutes());
						price += max((sint64)(proportion * (welt->get_settings().get_catering_level4_max_revenue())), ((sint64)(welt->get_settings().get_catering_level3_max_revenue() * 1000) + 4000));
						break;
					}

				case 3:
					if(journey_minutes >= welt->get_settings().get_catering_level2_minutes())
					{
						if(journey_minutes > welt->get_settings().get_catering_level3_minutes())
						{
							price += (sint64)(welt->get_settings().get_catering_level3_max_revenue() * 1000);
							break;
						}
					
						proportion = ((journey_minutes - welt->get_settings().get_catering_level2_minutes()) * 1000) / (welt->get_settings().get_catering_level3_minutes() - welt->get_settings().get_catering_level2_minutes());
						price += max((sint64)((proportion * welt->get_settings().get_catering_level3_max_revenue())), ((sint64)(welt->get_settings().get_catering_level2_max_revenue() * 1000) + 4000));
						break;
					}

				case 2:
					if(journey_minutes >= welt->get_settings().get_catering_level1_minutes())
					{
						if(journey_minutes > welt->get_settings().get_catering_level2_minutes())
						{
							price += (sint64)(welt->get_settings().get_catering_level2_max_revenue() * 1000);
							break;
						}
					
						proportion = ((journey_minutes - welt->get_settings().get_catering_level1_minutes()) * 1000) / (welt->get_settings().get_catering_level2_minutes() - welt->get_settings().get_catering_level1_minutes());
						price +=  max((sint64)(proportion * (welt->get_settings().get_catering_level2_max_revenue())), ((sint64)(welt->get_settings().get_catering_level1_max_revenue() * 1000) + 4000));
						break;
					}

				case 1:
					if(journey_minutes < welt->get_settings().get_catering_min_minutes())
					{
						break;
					}
					if(journey_minutes > welt->get_settings().get_catering_level1_minutes())
					{
						price += (sint64)(welt->get_settings().get_catering_level1_max_revenue() * 1000);
						break;
					}

					proportion = ((journey_minutes - welt->get_settings().get_catering_min_minutes()) * 1000) / (welt->get_settings().get_catering_level1_minutes() - welt->get_settings().get_catering_min_minutes());
					price += (sint64)(proportion * (welt->get_settings().get_catering_level1_max_revenue()));
					break;

				};
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
