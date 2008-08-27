/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../../simdebug.h"
#include "gui_tab_panel.h"
#include "../../dataobj/translator.h"
#include "../../simevent.h"
#include "../../simgraph.h"
#include "../../simcolor.h"

#include "../../besch/skin_besch.h"
#include "../../besch/bild_besch.h"

#define IMG_WIDTH 20

gui_tab_panel_t::gui_tab_panel_t()
{
    active_tab = 0;
}



void gui_tab_panel_t::add_tab(gui_komponente_t *c, const char *name, const skin_besch_t *besch, const char *tooltip )
{
	tabs.append(tab(c, besch?NULL:name, besch?besch->gib_bild(0):NULL, tooltip));
	c->setze_groesse(gib_groesse()-koord(0,HEADER_VSIZE));
}




void gui_tab_panel_t::setze_groesse(koord groesse)
{
	gui_komponente_t::setze_groesse(groesse);

	this->groesse = groesse;

	for (slist_tpl<tab>::const_iterator i = tabs.begin(), end = tabs.end(); i != end; ++i) {
		i->component->setze_groesse(gib_groesse() - koord(0, HEADER_VSIZE));
	}
}


void gui_tab_panel_t::infowin_event(const event_t *ev)
{
	if(ev->my >= HEADER_VSIZE || ev->cy >= HEADER_VSIZE) {
		// Komponente getroffen
		event_t ev2 = *ev;
		translate_event(&ev2, 0, -HEADER_VSIZE);
		gib_aktives_tab()->infowin_event(&ev2);
	}
	else {
		if(IS_LEFTCLICK(ev)) {
			if(ev->my > 0 && ev->my < HEADER_VSIZE-1) {
				// Reiter getroffen
				int text_x = 4;
				int k = 0;
				for (slist_tpl<tab>::const_iterator i = tabs.begin(), end = tabs.end(); i != end; ++i, ++k) {
					const char* text = i->title;
					const int width = text ? proportional_string_width( text ) : IMG_WIDTH;

					if(text_x < ev->mx && text_x+width+8 > ev->mx) {
						// either tooltip or change
						active_tab = k;
						call_listeners((long)active_tab);
						break;
					}

					text_x += width + 8;
				}
			}
		}
	}
}



void gui_tab_panel_t::zeichnen(koord parent_pos)
{
	// Position am Bildschirm/Fenster
	const int xpos = parent_pos.x + pos.x;
	const int ypos = parent_pos.y + pos.y;

	int text_x = xpos+8;

	display_fillbox_wh_clip(xpos, ypos+HEADER_VSIZE-1, 4, 1, COL_WHITE, true);

	int k = 0;
	for (slist_tpl<tab>::const_iterator i = tabs.begin(), end = tabs.end(); i != end; ++i, ++k) {
		const char* text = i->title;
		const int width = text ? proportional_string_width( text ) : IMG_WIDTH;

		if (k != active_tab) {
			display_fillbox_wh_clip(text_x-4, ypos+HEADER_VSIZE-1, width+8, 1, MN_GREY4, true);
			display_fillbox_wh_clip(text_x-3, ypos+4, width+5, 1, MN_GREY4, true);

			display_vline_wh_clip(text_x-4, ypos+5, HEADER_VSIZE-6, MN_GREY4, true);
			display_vline_wh_clip(text_x+width+3, ypos+5, HEADER_VSIZE-6, MN_GREY0, true);

			if(text) {
				display_proportional_clip(text_x, ypos+7, text, ALIGN_LEFT, COL_WHITE, true);
			}
			else {
				display_color_img( i->img->gib_nummer(), text_x - i->img->get_pic()->x + (IMG_WIDTH/2) - (i->img->get_pic()->w/2), ypos - i->img->get_pic()->y + 10 - (i->img->get_pic()->h/2), 0, false, true);
			}
		} else {
			display_fillbox_wh_clip(text_x-3, ypos+3, width+5, 1, MN_GREY4, true);

			display_vline_wh_clip(text_x-4, ypos+4, 13, MN_GREY4, true);
			display_vline_wh_clip(text_x+width+3, ypos+4, 13, MN_GREY0, true);

			if(text) {
				display_proportional_clip(text_x, ypos+7, text, ALIGN_LEFT, COL_BLACK, true);
			}
			else {
				display_color_img( i->img->gib_nummer(), text_x - i->img->get_pic()->x + (IMG_WIDTH/2) - (i->img->get_pic()->w/2), ypos - i->img->get_pic()->y + 10 - (i->img->get_pic()->h/2), 0, false, true);
			}
			i->component->zeichnen(koord(xpos + 0, ypos + HEADER_VSIZE));
		}

		text_x += width + 8;
	}
	display_fillbox_wh_clip(text_x-4, ypos+HEADER_VSIZE-1, groesse.x-(text_x-xpos)+4, 1, MN_GREY4, true);

	int my = gib_maus_y()-parent_pos.y-6;
	if(my>=0  &&  my < HEADER_VSIZE-1) {
		// Reiter getroffen?
		int mx = gib_maus_x()-parent_pos.x-11;
		int text_x = 4;
		int k = 0;
		for (slist_tpl<tab>::const_iterator i = tabs.begin(), end = tabs.end(); i != end; ++i, ++k) {
			const char* text = i->title;
			const int width = text ? proportional_string_width( text ) : IMG_WIDTH;

			if(text_x < mx && text_x+width+8 > mx) {
				// tooltip or change
				win_set_tooltip(gib_maus_x() + 16, gib_maus_y() - 16, i->tooltip );
				break;
			}

			text_x += width + 8;
		}
	}
}
