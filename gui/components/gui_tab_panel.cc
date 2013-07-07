/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * A class for distribution of tabs through the gui_komponente_t component.
 * @author Hj. Malthaner
 */

#include "gui_tab_panel.h"
#include "../gui_frame.h"
#include "../../simevent.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../simwin.h"

#include "../../besch/skin_besch.h"

#define IMG_WIDTH 20

KOORD_VAL gui_tab_panel_t::header_vsize = 18;


gui_tab_panel_t::gui_tab_panel_t() :
	required_groesse( 8, TAB_HEADER_V_SIZE )
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

	required_groesse = koord( 8, TAB_HEADER_V_SIZE );
	FOR(slist_tpl<tab>, & i, tabs) {
		i.x_offset          = required_groesse.x - 4;
		i.width             = 8 + (i.title ? proportional_string_width(i.title) : IMG_WIDTH);
		required_groesse.x += i.width;
		i.component->set_pos(koord(0, TAB_HEADER_V_SIZE));
		i.component->set_groesse(get_groesse() - koord(0, TAB_HEADER_V_SIZE));
	}

	if(  required_groesse.x>gr.x  ||  offset_tab > 0) {
		left.set_pos( koord( 2, 5 ) );
		right.set_pos( koord( gr.x-10, 5 ) );
	}
}


bool gui_tab_panel_t::action_triggered(gui_action_creator_t *komp, value_t)
{
	if(  komp==&right  ) {
		offset_tab = min( offset_tab+1, tabs.get_count()-1 );
	}
	else if(  komp==&left  ) {
		offset_tab = max( offset_tab-1, 0 );
	}
	return true;
}


bool gui_tab_panel_t::infowin_event(const event_t *ev)
{
	if(  (required_groesse.x>groesse.x  ||  offset_tab > 0)  &&  ev->ev_class!=EVENT_KEYBOARD  &&  ev->ev_code==MOUSE_LEFTBUTTON  ) {
		// buttons pressed
		if(  left.getroffen(ev->cx, ev->cy)  ) {
			event_t ev2 = *ev;
			translate_event(&ev2, -left.get_pos().x, -left.get_pos().y);
			return left.infowin_event(&ev2);
		}
		else if(  right.getroffen(ev->cx, ev->cy)  ) {
			event_t ev2 = *ev;
			translate_event(&ev2, -right.get_pos().x, -right.get_pos().y);
			return right.infowin_event(&ev2);
		}
	}

	if(  IS_LEFTRELEASE(ev)  &&  (ev->my > 0  &&  ev->my < TAB_HEADER_V_SIZE-1)  )  {
		// Reiter getroffen
		int text_x = required_groesse.x>groesse.x ? 14 : 4;
		int k=0;
		FORX(slist_tpl<tab>, const& i, tabs, ++k) {
			if(  k>=offset_tab  ) {
				if (text_x < ev->mx && text_x + i.width > ev->mx) {
					// either tooltip or change
					active_tab = k;
					call_listeners((long)active_tab);
					return true;
				}
				text_x += i.width;
			}
		}
		return true;
	}

	// Knightly : navigate among the tabs using Ctrl-PgUp and Ctrl-PgDn
	if(  ev->ev_class==EVENT_KEYBOARD  &&  IS_CONTROL_PRESSED(ev)  ) {
		if(  ev->ev_code==SIM_KEY_PGUP  ) {
			// Ctrl-PgUp -> go to the previous tab
			const int next_tab_idx = active_tab - 1;
			active_tab = next_tab_idx<0 ? max(0, (int)tabs.get_count()-1) : next_tab_idx;
			return true;
		}
		else if(  ev->ev_code==SIM_KEY_PGDN  ) {
			// Ctrl-PgDn -> go to the next tab
			const int next_tab_idx = active_tab + 1;
			active_tab = next_tab_idx>=(int)tabs.get_count() ? 0 : next_tab_idx;
			return true;
		}
	}

	if(  ev->ev_class == EVENT_KEYBOARD  ||  DOES_WINDOW_CHILDREN_NEED(ev)  ||  get_aktives_tab()->getroffen(ev->mx, ev->my)  ||  get_aktives_tab()->getroffen(ev->cx, ev->cy)) {
		// Komponente getroffen
		event_t ev2 = *ev;
		translate_event(&ev2, -get_aktives_tab()->get_pos().x, -get_aktives_tab()->get_pos().y );
		return get_aktives_tab()->infowin_event(&ev2);
	}
	return false;
}



