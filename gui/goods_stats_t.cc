/*
 * goods_stats_t.cc
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "goods_stats_t.h"

#include "../simgraph.h"
#include "../simcolor.h"
#include "../simworld.h"

#include "../bauer/warenbauer.h"
#include "../besch/ware_besch.h"

#include "../boden/wege/weg.h"

#include "../dataobj/translator.h"
#include "components/list_button.h"


goods_stats_t::goods_stats_t(karte_t *welt)
{
	this->welt = welt;
	setze_groesse(koord(BUTTON4_X+BUTTON_WIDTH+2,(warenbauer_t::gib_waren_anzahl()-1)*(LINESPACE+1)));
}


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void goods_stats_t::zeichnen(koord offset) const
{
	int yoff = offset.y;
	char buf[256];

	for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl()-1; i++) {
		const ware_besch_t * wtyp = warenbauer_t::gib_info(goodslist[i]);

		display_ddd_box_clip(offset.x + 2, yoff, 8, 8, MN_GREY0, MN_GREY4);
		display_fillbox_wh_clip(offset.x + 3, yoff+1, 6, 6, 255 - (goodslist[i]-1)*4, true);

		sprintf(buf, "%s", translator::translate(wtyp->gib_name()));
		display_proportional_clip(offset.x + 14, yoff,	buf, ALIGN_LEFT, COL_BLACK, true);

		// prissi
		const sint32 grundwert128 = wtyp->gib_preis()<<7;
		const sint32 grundwert_bonus = wtyp->gib_preis()*(1000l+(bonus-100l)*wtyp->gib_speed_bonus());
		const sint32 price = (grundwert128>grundwert_bonus ? grundwert128 : grundwert_bonus);
		sprintf(buf, "%.2f$", price/300000.0);
		display_proportional_clip(offset.x + 130, yoff, buf, 	ALIGN_RIGHT, 	COL_BLACK, true);

		sprintf(buf, "%d%%", wtyp->gib_speed_bonus());
		display_proportional_clip(offset.x + 155, yoff, buf, ALIGN_RIGHT, COL_BLACK, true);

		sprintf(buf, "%s",	translator::translate(wtyp->gib_catg_name()));
		display_proportional_clip(offset.x + 165, yoff, buf, 	ALIGN_LEFT, COL_BLACK, 	true);

		sprintf(buf, "%dKg", wtyp->gib_weight_per_unit());
		display_proportional_clip(offset.x + 310, yoff, buf, ALIGN_RIGHT, COL_BLACK, true);

		yoff += LINESPACE+1;
	}
}
