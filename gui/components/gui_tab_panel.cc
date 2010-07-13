/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../../simdebug.h"
#include "gui_tab_panel.h"
#include "../../simevent.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../simwin.h"

#include "../../besch/skin_besch.h"
#include "../../besch/bild_besch.h"

#define IMG_WIDTH 20

gui_tab_panel_t::gui_tab_panel_t() :
	required_groesse( 8, HEADER_VSIZE )
{
	active_tab = 0;
	offset_tab = 0;
	left.init( button_t::arrowleft, NULL, koord(0,0) );
	left.add_listener( this );
	right.init( button_t::arrowright, NULL, koord(0,0) );
	right.add_listener( this );
}



void gui_tab_panel_t::add_tab(gui_komponente_t *c, const char *name, const skin_besch_t *besch, const char *tooltip )
{
	tabs.append( tab(c, besch?NULL:name, besch?besch->get_bild(0):NULL, tooltip) );
	set_groesse( get_groesse() );
}




void gui_tab_panel_t::set_groesse(koord gr)
{
	gui_komponente_t::set_groesse(gr);

	required_groesse = koord( 8, HEADER_VSIZE );
	for (slist_tpl<tab>::iterator i = tabs.begin(), end = tabs.end(); i != end; ++i) {
		i->x_offset = required_groesse.x-4;
		i->width = 8 + (i->title ? proportional_string_width( i->title ) : IMG_WIDTH);
		required_groesse.x += i->width;
		i->component->set_groesse( get_groesse() - koord(0, HEADER_VSIZE) );
	}

	if(  required_groesse.x>gr.x  ) {
		left.set_pos( koord( 2, 2 ) );
		right.set_pos( koord( gr.x-10, 2 ) );
	}
}


bool gui_tab_panel_t::action_triggered(gui_action_creator_t *komp, value_t)
{
	if(  komp == &left  ) {
		offset_tab = min( offset_tab+1, tabs.get_count()-1 );
	}
	else if(  komp == &right  ) {
		offset_tab = max( offset_tab-1, 0 );
	}
	return true;
}


bool gui_tab_panel_t::infowin_event(const event_t *ev)
{
	if(  required_groesse.x>groesse.x  ) {
		// buttons pressed
		if(  left.getroffen(ev->cx, ev->cy)  &&  ev->ev_code == MOUSE_LEFTBUTTON  ) {
			event_t ev2 = *ev;
			translate_event(&ev2, -left.get_pos().x, -left.get_pos().y);
			return left.infowin_event(&ev2);
		}
		else if(  right.getroffen(ev->cx, ev->cy)  &&  ev->ev_code == MOUSE_LEFTBUTTON  ) {
			event_t ev2 = *ev;
			translate_event(&ev2, -right.get_pos().x, -right.get_pos().y);
			return right.infowin_event(&ev2);
		}
	}

	if(  IS_LEFTRELEASE(ev)  &&  (ev->my > 0  &&  ev->my < HEADER_VSIZE-1)  )  {
		// Reiter getroffen
		int text_x = required_groesse.x>groesse.x ? 14 : 4;
		int k=0;
		for(  slist_tpl<tab>::const_iterator i = tabs.begin(), end = tabs.end();  i != end;  ++i, ++k  ) {
			if(  k>=offset_tab  ) {
				if(  text_x < ev->mx  &&  text_x+i->width > ev->mx  ) {
					// either tooltip or change
					active_tab = k;
					call_listeners((long)active_tab);
					return true;
				}
				text_x += i->width;
			}
		}
		return true;
	}

	// Knightly : navigate among the tabs using Ctrl-PgUp and Ctrl-PgDn
	if(  ev->ev_class==EVENT_KEYBOARD  &&  (ev->ev_key_mod & 2)  ) {
		if(  ev->ev_code==62  ) {
			// Ctrl-PgUp -> go to the previous tab
			const int next_tab_idx = active_tab - 1;
			active_tab = next_tab_idx<0 ? max(0, (int)tabs.get_count()-1) : next_tab_idx;
			return true;
		}
		else if(  ev->ev_code==60  ) {
			// Ctrl-PgDn -> go to the next tab
			const int next_tab_idx = active_tab + 1;
			active_tab = next_tab_idx>=(int)tabs.get_count() ? 0 : next_tab_idx;
			return true;
		}
	}

	if(  ev->ev_class == EVENT_KEYBOARD  ||  DOES_WINDOW_CHILDREN_NEED(ev)  ||  get_aktives_tab()->getroffen(ev->mx, ev->my)  ||  get_aktives_tab()->getroffen(ev->cx, ev->cy)) {
		// Komponente getroffen
		event_t ev2 = *ev;
		translate_event(&ev2, -get_aktives_tab()->get_pos().x, -get_aktives_tab()->get_pos().y-HEADER_VSIZE);
		return get_aktives_tab()->infowin_event(&ev2);
	}
	return false;
}