void gui_tab_panel_t::zeichnen(koord parent_pos)
{
	// Position am Bildschirm/Fenster
	int xpos = parent_pos.x + pos.x;
	const int ypos = parent_pos.y + pos.y;

	if(  required_groesse.x>groesse.x  ||  offset_tab > 0) {
		left.zeichnen( parent_pos+pos );
		right.zeichnen( parent_pos+pos );
		display_fillbox_wh_clip(xpos, ypos+TAB_HEADER_V_SIZE-1, 10, 1, COL_WHITE, true);
		xpos += 10;
	}

	int text_x = xpos+8;

	display_fillbox_wh_clip(xpos, ypos+TAB_HEADER_V_SIZE-1, 4, 1, COL_WHITE, true);

	// do not draw under right button
	int xx = required_groesse.x>get_groesse().x ? get_groesse().x-22 : get_groesse().x;

	int i=0;
	FORX(slist_tpl<tab>, const& iter, tabs, ++i) {
		// just draw component, if here ...
		if (i == active_tab) {
			iter.component->zeichnen(parent_pos + pos);
		}
		if(i>=offset_tab) {
			// set clipping
			PUSH_CLIP(xpos, ypos, xx, TAB_HEADER_V_SIZE);
			// only start drawing here ...
			char const* const text = iter.title;
			const int width = text ? proportional_string_width( text ) : IMG_WIDTH;

			if (i != active_tab) {
				display_fillbox_wh_clip(text_x-4, ypos+TAB_HEADER_V_SIZE-1, width+8, 1, MN_GREY4, true);
				display_fillbox_wh_clip(text_x-3, ypos+4, width+5, 1, MN_GREY4, true);

				display_vline_wh_clip(text_x-4, ypos+5, TAB_HEADER_V_SIZE-6, MN_GREY4, true);
				display_vline_wh_clip(text_x+width+3, ypos+5, TAB_HEADER_V_SIZE-6, MN_GREY0, true);

				if(text) {
					display_proportional_clip(text_x, ypos+7, text, ALIGN_LEFT, COL_WHITE, true);
				}
				else {
					KOORD_VAL const y = ypos   - iter.img->get_pic()->y + 10            - iter.img->get_pic()->h / 2;
					KOORD_VAL const x = text_x - iter.img->get_pic()->x + IMG_WIDTH / 2 - iter.img->get_pic()->w / 2;
					display_img_blend(iter.img->get_nummer(), x, y, TRANSPARENT50_FLAG, false, true);
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
					KOORD_VAL const y = ypos   - iter.img->get_pic()->y + 10            - iter.img->get_pic()->h / 2;
					KOORD_VAL const x = text_x - iter.img->get_pic()->x + IMG_WIDTH / 2 - iter.img->get_pic()->w / 2;
					display_color_img(iter.img->get_nummer(), x, y, 0, false, true);
				}
			}
			text_x += width + 8;
			// reset clipping
			POP_CLIP();
		}
	}
	display_fillbox_wh_clip(text_x-4, ypos+TAB_HEADER_V_SIZE-1, xpos+groesse.x-(text_x-4), 1, MN_GREY4, true);

	// now for tooltips ...
	int my = get_maus_y()-parent_pos.y-pos.y-6;
	if(my>=0  &&  my < TAB_HEADER_V_SIZE-1) {
		// Reiter getroffen?
		int mx = get_maus_x()-parent_pos.x-pos.x-11;
		int text_x = 4;
		int i=0;
		FORX(slist_tpl<tab>, const& iter, tabs, ++i) {
			if(  i>=offset_tab  ) {
				char const* const text = iter.title;
				const int width = text ? proportional_string_width( text ) : IMG_WIDTH;

				if(text_x < mx && text_x+width+8 > mx  && (required_groesse.x<=get_groesse().x || mx < right.get_pos().x-12)) {
					// tooltip or change
					win_set_tooltip(get_maus_x() + 16, ypos + TAB_HEADER_V_SIZE + 12, iter.tooltip, &iter, this);
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
