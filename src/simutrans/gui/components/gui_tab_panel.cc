/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */


#include "gui_tab_panel.h"
#include "../gui_frame.h"
#include "../../simevent.h"
#include "../../dataobj/environment.h"
#include "../../display/simgraph.h"
#include "../../simcolor.h"
#include "../simwin.h"
#include "../../world/simworld.h"

#include "../../descriptor/skin_desc.h"

#define IMG_WIDTH 20

scr_coord_val gui_tab_panel_t::header_vsize = 18;


gui_tab_panel_t::gui_tab_panel_t() :
	required_size( 8, D_TAB_HEADER_HEIGHT )
{
	active_tab = 0;
	tab_offset_x = 0;
	is_dragging = false;
	left.init( button_t::arrowleft, NULL, scr_coord(0,0) );
	left.add_listener( this );
	right.init( button_t::arrowright, NULL, scr_coord(0,0) );
	right.add_listener( this );
}



void gui_tab_panel_t::add_tab(gui_component_t *c, const char *name, const skin_desc_t *desc, const char *tooltip )
{
	tabs.append( tab(c, desc?NULL:name, desc?desc->get_image(0):NULL, tooltip) );
	// only call set_size, if size was already assigned
	if (size.w > 0  && size.h > 0) {
		set_size( get_size() );
	}
}




void gui_tab_panel_t::set_size(scr_size size)
{
	gui_component_t::set_size(size);

	required_size = scr_size( 8, required_size.h );
	gui_component_t *last_component = NULL;
	for(tab & i : tabs) {
		i.x_offset = required_size.w - 4;
		if( i.title ) {
			i.width = D_H_SPACE*2 + proportional_string_width( i.title );
			required_size.h = max( required_size.h, LINESPACE + D_V_SPACE );
		}
		else if( i.img ) {
			i.width = max( 2 + i.img->get_pic()->w, D_H_SPACE*2+IMG_WIDTH );;
			required_size.h = max( required_size.h, i.img->get_pic()->h + D_V_SPACE );
		}
		else {
			i.width = 8+IMG_WIDTH;
		}
		required_size.w += i.width;
		if (i.component != last_component) {
			i.component->set_pos(scr_coord(0, required_size.h));
			i.component->set_size(get_size() - scr_size(0, required_size.h));
			last_component = i.component;
		}
	}

	if(  required_size.w > size.w  || tab_offset_x > 0  ) {
		left.set_pos( scr_coord( 0, 0 ) );
		left.set_size( scr_size( D_ARROW_LEFT_WIDTH, required_size.h ) );
		right.set_pos( scr_coord( size.w-D_ARROW_RIGHT_WIDTH, 0 ) );
		right.set_size( scr_size( D_ARROW_RIGHT_WIDTH, required_size.h ) );
	}
	else {
		tab_offset_x = 0;
	}
}


scr_size gui_tab_panel_t::get_min_size() const
{
	scr_size t_size(0, required_size.h);
	scr_size c_size(0, 0);
	gui_component_t *last_component = NULL;
	for(tab const& iter : tabs) {
		if (iter.title) {
			t_size.h = max(t_size.h, LINESPACE + D_V_SPACE);
		}
		if (iter.component != last_component) {
			scr_size cs = iter.component->get_min_size();
			if (!iter.component->is_marginless()) {
				cs.h += D_MARGIN_BOTTOM;
			}
			c_size.clip_lefttop( cs );
			last_component = iter.component;
		}
	}
	return t_size+c_size;
}


bool gui_tab_panel_t::action_triggered(gui_action_creator_t *comp, value_t)
{
	if(  comp == &right  ) {
		tab_offset_x += 32;
	}
	else if(  comp == &left  ) {
		tab_offset_x -= 32;
	}
	tab_offset_x = clamp(tab_offset_x, 0, required_size.w - size.w);
	return true;
}


bool gui_tab_panel_t::tab_getroffen(scr_coord p)
{
	return p.x >= 0  &&  p.x < size.w  &&  p.y >= 0  &&  p.y < required_size.h;
}

