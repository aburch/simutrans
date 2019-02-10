/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * A class for distribution of tabs through the gui_component_t component.
 * @author Hj. Malthaner
 */

#include "gui_tab_panel.h"
#include "../gui_frame.h"
#include "../../simevent.h"
#include "../../display/simgraph.h"
#include "../../simcolor.h"
#include "../simwin.h"

#include "../../descriptor/skin_desc.h"

#define IMG_WIDTH 20

scr_coord_val gui_tab_panel_t::header_vsize = 18;


gui_tab_panel_t::gui_tab_panel_t() :
	required_size( 8, D_TAB_HEADER_HEIGHT )
{
	active_tab = 0;
	offset_tab = 0;
	left.init( button_t::arrowleft, NULL, scr_coord(0,0) );
	left.add_listener( this );
	right.init( button_t::arrowright, NULL, scr_coord(0,0) );
	right.add_listener( this );
}



void gui_tab_panel_t::add_tab(gui_component_t *c, const char *name, const skin_desc_t *desc, const char *tooltip )
{
	tabs.append( tab(c, desc?NULL:name, desc?desc->get_image(0):NULL, tooltip) );
	set_size( get_size() );
}




void gui_tab_panel_t::set_size(scr_size size)
{
	gui_component_t::set_size(size);

	required_size = scr_size( 8, required_size.h );
	FOR(slist_tpl<tab>, & i, tabs) {
		i.x_offset = required_size.w - 4;
		i.width             = 8 + (i.title ? proportional_string_width(i.title) : IMG_WIDTH);
		required_size.w += i.width;
		if (i.title) {
			required_size.h = max(required_size.h, LINESPACE + D_V_SPACE);
		}
		i.component->set_pos(scr_coord(0, required_size.h));
		i.component->set_size(get_size() - scr_size(0, required_size.h));
	}

	if(  required_size.w > size.w  ||  offset_tab > 0  ) {
		left.set_pos( scr_coord( 0, 0 ) );
		left.set_size( scr_size( D_ARROW_LEFT_WIDTH, required_size.h ) );
		right.set_pos( scr_coord( size.w-D_ARROW_RIGHT_WIDTH, 0 ) );
		right.set_size( scr_size( D_ARROW_RIGHT_WIDTH, required_size.h ) );
	}
}


scr_size gui_tab_panel_t::get_min_size() const
{
	scr_size t_size(0, required_size.h);
	scr_size c_size(0, 0);
	FOR(slist_tpl<tab>, const& iter, tabs) {
		if (iter.title) {
			t_size.h = max(t_size.h, LINESPACE + D_V_SPACE);
		}
		c_size.clip_lefttop( iter.component->get_min_size() );
	}
	return t_size + c_size;
}


bool gui_tab_panel_t::action_triggered(gui_action_creator_t *comp, value_t)
{
	if(  comp == &right  ) {
		offset_tab = min( offset_tab+1, tabs.get_count()-1 );
	}
	else if(  comp == &left  ) {
		offset_tab = max( offset_tab-1, 0 );
	}
	return true;
}


