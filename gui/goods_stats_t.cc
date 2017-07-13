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

#include "../bauer/goods_manager.h"
#include "../descriptor/goods_desc.h"

#include "../dataobj/translator.h"
#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "../descriptor/goods_desc.h"
#include "gui_frame.h"
#include "../gui/components/gui_button.h"



goods_stats_t::goods_stats_t()
{
	set_size( scr_size(BUTTON4_X + D_BUTTON_WIDTH + 2, goods_manager_t::get_count() * (LINESPACE+1) ) );
}


void goods_stats_t::update_goodslist( uint16 *g, uint32 b, int l, uint32 d, uint8 c, uint8 ct, uint8 gc)
{
	goodslist = g;
	vehicle_speed = b;
	distance_meters = d;
	comfort = c;
	catering_level = ct;
	g_class = gc;
	listd_goods = l;
	set_size( scr_size(BUTTON4_X + D_BUTTON_WIDTH + 2, listd_goods * (LINESPACE+1) ) );
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
	/*for(  uint16 i=0;  i<goods_manager_t::get_count()-1u;  i++  )*/

	for(  uint16 i=0;  i<listd_goods;  i++  )
	{
		const goods_desc_t * wtyp = goods_manager_t::get_info(goodslist[i]);

		display_ddd_box_clip(offset.x + 2, yoff, 8, 8, MN_GREY0, MN_GREY4);
		display_fillbox_wh_clip(offset.x + 3, yoff+1, 6, 6, wtyp->get_color(), true);

		buf.clear();
		buf.printf("%s", translator::translate(wtyp->get_name()));
		display_proportional_clip(offset.x + 14, yoff,	buf, ALIGN_LEFT, SYSCOL_TEXT, true);

		// Massively cleaned up by neroden, June 2013
		// Roundoff is deliberate here (get two-digit speed)... question this
		if (vehicle_speed <= 0) {
			// Negative and zero speeds will be due to roundoff errors
			vehicle_speed = 1;
		}
		const sint64 journey_tenths = tenths_from_meters_and_kmh(distance_meters, vehicle_speed);

		sint64 revenue = wtyp->get_total_fare(distance_meters, 0u, comfort, catering_level, min(g_class, wtyp->get_number_of_classes() - 1), journey_tenths);

		// Convert to simucents.  Should be very fast.
		sint64 price = (revenue + 2048) / 4096;

		money_to_string( money_buf, (double)price/100.0 );
		buf.clear();
		buf.printf(money_buf);
		display_proportional_clip(offset.x + 170, yoff, buf, 	ALIGN_RIGHT, 	SYSCOL_TEXT, true);

		buf.clear();
		buf.printf( "%s",	translator::translate(wtyp->get_catg_name()));
		display_proportional_clip(offset.x + 220, yoff, buf, 	ALIGN_LEFT, SYSCOL_TEXT, 	true);

		buf.clear();
		buf.printf("%dKg", wtyp->get_weight_per_unit());
		display_proportional_clip(offset.x + 360, yoff, buf, ALIGN_RIGHT, SYSCOL_TEXT, true);

		yoff += LINESPACE+1;
	}
}