void gui_tab_panel_t::zeichnen(koord parent_pos)
{
	// Position am Bildschirm/Fenster
	int xpos = parent_pos.x + pos.x;
	const int ypos = parent_pos.y + pos.y;

	if(  required_groesse.x>groesse.x  ) {
		left.zeichnen( parent_pos+pos );
		right.zeichnen( parent_pos+pos );
		display_fillbox_wh_clip(xpos, ypos+HEADER_VSIZE-1, 10, 1, COL_WHITE, true);
		xpos += 10;
	}

	int text_x = xpos+8;

	display_fillbox_wh_clip(xpos, ypos+HEADER_VSIZE-1, 4, 1, COL_WHITE, true);

	int i=0;
	for (slist_tpl<tab>::const_iterator iter = tabs.begin(), end = tabs.end(); iter != end; ++iter, ++i) {
		if(  i<offset_tab  ) {
			// just draw component, if here ...
			if (i == active_tab) {
				iter->component->zeichnen(koord(parent_pos.x+pos.x, ypos + required_groesse.y));
			}
		}
		else {
			// only start drwing here ...
			const char* text = iter->title;
			const int width = text ? proportional_string_width( text ) : IMG_WIDTH;

			if (i != active_tab) {
				display_fillbox_wh_clip(text_x-4, ypos+HEADER_VSIZE-1, width+8, 1, MN_GREY4, true);
				display_fillbox_wh_clip(text_x-3, ypos+4, width+5, 1, MN_GREY4, true);

				display_vline_wh_clip(text_x-4, ypos+5, HEADER_VSIZE-6, MN_GREY4, true);
				display_vline_wh_clip(text_x+width+3, ypos+5, HEADER_VSIZE-6, MN_GREY0, true);

				if(text) {
					display_proportional_clip(text_x, ypos+7, text, ALIGN_LEFT, COL_WHITE, true);
				}
				else {
					display_img_blend( iter->img->get_nummer(), text_x - iter->img->get_pic()->x + (IMG_WIDTH/2) - (iter->img->get_pic()->w/2), ypos - iter->img->get_pic()->y + 10 - (iter->img->get_pic()->h/2), TRANSPARENT50_FLAG, false, true);
				}
			}
			else {
				display_fillbox_wh_clip(text_x-3, ypos+3, width+5, 1, MN_GREY4, true);

				display_vline_wh_clip(text_x-4, ypos+4, 13, MN_GREY4, true);
				display_vline_wh_clip(text_x+width+3, ypos+4, 13, MN_GREY0, true);

				if(text) {
					display_proportional_clip(text_x, ypos+7, text, ALIGN_LEFT, COL_BLACK, true);
				}
				else {
					display_color_img( iter->img->get_nummer(), text_x - iter->img->get_pic()->x + (IMG_WIDTH/2) - (iter->img->get_pic()->w/2), ypos - iter->img->get_pic()->y + 10 - (iter->img->get_pic()->h/2), 0, false, true);
				}
				iter->component->zeichnen(koord(parent_pos.x+pos.x, ypos + required_groesse.y));
			}

			text_x += width + 8;
		}
	}
	display_fillbox_wh_clip(text_x-4, ypos+HEADER_VSIZE-1, xpos+groesse.x-(text_x-4), 1, MN_GREY4, true);

	// now for tooltips ...
	int my = get_maus_y()-parent_pos.y-6;
	if(my>=0  &&  my < HEADER_VSIZE-1) {
		// Reiter getroffen?
		int mx = get_maus_x()-parent_pos.x-11;
		int text_x = 4;
		int i=0;
		for (slist_tpl<tab>::const_iterator iter = tabs.begin(), end = tabs.end(); iter != end; ++iter, ++i) {
			if(  i>=offset_tab  ) {
				const char* text = iter->title;
				const int width = text ? proportional_string_width( text ) : IMG_WIDTH;

				if(text_x < mx && text_x+width+8 > mx) {
					// tooltip or change
					win_set_tooltip(get_maus_x() + 16, get_maus_y() - 16, iter->tooltip );
					break;
				}

				text_x += width + 8;
			}
		}
	}
}


void gui_tab_panel_t::clear()
{
	tabs.clear();
	active_tab = 0;
}
