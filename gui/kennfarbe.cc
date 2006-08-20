/*
 * kennfarbe.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "../simdebug.h"
#include "../simevent.h"
#include "../simimg.h"
#include "../simplay.h"
#include "../besch/skin_besch.h"
#include "../simskin.h"
#include "../dataobj/translator.h"
#include "kennfarbe.h"
#include "../simgraph.h"


farbengui_t::farbengui_t(spieler_t *sp) :
	gui_frame_t("Meldung",sp),
	txt(translator::translate("COLOR_CHOOSE\n"))
{
	this->sp = sp;
	setze_fenstergroesse( koord(216, 84) );
	setze_opaque(true);
	txt.setze_pos( koord(10,10) );
	add_komponente( &txt );
}



void farbengui_t::infowin_event(const event_t *ev)
{
	gui_frame_t::infowin_event(ev);

	if(IS_LEFTCLICK(ev)) {
		if(ev->mx >= 136 && ev->mx <= 200) {
			// choose new color
			const int x = (ev->mx-136)/32;
			const int y = (ev->my-16)/8;
			const int f = (y + x*8);
			if(f>=0 && f<16) {
				sp->set_player_color(f*4);
			}
		}
	}
}


/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 *
 * @author Hj. Malthaner
 */
void farbengui_t::zeichnen(koord pos, koord gr)
{
	gui_frame_t::zeichnen(pos, gr);

	display_color_img(skinverwaltung_t::farbmenu->gib_bild_nr(0), pos.x + 136, pos.y + 16, 0, false, false);
}
