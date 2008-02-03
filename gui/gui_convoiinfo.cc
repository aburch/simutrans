/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "gui_convoiinfo.h"
#include "../simdebug.h"
#include "../simworld.h"
#include "../simdepot.h"
#include "../vehicle/simvehikel.h"
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

    filled_bar.setze_pos(koord(2, 33));
    filled_bar.setze_groesse(koord(100, 4));
    filled_bar.add_color_value(&cnv->get_loading_limit(), COL_YELLOW);
    filled_bar.add_color_value(&cnv->get_loading_level(), COL_GREEN);
}

/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void gui_convoiinfo_t::infowin_event(const event_t *ev)
{
	if(cnv.is_bound()) {
		if(IS_LEFTRELEASE(ev)) {
			if(cnv->in_depot()) {
				grund_t *gr = cnv->get_welt()->lookup(cnv->gib_vehikel(0)->gib_pos());
				if(gr==NULL  ||  gr->gib_depot()==NULL) {
					gr = cnv->get_welt()->lookup(cnv->get_home_depot());
				}
				gr->gib_depot()->zeige_info();
			}
			else {
				cnv->zeige_info();
			}
		}
		else if(IS_RIGHTRELEASE(ev)) {
			cnv->get_welt()->change_world_position(cnv->gib_vehikel(0)->gib_pos());
		}
	}
}


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_convoiinfo_t::zeichnen(koord offset)
{
	clip_dimension clip = display_gib_clip_wh();
	if(! ((pos.y+offset.y) > clip.yy ||  (pos.y+offset.y) < clip.y-32) &&  cnv.is_bound()) {
		int max_x = display_proportional_clip(pos.x+offset.x+2,pos.y+offset.y+8+LINESPACE, translator::translate("Gewinn"), ALIGN_LEFT, COL_BLACK, true);

		char buf[256];
		money_to_string(buf, cnv->gib_jahresgewinn()/100);
		max_x += display_proportional_clip(pos.x+offset.x+2+max_x+5,pos.y+offset.y+8+LINESPACE, buf, ALIGN_LEFT, cnv->gib_jahresgewinn()>0?MONEY_PLUS:MONEY_MINUS, true);

		/*
		* only show assigned line, if there is one!
		*/
		if (cnv->in_depot()) {
			const char *txt=translator::translate("(in depot)");
			int w=display_proportional_clip(pos.x+offset.x+2, pos.y+offset.y+8+2*LINESPACE,txt,ALIGN_LEFT, COL_BLACK, true);
			max_x = max(max_x,w+2);
		}
		else if(cnv->get_line().is_bound()) {
			sint16 w = display_proportional_clip( pos.x+offset.x+2, pos.y+offset.y+8+2*LINESPACE, translator::translate("Line"), ALIGN_LEFT, COL_BLACK, true );
			w += display_proportional_clip( pos.x+offset.x+2+w+5, pos.y+offset.y+8+2*LINESPACE, cnv->get_line()->get_name(), ALIGN_LEFT, cnv->get_line()->get_state_color(), true );
			max_x = max(max_x,w+5);
		}

		// name
		int w = display_calc_proportional_string_len_width(cnv->gib_name(), 35535) + 2;
		max_x = max(max_x,w);

		// we will use their images offests and width to shift them to their correct position
		// this should work with any vehicle size ...
		const int xoff = max(128, max_x);
		int left = pos.x+offset.x+xoff+4;
		for(unsigned i=0; i<cnv->gib_vehikel_anzahl();i++) {
			KOORD_VAL x, y, w, h;
			const image_id bild=cnv->gib_vehikel(i)->gib_basis_bild();
			display_get_image_offset(bild, &x, &y, &w, &h );
			display_color_img(bild,left-x,pos.y+offset.y+13-y-h/2,cnv->gib_besitzer()->get_player_nr(),false,true);
			left += (w*2)/3;
		}

		// use the convoi status color for redraw: Possible colors are YELLOW (not moving) BLUE: obsolete in convoi, RED: minus income, BLACK: ok
		display_proportional_clip(pos.x+offset.x+2, pos.y+offset.y+8,cnv->gib_name(),ALIGN_LEFT, cnv->get_status_color(), true);

		// since the only remaining object is the loading bar, we can alter its position this way ...
		filled_bar.zeichnen(pos+offset+koord(xoff,0));
	}
}
