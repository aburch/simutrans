/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
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

#include "../bauer/warenbauer.h"
#include "../besch/ware_besch.h"

#include "../dataobj/translator.h"
#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "gui_frame.h"


karte_t *goods_stats_t::welt = NULL;

goods_stats_t::goods_stats_t( karte_t *wl )
{
	welt = wl;
	set_groesse( koord(BUTTON4_X + D_BUTTON_WIDTH + 2, warenbauer_t::get_waren_anzahl() * (LINESPACE+1) ) );
}


void goods_stats_t::update_goodslist( uint16 *g, int b, int l )
{
	goodslist = g;
	bonus = b;
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

	for(  uint16 i=0;  i<listed_goods;  i++  ) {
		const ware_besch_t * wtyp = warenbauer_t::get_info(goodslist[i]);

		display_ddd_box_clip(offset.x + 2, yoff, 8, 8, MN_GREY0, MN_GREY4);
		display_fillbox_wh_clip(offset.x + 3, yoff+1, 6, 6, wtyp->get_color(), true);

		buf.clear();
		buf.printf("%s", translator::translate(wtyp->get_name()));
		display_proportional_clip(offset.x + 14, yoff,	buf, ALIGN_LEFT, COL_BLACK, true);

		// prissi
		const sint32 grundwert128 = wtyp->get_preis() * welt->get_settings().get_bonus_basefactor();	// bonus price will be always at least this
		const sint32 grundwert_bonus = wtyp->get_preis()*(1000l+(bonus-100l)*wtyp->get_speed_bonus());
		const sint32 price = (grundwert128>grundwert_bonus ? grundwert128 : grundwert_bonus);
		money_to_string( money_buf, price/300000.0 );
		display_proportional_clip(offset.x + 130, yoff, money_buf, 	ALIGN_RIGHT, 	COL_BLACK, true);

		buf.clear();
		buf.printf("%d%%", wtyp->get_speed_bonus());
		display_proportional_clip(offset.x + 155, yoff, buf, ALIGN_RIGHT, COL_BLACK, true);

		buf.clear();
		buf.printf("%s", translator::translate(wtyp->get_catg_name()));
		display_proportional_clip(offset.x + 165, yoff, buf, 	ALIGN_LEFT, COL_BLACK, 	true);

		buf.clear();
		buf.printf("%dKg", wtyp->get_weight_per_unit());
		display_proportional_clip(offset.x + 310, yoff, buf, ALIGN_RIGHT, COL_BLACK, true);

		yoff += LINESPACE+1;
	}
}
