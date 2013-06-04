/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Convoi info stats, like loading status bar
 */

#include "gui_convoiinfo.h"
#include "../simworld.h"
#include "../vehicle/simvehikel.h"
#include "../simconvoi.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../player/simplay.h"
#include "../simline.h"

#include "../dataobj/translator.h"

#include "../utils/simstring.h"


gui_convoiinfo_t::gui_convoiinfo_t(convoihandle_t cnv)
{
    this->cnv = cnv;

    filled_bar.set_pos(koord(2, 33));
    filled_bar.set_groesse(koord(100, 4));
    filled_bar.add_color_value(&cnv->get_loading_limit(), COL_YELLOW);
    filled_bar.add_color_value(&cnv->get_loading_level(), COL_GREEN);
}

/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool gui_convoiinfo_t::infowin_event(const event_t *ev)
{
	if(cnv.is_bound()) {
		if(IS_LEFTRELEASE(ev)) {
			cnv->zeige_info();
			return true;
		}
		else if(IS_RIGHTRELEASE(ev)) {
			cnv->get_welt()->change_world_position(cnv->get_pos());
			return true;
		}
	}
	return false;
}


/**
 * Draw the component
 * @author Hj. Malthaner
 */
void gui_convoiinfo_t::zeichnen(koord offset)
{
	clip_dimension clip = display_get_clip_wh();
	if(! ((pos.y+offset.y) > clip.yy ||  (pos.y+offset.y) < clip.y-32) &&  cnv.is_bound()) {
		// name, use the convoi status color for redraw: Possible colors are YELLOW (not moving) BLUE: obsolete in convoi, RED: minus income, BLACK: ok
		int max_x = display_proportional_clip(pos.x+offset.x+2, pos.y+offset.y+6, cnv->get_name(), ALIGN_LEFT, cnv->get_status_color(), true)+2;

		int w = display_proportional_clip(pos.x+offset.x+2,pos.y+offset.y+6+LINESPACE, translator::translate("Gewinn"), ALIGN_LEFT, COL_BLACK, true)+2;
		char buf[256];
		money_to_string(buf, cnv->get_jahresgewinn() / 100.0 );
		w += display_proportional_clip(pos.x+offset.x+2+w+5,pos.y+offset.y+6+LINESPACE, buf, ALIGN_LEFT, cnv->get_jahresgewinn()>0?MONEY_PLUS:MONEY_MINUS, true);
		max_x = max(max_x,w+5);

		// only show assigned line, if there is one!
		if (cnv->in_depot()) {
			const char *txt = translator::translate("(in depot)");
			int w = display_proportional_clip(pos.x+offset.x+2, pos.y+offset.y+6+2*LINESPACE,txt,ALIGN_LEFT, COL_BLACK, true)+2;
			max_x = max(max_x,w);
		}
		else if(cnv->get_line().is_bound()) {
			int w = display_proportional_clip( pos.x+offset.x+2, pos.y+offset.y+6+2*LINESPACE, translator::translate("Line"), ALIGN_LEFT, COL_BLACK, true)+2;
			w += display_proportional_clip( pos.x+offset.x+2+w+5, pos.y+offset.y+6+2*LINESPACE, cnv->get_line()->get_name(), ALIGN_LEFT, cnv->get_line()->get_state_color(), true);
			max_x = max(max_x,w+5);
		}

		// we will use their images offests and width to shift them to their correct position
		// this should work with any vehicle size ...
		const int xoff = max(190, max_x);
		int left = pos.x+offset.x+xoff+4;
		for(unsigned i=0; i<cnv->get_vehikel_anzahl();i++) {
			KOORD_VAL x, y, w, h;
			const image_id bild=cnv->get_vehikel(i)->get_basis_bild();
			display_get_base_image_offset(bild, &x, &y, &w, &h );
			display_base_img(bild,left-x,pos.y+offset.y+13-y-h/2,cnv->get_besitzer()->get_player_nr(),false,true);
			left += (w*2)/3;
		}

		// since the only remaining object is the loading bar, we can alter its position this way ...
		filled_bar.zeichnen(pos+offset+koord(xoff,0));
	}
}
