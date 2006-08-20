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

#include "../bauer/warenbauer.h"
#include "../besch/ware_besch.h"

#include "../dataobj/translator.h"


goods_stats_t::goods_stats_t()
{
  setze_groesse(koord(210,
		      warenbauer_t::gib_waren_anzahl()*LINESPACE+48));
}


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void goods_stats_t::zeichnen(koord offset) const
{
  int yoff = offset.y+10;
  char buf[256];


  // Hajo: Headline, list follows below

  sprintf(buf, "%s:",
	  translator::translate("Fracht"));
  display_proportional_clip(offset.x + 20, yoff,
			    buf,
			    ALIGN_LEFT,
			    WEISS,
			    true);


  sprintf(buf, "%s:",
	  translator::translate("Preis"));
  display_proportional_clip(offset.x + 116, yoff,
			    buf,
			    ALIGN_LEFT,
			    WEISS,
			    true);

  sprintf(buf, "%s:",
	  translator::translate("Bonus"));
  display_proportional_clip(offset.x + 160, yoff,
			    buf,
			    ALIGN_LEFT,
			    WEISS,
			    true);


  sprintf(buf, "%s:",
	  translator::translate("Category"));
  display_proportional_clip(offset.x + 220, yoff,
			    buf,
			    ALIGN_LEFT,
			    WEISS,
			    true);

  sprintf(buf, "%s:",
	  translator::translate("Gewicht"));
  display_proportional_clip(offset.x + 308, yoff,
			    buf,
			    ALIGN_LEFT,
			    WEISS,
			    true);


  yoff += LINESPACE+5;


  // Hajo: now print the list


  for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
    const ware_besch_t * wtyp = warenbauer_t::gib_info(i);

    // Hajo: we skip goods that don't generate income
    //       this should only be true for the special good 'None'
    //       a strcmp() seems to be too wasteful here
    if(wtyp->gib_preis() != 0) {

      display_ddd_box_clip(offset.x + 20, yoff, 8, 8, MN_GREY0, MN_GREY4);
      display_fillbox_wh_clip(offset.x + 21, yoff+1, 6, 6, 255 - (i-1)*4, true);


      sprintf(buf, "%s",
	      translator::translate(wtyp->gib_name()));
      display_proportional_clip(offset.x + 32, yoff,
				buf,
				ALIGN_LEFT,
				SCHWARZ,
				true);


      sprintf(buf, "%.2f$",
	      wtyp->gib_preis()/100.0);
      display_proportional_clip(offset.x + 150, yoff,
				buf,
				ALIGN_RIGHT,
				SCHWARZ,
				true);

      sprintf(buf, "%d%%",
	      wtyp->gib_speed_bonus());
      display_proportional_clip(offset.x + 196, yoff,
				buf,
				ALIGN_RIGHT,
				SCHWARZ,
				true);


      sprintf(buf, "%s",
	      translator::translate(wtyp->gib_catg_name()));
      display_proportional_clip(offset.x + 220, yoff,
				buf,
				ALIGN_LEFT,
				SCHWARZ,
				true);

      sprintf(buf, "%dKg",
	      wtyp->gib_weight_per_unit());
      display_proportional_clip(offset.x + 356, yoff,
				buf,
				ALIGN_RIGHT,
				SCHWARZ,
				true);

      yoff += LINESPACE+1;
    }
  }
}
