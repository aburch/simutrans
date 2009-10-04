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
	char buf[256];

	for(  uint16 i=0;  i<warenbauer_t::get_waren_anzahl()-1u;  i++  ) {
		const ware_besch_t * wtyp = warenbauer_t::get_info(goodslist[i]);

		display_ddd_box_clip(offset.x + 2, yoff, 8, 8, MN_GREY0, MN_GREY4);
		display_fillbox_wh_clip(offset.x + 3, yoff+1, 6, 6, wtyp->get_color(), true);

		sprintf(buf, "%s", translator::translate(wtyp->get_name()));
		display_proportional_clip(offset.x + 14, yoff,	buf, ALIGN_LEFT, COL_BLACK, true);

		// prissi
		const sint32 grundwert128 = wtyp->get_preis()<<7;
		const sint32 grundwert_bonus = wtyp->get_preis()*(1000l+(bonus-100l)*wtyp->get_speed_bonus());
		const sint32 price = (grundwert128>grundwert_bonus ? grundwert128 : grundwert_bonus);
		money_to_string( buf, price/300000.0 );
		display_proportional_clip(offset.x + 130, yoff, buf, 	ALIGN_RIGHT, 	COL_BLACK, true);

		sprintf(buf, "%d%%", wtyp->get_speed_bonus());
		display_proportional_clip(offset.x + 155, yoff, buf, ALIGN_RIGHT, COL_BLACK, true);

		sprintf(buf, "%s",	translator::translate(wtyp->get_catg_name()));
		display_proportional_clip(offset.x + 165, yoff, buf, 	ALIGN_LEFT, COL_BLACK, 	true);

		sprintf(buf, "%dKg", wtyp->get_weight_per_unit());
		display_proportional_clip(offset.x + 310, yoff, buf, ALIGN_RIGHT, COL_BLACK, true);

		yoff += LINESPACE+1;
	}
}