bool gui_tab_panel_t::infowin_event(const event_t *ev)
{
	if(  (required_size.w>size.w  ||  offset_tab > 0)  &&  ev->ev_class!=EVENT_KEYBOARD  &&  ev->ev_code==MOUSE_LEFTBUTTON  ) {
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

	if(  IS_LEFTRELEASE(ev)  &&  (ev->my > 0  &&  ev->my < required_size.h-1)  )  {
		// tab selector was hit
		int text_x = required_size.w>size.w ? 14 : 4;
		int k=0;
		FORX(slist_tpl<tab>, const& i, tabs, ++k) {
			if(  k >= offset_tab  ) {
				if (text_x < ev->mx && text_x + i.width > ev->mx) {
					// either tooltip or change
					active_tab = k;
					call_listeners((long)active_tab);
					return true;
				}
				text_x += i.width;
			}
		}
		return false;
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
		// active tab was hit
		event_t ev2 = *ev;
		translate_event(&ev2, -get_aktives_tab()->get_pos().x, -get_aktives_tab()->get_pos().y );
		return get_aktives_tab()->infowin_event(&ev2);
	}
	return false;
}



void gui_tab_panel_t::draw(scr_coord parent_pos)
{
	// Position in screen/window
	int xpos = parent_pos.x + pos.x;
	const int ypos = parent_pos.y + pos.y;

	if(  required_size.w>size.w  ||  offset_tab > 0) {
		left.draw( parent_pos+pos );
		right.draw( parent_pos+pos );
		//display_fillbox_wh_clip_rgb(xpos, ypos+required_size.h-1, 10, 1, SYSCOL_TEXT_HIGHLIGHT, true);
		display_fillbox_wh_clip_rgb(xpos, ypos+required_size.h-1, D_ARROW_LEFT_WIDTH, 1, SYSCOL_HIGHLIGHT, true);
		xpos += D_ARROW_LEFT_WIDTH;
	}

	int text_x = xpos + 8;
	int text_y = ypos + (required_size.h - LINESPACE)/2;

	//display_fillbox_wh_clip_rgb(xpos, ypos+required_size.h-1, 4, 1, color_idx_to_rgb(COL_WHITE), true);
	display_fillbox_wh_clip_rgb(xpos, ypos+required_size.h-1, 4, 1, SYSCOL_HIGHLIGHT, true);

	// do not draw under right button
	int xx = required_size.w>get_size().w ? get_size().w-(D_ARROW_LEFT_WIDTH+2+D_ARROW_RIGHT_WIDTH) : get_size().w;

	int i=0;
	FORX(slist_tpl<tab>, const& iter, tabs, ++i) {
		// just draw component, if here ...
		if (i == active_tab) {
			iter.component->draw(parent_pos + pos);
		}
		if(i>=offset_tab) {
			// set clipping
			PUSH_CLIP_FIT(xpos, ypos, xx, required_size.h);
			// only start drawing here ...
			char const* const text = iter.title;
			const int width = text ? proportional_string_width( text ) : IMG_WIDTH;

			if (i != active_tab) {
				// Non active tabs
				display_fillbox_wh_clip_rgb(text_x-3, ypos+2, width+6, 1, SYSCOL_HIGHLIGHT, true);
				display_fillbox_wh_clip_rgb(text_x-4, ypos+required_size.h-1, width+8, 1, SYSCOL_HIGHLIGHT, true);

				display_vline_wh_clip_rgb(text_x-4, ypos+3, required_size.h-4, SYSCOL_HIGHLIGHT, true);
				display_vline_wh_clip_rgb(text_x+width+3, ypos+3, required_size.h-4, SYSCOL_SHADOW, true);

				if(text) {
					display_proportional_clip_rgb(text_x, text_y, text, ALIGN_LEFT, SYSCOL_TEXT, true);
				}
				else {
					scr_coord_val const y = ypos   - iter.img->get_pic()->y + 10            - iter.img->get_pic()->h / 2;
					scr_coord_val const x = text_x - iter.img->get_pic()->x + IMG_WIDTH / 2 - iter.img->get_pic()->w / 2;
					display_img_blend(iter.img->get_id(), x, y, TRANSPARENT50_FLAG, false, true);
				}
			}
			else {
				// Active tab
				display_fillbox_wh_clip_rgb(text_x-3, ypos, width+6, 1, SYSCOL_HIGHLIGHT, true);

				display_vline_wh_clip_rgb(text_x-4, ypos+1, required_size.h-2, SYSCOL_HIGHLIGHT, true);
				display_vline_wh_clip_rgb(text_x+width+3, ypos+1, required_size.h-2, SYSCOL_SHADOW, true);

				if(text) {
					display_proportional_clip_rgb(text_x, text_y, text, ALIGN_LEFT, SYSCOL_TEXT_HIGHLIGHT, true);
				}
				else {
					scr_coord_val const y = ypos   - iter.img->get_pic()->y + 10            - iter.img->get_pic()->h / 2;
					scr_coord_val const x = text_x - iter.img->get_pic()->x + IMG_WIDTH / 2 - iter.img->get_pic()->w / 2;
					display_color_img(iter.img->get_id(), x, y, 0, false, true);
				}
			}
			text_x += width + 8;
			// reset clipping
			POP_CLIP();
		}
	}
	display_fillbox_wh_clip_rgb(text_x-4, ypos+required_size.h-1, xpos+size.w-(text_x-4), 1, SYSCOL_HIGHLIGHT, true);

	// now for tooltips ...
	int my = get_mouse_y()-parent_pos.y-pos.y-6;
	if(my>=0  &&  my < required_size.h-1) {
		// Reiter getroffen?
		int mx = get_mouse_x()-parent_pos.x-pos.x-11;
		int text_x = 4;
		int i=0;
		FORX(slist_tpl<tab>, const& iter, tabs, ++i) {
			if(  i>=offset_tab  ) {
				char const* const text = iter.title;
				const int width = text ? proportional_string_width( text ) : IMG_WIDTH;

				if(text_x < mx && text_x+width+8 > mx  && (required_size.w<=get_size().w || mx < right.get_pos().x-12)) {
					// tooltip or change
					win_set_tooltip(get_mouse_x() + 16, ypos + required_size.h + 12, iter.tooltip, &iter, this);
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


void gui_tab_panel_t::take_tabs(gui_tab_panel_t* other)
{
	tabs.append_list(other->tabs);
}


void gui_tab_panel_t::rdwr( loadsave_t *file )
{
	sint32 a = get_active_tab_index();
	file->rdwr_long(a);
	if (file->is_loading()) {
		set_active_tab_index(a);
	}
}
