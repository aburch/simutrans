/*
 * gui_convoiinfo.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifdef _MSC_VER
#include <string.h>
#endif

#include <stdio.h>

#include "gui_convoiinfo.h"
#include "../simdebug.h"
#include "../simvehikel.h"
#include "../simconvoi.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simplay.h"
#include "../simevent.h"
#include "../simlinemgmt.h"
#include "../simline.h"

#include "../dataobj/translator.h"

#include "../utils/simstring.h"


gui_convoiinfo_t::gui_convoiinfo_t(convoihandle_t cnv, int n)
{
    this->cnv = cnv;
    nummer = n;

    filled_bar.setze_pos(koord(188, 33));
    filled_bar.setze_groesse(koord(100, 4));
    filled_bar.add_color_value(&cnv->get_loading_limit(), GELB);
    filled_bar.add_color_value(&cnv->get_loading_level(), GREEN);
    add_komponente(&filled_bar);
}

/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void gui_convoiinfo_t::infowin_event(const event_t *ev)
{
    if(IS_LEFTRELEASE(ev) && cnv.is_bound()) {
	cnv->zeige_info();
    }
}


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_convoiinfo_t::zeichnen(koord offset) const
{
    clip_dimension clip = display_gib_clip_wh();

    if(! ((pos.y+offset.y) > clip.yy ||  (pos.y+offset.y) < clip.y-32) &&
	cnv.is_bound()) {

	gui_container_t::zeichnen(pos + offset);

	char buf[128];

	sprintf(buf, "%d.", nummer);


	display_proportional_clip(pos.x+offset.x+4, pos.y+offset.y+8,
                                  buf,
				  ALIGN_LEFT, SCHWARZ, true);

	const int n = sprintf(buf, "%s ", translator::translate("Gewinn"));
	money_to_string(buf+n, cnv->gib_jahresgewinn()/100);


	display_proportional_clip(pos.x+offset.x+30,
				  pos.y+offset.y+8+LINESPACE,
				  buf,
				  ALIGN_LEFT, SCHWARZ, true);

	/*
	 * only show assigned line, if there is one!
	 */
	tstrncpy(buf, "", 1);
	if (cnv->in_depot())
	{
		sprintf(buf, "%s", translator::translate("(in depot)"));
	} else if (cnv->get_line() != NULL)
	{
		sprintf(buf, "%s: %s", translator::translate("Line"), cnv->get_line()->get_name());
		// cut off too long line names, so they don't overwrite the load-bar
		tstrncpy(buf, buf, 24);
	}

    display_proportional_clip(pos.x+offset.x+30, pos.y+offset.y+8+2*LINESPACE,
                              buf,
			  ALIGN_LEFT, SCHWARZ, true);


	tstrncpy(buf, translator::translate(cnv->gib_name()), 128);

        display_proportional_clip(pos.x+offset.x+30, pos.y+offset.y+8,
                                  buf,
				  ALIGN_LEFT, SCHWARZ, true);



	const int xoff = MAX(168, proportional_string_width(buf) + 14);
	const int yoff =
	  get_tile_raster_width() == 64 ? -26 : -(128 >> get_zoom_factor());

        for(int i=0; i<cnv->gib_vehikel_anzahl();i++) {
				display_color_img(cnv->gib_vehikel(i)->gib_basis_bild(),
	                      pos.x+offset.x+xoff+i*16,
			      pos.y+offset.y+yoff,
			      cnv->gib_besitzer()->kennfarbe,
			      false,
			      true);
        }
    }
}