bool gui_tab_panel_t::infowin_event(const event_t *ev)
{
	if (gui_component_t *t=get_aktives_tab()) {
		if (t->get_focus()) {
			// we must go through the containers or we do not get the correct offsets
			event_t ev2 = *ev;
			ev2.move_origin(t->get_pos());
			if (t->infowin_event(&ev2)) {
				return true;
			}
		}
	}

	// since we get can grab the focus to get keyboard events, we must make sure to handle mouse events only if we are hit
	if (ev->ev_class < EVENT_CLICK || IS_WHEELUP(ev) || IS_WHEELDOWN(ev)) {
		is_dragging = false;
	}

	if(  required_size.w > size.w  &&  tab_getroffen(ev->click_pos)  ) {
		if (!is_dragging) {
			// handle scroll buttons pressed
			if(  left.getroffen(ev->click_pos)  ) {
				event_t ev2 = *ev;
				ev2.move_origin(left.get_pos());
				return left.infowin_event(&ev2);
			}
			if(  right.getroffen(ev->click_pos)  ) {
				event_t ev2 = *ev;
				ev2.move_origin(right.get_pos());
				return right.infowin_event(&ev2);
			}
		}

		if (ev->ev_class == EVENT_CLICK || ev->ev_class == EVENT_DRAG) {
			// init dragging? (Android SDL starts dragging without preceeding click!)
			is_dragging = true;
			return true;
		}
	}

	// we will handle dragging ourselves inf not prevented
	if(is_dragging  &&  ev->ev_class < INFOWIN) {
		// now drag: scrollbars are not in pixel, but we will scroll one unit per pixels ...
		tab_offset_x -= (ev->mouse_pos.x - ev->click_pos.x);
		tab_offset_x = clamp(tab_offset_x, 0, required_size.w - size.w);
		// and finally end dragging on release of any button
		if (ev->ev_class == EVENT_RELEASE) {
			is_dragging = false;
			if (abs(ev->mouse_pos.x - ev->click_pos.x) >= 5 || abs(ev->click_pos.x - ev->mouse_pos.x) + abs(ev->click_pos.y - ev->mouse_pos.y) >= env_t::scroll_threshold) {
				// dragged a lot => swallow click
				return true;
			}
		}
		else {
			// continue dragging, swallow other events
			return true;
		}
	}

	if(  IS_LEFTRELEASE(ev)  && tab_getroffen(ev->click_pos)  )  {
		// tab selector was hit
		int text_x = (required_size.w>size.w ? D_ARROW_LEFT_WIDTH : 0) + D_H_SPACE - tab_offset_x;
		int k=0;
		for(tab const& i : tabs) {
			if (!i.component->is_visible()) {
				continue;
			}
			if (text_x <= ev->mouse_pos.x && text_x + i.width > ev->mouse_pos.x) {
				// either tooltip or change
				active_tab = k;
				call_listeners((long)active_tab);
				return true;
			}
			k++;
			text_x += i.width;
		}
		return false;
	}

	// navigate among the tabs using shift+tab and tab
	if(  ev->ev_class==EVENT_KEYBOARD  &&  ev->ev_code == SIM_KEYCODE_TAB  ) {
		int di = 1; // tab -> go to the next tab
		if(  IS_SHIFT_PRESSED(ev)  ) {
			// shift+tab -> go to the previous tab
			di = -1;
		}
		// change index by di*i
		for(int i = 1; i<(int)tabs.get_count(); i++) {
			const int next_tab_idx = (active_tab + tabs.get_count() + di*i) % tabs.get_count();
			if (tabs.at(next_tab_idx).component->is_visible()) {
				active_tab = next_tab_idx;
				call_listeners((long)active_tab);
				break;
			}
		}
		return true;
	}

	if(  ev->ev_class == EVENT_KEYBOARD  ||  DOES_WINDOW_CHILDREN_NEED(ev)  ||  get_aktives_tab()->getroffen(ev->mouse_pos)  ||  get_aktives_tab()->getroffen(ev->click_pos)) {
		// active tab was hit
		event_t ev2 = *ev;
		ev2.move_origin(get_aktives_tab()->get_pos());
		return get_aktives_tab()->infowin_event(&ev2);
	}
	return false;
}



