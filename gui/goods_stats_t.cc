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

#include "../display/simgraph.h"
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
#include "../gui/components/gui_button.h"



goods_stats_t::goods_stats_t()
{
	set_size( scr_size(BUTTON4_X + D_BUTTON_WIDTH + 2, warenbauer_t::get_waren_anzahl() * (LINESPACE+1) ) );
}


void goods_stats_t::update_goodslist( uint16 *g, int b, int l, uint32 d, uint8 c, uint8 ct, waytype_t wt)
{
	goodslist = g;
	relative_speed_percentage = b;
	distance_meters = d;
	comfort = c;
	catering_level = ct;
	way_type = wt;
	listed_goods = l;
	set_size( scr_size(BUTTON4_X + D_BUTTON_WIDTH + 2, listed_goods * (LINESPACE+1) ) );
}


/**
 * Draw the component
 * @author Hj. Malthaner
 */
void goods_stats_t::draw(scr_coord offset)
{
	scr_coord_val yoff = offset.y;
	char money_buf[256];
	cbuffer_t buf;
	offset.x += pos.x;

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
		sint64 relevant_speed = ( welt->get_average_speed(way_type) * (relative_speed_percentage + 100) ) / 100;
		// Roundoff is deliberate here (get two-digit speed)... question this
		if (relevant_speed <= 0) {
			// Negative and zero speeds will be due to roundoff errors
			relevant_speed = 1;
		}
		const sint64 journey_tenths = tenths_from_meters_and_kmh(distance_meters, relevant_speed);

		sint64 revenue = wtyp->get_fare_with_comfort_catering_speedbonus(welt,
				comfort, catering_level, journey_tenths, relative_speed_percentage, distance_meters);
		// Convert to simcents.  Should be very fast.
		sint64 price = (revenue + 2048) / 4096;

		money_to_string( money_buf, (double)price/100.0 );
		buf.clear();
		buf.printf(money_buf);
		display_proportional_clip(offset.x + 170, yoff, buf, 	ALIGN_RIGHT, 	COL_BLACK, true);

		buf.clear();
		buf.printf("%d%%", wtyp->get_adjusted_speed_bonus(distance_meters));
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
