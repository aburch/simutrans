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
#include "../simworld.h"
#include "../simdepot.h"
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
    filled_bar.add_color_value(&cnv->get_loading_limit(), COL_YELLOW);
    filled_bar.add_color_value(&cnv->get_loading_level(), COL_GREEN);
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
    	if(cnv->in_depot()) {
    		cnv->gib_welt()->lookup(cnv->get_home_depot())->gib_depot()->zeige_info();
    }
    else {
	cnv->zeige_info();
	}
    }
}


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_convoiinfo_t::zeichnen(koord offset) const
{
	clip_dimension clip = display_gib_clip_wh();
	if(! ((pos.y+offset.y) > clip.yy ||  (pos.y+offset.y) < clip.y-32) &&  cnv.is_bound()) {

		static char buf[256];

/* prissi: no nummer
		sprintf(buf, "%d.", nummer);
		display_proportional_clip(pos.x+offset.x+4, pos.y+offset.y+8, buf, ALIGN_LEFT, COL_BLACK, true);
*/

		int max_x = display_proportional_clip(pos.x+offset.x+2,pos.y+offset.y+8+LINESPACE, translator::translate("Gewinn"), ALIGN_LEFT, COL_BLACK, true);

		money_to_string(buf, cnv->gib_jahresgewinn()/100);
		max_x += display_proportional_clip(pos.x+offset.x+2+max_x+5,pos.y+offset.y+8+LINESPACE, buf, ALIGN_LEFT, cnv->gib_jahresgewinn()>0?MONEY_PLUS:MONEY_MINUS, true);


		/*
		* only show assigned line, if there is one!
		*/
		if (cnv->in_depot())
		{
			const char *txt=translator::translate("(in depot)");
			int w=display_proportional_clip(pos.x+offset.x+2, pos.y+offset.y+8+2*LINESPACE,txt,ALIGN_LEFT, COL_BLACK, true);
			max_x = max(max_x,w);
		}
		else if (cnv->get_line() != NULL)
		{
			sprintf(buf, "%s: %s", translator::translate("Line"), cnv->get_line()->get_name());
			int w = display_proportional_clip(pos.x+offset.x+2, pos.y+offset.y+8+2*LINESPACE,buf,ALIGN_LEFT, COL_BLACK, true);
			max_x = max(max_x,w);
		}

		// name
		int w=display_proportional_clip(pos.x+offset.x+2, pos.y+offset.y+8,translator::translate(cnv->gib_name()),ALIGN_LEFT, COL_BLACK, true);
		max_x = max(max_x,w);

		// vehicles
		// we will use their images offests and width to shift them to their correct position
		// this should work with any vehicle size ...
		const int xoff = max(128, max_x);
		int left = pos.x+offset.x+xoff+4;
		for(unsigned i=0; i<cnv->gib_vehikel_anzahl();i++) {
			int x, y, w, h;
			const image_id bild=cnv->gib_vehikel(i)->gib_basis_bild();
			display_get_image_offset(bild, &x, &y, &w, &h );
			display_color_img(bild,left-x,pos.y+offset.y+13-y-h/2,cnv->gib_besitzer()->kennfarbe,false,true);
			left += (w*2)/3;
		}

		// since the only object is the loading bar, we can alter its position this way ...
		gui_container_t::zeichnen(pos + offset+koord(xoff-188+2,0));

	}
}