void gui_tab_panel_t::draw(scr_coord parent_pos)
{
	// if active tab is invisible, choose the previous visible one
	if (!tabs.at(active_tab).component->is_visible()) {
		for(int i = 1; i<(int)tabs.get_count(); i++) {
			const int next_tab_idx = (active_tab + tabs.get_count() - i) % tabs.get_count();
			if (tabs.at(next_tab_idx).component->is_visible()) {
				active_tab = next_tab_idx;
				break;
			}
		}
	}
	// Position in screen/window
	int xpos = parent_pos.x + pos.x;
	const int ypos = parent_pos.y + pos.y;

	if(  required_size.w>size.w  || tab_offset_x > 0) {
		left.draw( parent_pos+pos );
		right.draw( parent_pos+pos );
		//display_fillbox_wh_clip_rgb(xpos, ypos+required_size.h-1, 10, 1, SYSCOL_TEXT_HIGHLIGHT, true);
		display_fillbox_wh_clip_rgb(xpos, ypos+required_size.h-1, D_ARROW_LEFT_WIDTH, 1, SYSCOL_HIGHLIGHT, true);
		xpos += D_ARROW_LEFT_WIDTH;
	}

	int text_x = xpos + D_H_SPACE;
	int text_y = ypos + (required_size.h - LINESPACE)/2;

	//display_fillbox_wh_clip_rgb(xpos, ypos+required_size.h-1, 4, 1, color_idx_to_rgb(COL_WHITE), true);
	display_fillbox_wh_clip_rgb(xpos, ypos+required_size.h-1, 4, 1, SYSCOL_HIGHLIGHT, true);

	// do not draw under right button
	int xx = required_size.w>get_size().w ? get_size().w-(D_ARROW_LEFT_WIDTH+2+D_ARROW_RIGHT_WIDTH) : get_size().w;
	text_x -= tab_offset_x;

	int i=0;
	for(tab const& iter : tabs) {

		if (!iter.component->is_visible()) {
			continue;
		}
		// set clipping
		PUSH_CLIP_FIT(xpos, ypos, xx, required_size.h);
		// only start drawing here ...
		char const* const text = iter.title;

		if (i != active_tab) {
			// Non active tabs
			display_fillbox_wh_clip_rgb(text_x+1, ypos+2, iter.width-2, 1, SYSCOL_HIGHLIGHT, true);
			display_fillbox_wh_clip_rgb(text_x, ypos+required_size.h-1, iter.width-2, 1, SYSCOL_HIGHLIGHT, true);

			display_vline_wh_clip_rgb(text_x, ypos+3, required_size.h-4, SYSCOL_HIGHLIGHT, true);
			display_vline_wh_clip_rgb(text_x+iter.width-1, ypos+3, required_size.h-4, SYSCOL_SHADOW, true);

			if(text) {
				display_proportional_clip_rgb(text_x+D_H_SPACE, text_y+2, text, ALIGN_LEFT, SYSCOL_TEXT, true);
			}
			else {
				scr_coord_val const y = ypos   - iter.img->get_pic()->y + required_size.h / 2 - iter.img->get_pic()->h / 2 + 1;
				scr_coord_val const x = text_x - iter.img->get_pic()->x + iter.width / 2      - iter.img->get_pic()->w / 2;
//					display_img_blend(iter.img->get_id(), x, y, TRANSPARENT50_FLAG, false, true);
				display_base_img(iter.img->get_id(), x, y, 1, false, true);
			}
		}
		else {
			// Active tab
			display_fillbox_wh_clip_rgb(text_x+1, ypos, iter.width-2, 1, SYSCOL_HIGHLIGHT, true);

			display_vline_wh_clip_rgb(text_x, ypos+1, required_size.h-2, SYSCOL_HIGHLIGHT, true);
			display_vline_wh_clip_rgb(text_x+iter.width-1, ypos+1, required_size.h-2, SYSCOL_SHADOW, true);

			if(text) {
				display_proportional_clip_rgb(text_x+D_H_SPACE, text_y, text, ALIGN_LEFT, SYSCOL_TEXT_HIGHLIGHT, true);
			}
			else {
				scr_coord_val const y = ypos   - iter.img->get_pic()->y + required_size.h / 2 - iter.img->get_pic()->h / 2 - 1;
				scr_coord_val const x = text_x - iter.img->get_pic()->x + iter.width / 2      - iter.img->get_pic()->w / 2;
				display_base_img(iter.img->get_id(), x, y, 1, false, true);
			}
		}
		text_x += iter.width;
		// reset clipping
		POP_CLIP();

		i++;
	}
	display_fillbox_wh_clip_rgb(text_x, ypos+required_size.h-1, xpos+size.w-text_x, 1, SYSCOL_HIGHLIGHT, true);

	// draw tab content after tab row
	// (combobox may open to above, and tab row may draw into it)
	get_aktives_tab()->draw(parent_pos + pos);

	// now for tooltips ...
	int my = get_mouse_pos().y-parent_pos.y-pos.y-6;
	if(my>=0  &&  my < required_size.h-1) {
		// Reiter getroffen?
		int mx = get_mouse_pos().x-parent_pos.x-pos.x;
		int text_x = D_H_SPACE + tab_offset_x;

		for(tab const& iter : tabs) {
			if(text_x <= mx && text_x+iter.width > mx  && (required_size.w<=get_size().w || mx < right.get_pos().x-12)) {
				// tooltip or change
				const scr_coord tooltip_pos{ get_mouse_pos().x + 16, ypos + required_size.h + 12 };
				win_set_tooltip(tooltip_pos, iter.tooltip, &iter, this);
				break;
			}

			text_x += iter.width;
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
