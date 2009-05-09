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

#include "../bauer/warenbauer.h"
#include "../besch/ware_besch.h"

#include "../dataobj/translator.h"
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
	char buf[256];

	for(  uint16 i=0;  i<warenbauer_t::get_waren_anzahl()-1u;  i++  ) 
	{
		const ware_besch_t * wtyp = warenbauer_t::get_info(goodslist[i]);

		display_ddd_box_clip(offset.x + 2, yoff, 8, 8, MN_GREY0, MN_GREY4);
		display_fillbox_wh_clip(offset.x + 3, yoff+1, 6, 6, wtyp->get_color(), true);

		sprintf(buf, "%s", translator::translate(wtyp->get_name()));
		display_proportional_clip(offset.x + 14, yoff,	buf, ALIGN_LEFT, COL_BLACK, true);

		// prissi
		// Modified by jamespetts 18 Apr. 2009

		const uint16 base_price = wtyp->get_preis();
		const sint32 min_price = base_price << 7;
		const uint16 speed_bonus_rating = convoi_t::calc_adjusted_speed_bonus(wtyp->get_speed_bonus(), distance, welt);
		const sint32 base_bonus = base_price * (1000l + (bonus - 100l) * speed_bonus_rating);
		const sint32 revenue = (min_price > base_bonus ? min_price : base_bonus) * distance;
		sint32 price = revenue;

		const uint16 journey_minutes = (((float)distance / (float)welt->get_average_speed(way_type)*bonus)/100) * welt->get_einstellungen()->get_journey_time_multiplier() * 60;

		if(wtyp->get_catg_index() < 1)
		{
			//Passengers care about their comfort
			const uint8 tolerable_comfort = convoi_t::calc_tolerable_comfort(journey_minutes, welt);

			if(comfort > tolerable_comfort)
			{
				// Apply luxury bonus
				const uint8 max_differential = welt->get_einstellungen()->get_max_luxury_bonus_differential();
				const uint8 differential = comfort - tolerable_comfort;
				const float multiplier = welt->get_einstellungen()->get_max_luxury_bonus();
				if(differential >= max_differential)
				{
					price += (revenue * multiplier);
				}
				else
				{
					const float proportion = (float)differential / (float)max_differential;
					price += revenue * (multiplier * proportion);
				}
			}
			else if(comfort < tolerable_comfort)
			{
				// Apply discomfort penalty
				const uint8 max_differential = welt->get_einstellungen()->get_max_discomfort_penalty_differential();
				const uint8 differential = tolerable_comfort - comfort;
				const float multiplier = welt->get_einstellungen()->get_max_discomfort_penalty();
				if(differential >= max_differential)
				{
					price -= (revenue * multiplier);
				}
				else
				{
					const float proportion = (float)differential / (float)max_differential;
					price -= revenue * (multiplier * proportion);
				}
			}	
			// Do nothing if comfort == tolerable_comfort			
		}
	
		sprintf(buf, "%.2f$", price/300000.0);
		display_proportional_clip(offset.x + 170, yoff, buf, 	ALIGN_RIGHT, 	COL_BLACK, true);

		sprintf(buf, "%d%%", wtyp->get_speed_bonus());
		display_proportional_clip(offset.x + 205, yoff, buf, ALIGN_RIGHT, COL_BLACK, true);

		sprintf(buf, "%s",	translator::translate(wtyp->get_catg_name()));
		display_proportional_clip(offset.x + 220, yoff, buf, 	ALIGN_LEFT, COL_BLACK, 	true);

		sprintf(buf, "%dKg", wtyp->get_weight_per_unit());
		display_proportional_clip(offset.x + 360, yoff, buf, ALIGN_RIGHT, COL_BLACK, true);

		yoff += LINESPACE+1;
	}
}
