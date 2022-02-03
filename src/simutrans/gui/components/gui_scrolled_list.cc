/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "gui_scrollbar.h"
#include "gui_scrolled_list.h"

#include "../simwin.h"

#include "../../display/simgraph.h"
#include "../../descriptor/skin_desc.h"
#include "../../simskin.h"


// help for sorting
bool  gui_scrolled_list_t::scrollitem_t::compare(const gui_component_t *aa, const gui_component_t *bb )
{
	const scrollitem_t* a = dynamic_cast<const scrollitem_t*>(aa);
	const scrollitem_t* b = dynamic_cast<const scrollitem_t*>(bb);
	assert(a  &&  b  &&  a->get_text() != NULL  &&  b->get_text() != NULL);
	return strcmp(a->get_text(), b->get_text() );
}


scr_size gui_scrolled_list_t::const_text_scrollitem_t::get_min_size() const
{
	if (!is_editable()) {
		const char* text = get_text();
		return scr_size(2*D_H_SPACE + (text ? display_calc_proportional_string_len_width(text,strlen(text)) : D_BUTTON_WIDTH), LINESPACE);
	}
	else {
		return scr_size(D_BUTTON_WIDTH, LINESPACE);
	}
}

scr_size gui_scrolled_list_t::const_text_scrollitem_t::get_max_size() const
{
//	if (!is_editable()) {
//		return get_min_size();
//	}
//	else {
		return scr_size(scr_size::inf.w, LINESPACE);
//	}
}

// draws a single line of text
void gui_scrolled_list_t::const_text_scrollitem_t::draw(scr_coord pos)
{
	pos += get_pos();
	if(selected) {
		// selected element
		display_fillbox_wh_clip_rgb( pos.x+D_H_SPACE/2, pos.y-1, get_size().w-D_H_SPACE, get_size().h + 1, (focused ? SYSCOL_LIST_BACKGROUND_SELECTED_F : SYSCOL_LIST_BACKGROUND_SELECTED_NF), true);
		display_proportional_clip_rgb( pos.x+D_H_SPACE, pos.y, get_text(), ALIGN_LEFT, (focused ? SYSCOL_LIST_TEXT_SELECTED_FOCUS : SYSCOL_LIST_TEXT_SELECTED_NOFOCUS), true);
	}
	else {
		// normal text
		display_proportional_clip_rgb( pos.x+D_H_SPACE, pos.y, get_text(), ALIGN_LEFT, get_color(), true);
	}
}


gui_scrolled_list_t::gui_scrolled_list_t(enum type type, item_compare_func cmp) :
	gui_scrollpane_t(NULL, true),
	item_list(container.get_components())
{
	container.set_table_layout(1,0);
	container.set_margin( scr_size( D_H_SPACE, D_V_SPACE ), scr_size( D_H_SPACE, D_V_SPACE ) );
	container.set_spacing( scr_size( D_H_SPACE, 0 ) );

	set_component(&container);

	this->type = type;
	compare = cmp;
	size = scr_size(0,0);
	pos = scr_coord(0,0);
	multiple_selection = false;
	maximize = false;
}


// set the scrollbar offset, so that the selected item is visible
void gui_scrolled_list_t::show_selection(int sel)
{
	set_selection(sel);
	cleanup_elements();
	show_focused();
}


void gui_scrolled_list_t::set_selection(int s)
{
	if (s<0  ||  ((uint32)s)>=item_list.get_count()) {
		container.set_focus(NULL);
		return;
	}
	gui_component_t* new_focus = item_list[s];

	// reset selected status
	for(gui_component_t* v : item_list) {
		scrollitem_t* item = dynamic_cast<scrollitem_t*>(v);
		if(  item  ) {
			item->selected = item==new_focus;
		}
	}
	container.set_focus(new_focus);
}


sint32 gui_scrolled_list_t::get_selection() const
{
	scrollitem_t* focus = get_selected_item();
	return focus  ? item_list.index_of(focus) : -1;
}

gui_scrolled_list_t::scrollitem_t* gui_scrolled_list_t::get_selected_item() const
{
	scrollitem_t* focus = dynamic_cast<scrollitem_t*>( comp->get_focus() );
	return focus  &&  item_list.is_contained(focus) ? focus : NULL;
}

vector_tpl<sint32> gui_scrolled_list_t::get_selections() const
{
	vector_tpl<sint32> selections;
	for(  uint32 i=0;  i<item_list.get_count();  i++  ) {
		scrollitem_t* item = dynamic_cast<scrollitem_t*> (item_list[i]);
		if(  item  &&  item->selected  ) {
			selections.append(i);
		}
	}
	return selections;
}


void gui_scrolled_list_t::clear_elements()
{
	container.remove_all();
}


void gui_scrolled_list_t::sort( int offset )
{
	cleanup_elements();

	if (compare == 0  ||  item_list.get_count() <= 1) {
		reset_container_size();
		return;
	}

	if (offset >=0  &&  (uint32)offset < item_list.get_count()) {
		vector_tpl<gui_component_t *>::iterator start = item_list.begin();
		for(int i=0; i<offset; i++) {
			++start;
		}
		std::sort( start, item_list.end(), compare);
	}
	reset_container_size();
}


void gui_scrolled_list_t::set_size(scr_size size)
{
	cleanup_elements();

	gui_scrollpane_t::set_size(size);

	// set all elements in list to same width
	scr_coord_val width = 0;
	for(  vector_tpl<gui_component_t*>::iterator iter = item_list.begin();  iter != item_list.end();  ++iter) {
		width = max( (*iter)->get_size().w, width);
	}

	for(  vector_tpl<gui_component_t*>::iterator iter = item_list.begin();  iter != item_list.end();  ++iter) {
		(*iter)->set_size( scr_size(width, (*iter)->get_size().h));
	}
}


void gui_scrolled_list_t::reset_container_size()
{
	// reset element positioning
	container.set_margin( scr_size( D_H_SPACE, 0 ), scr_size( D_H_SPACE, 0 ) );
	container.set_spacing( scr_size( D_H_SPACE, 0 ) );

	scr_size csize = container.get_min_size();

	container.set_size( csize );
}


bool gui_scrolled_list_t::infowin_event(const event_t *ev)
{
	scrollitem_t* focus = dynamic_cast<scrollitem_t*>( comp->get_focus() );

	event_t ev2 = *ev;
	// translate key up/down to tab/shift-tab
	if(  ev->ev_class==EVENT_KEYBOARD  && ev->ev_code == SIM_KEY_UP  &&  get_selection()>0) {
		ev2.ev_code = SIM_KEY_TAB;
		ev2.ev_key_mod |= SIM_MOD_SHIFT;
	}
	if(  ev->ev_class==EVENT_KEYBOARD  && ev->ev_code == SIM_KEY_DOWN  &&  (uint32)(get_selection()+1) < item_list.get_count()) {
		ev2.ev_code = SIM_KEY_TAB;
		ev2.ev_key_mod &= ~SIM_MOD_SHIFT;
	}

	bool swallowed = gui_scrollpane_t::infowin_event(&ev2);
	scrollitem_t* const new_focus = dynamic_cast<scrollitem_t*>( comp->get_focus() );

	// if different element is focused, calculate selection and call listeners
	if (  focus != new_focus  ) {
		calc_selection(focus, new_focus, *ev);
		int new_selection = get_selection();
		call_listeners((long)new_selection);
		swallowed = true;
	}

	return swallowed;
}


void gui_scrolled_list_t::calc_selection(scrollitem_t* old_focus, scrollitem_t* new_focus, event_t ev)
{
	if(  !new_focus  ) {
		// do nothing.
		return;
	}
	else if(  !old_focus  ||  !multiple_selection  ||  ev.ev_key_mod==0  ) {
		// simply select new_focus
		for(gui_component_t* v : item_list) {
			scrollitem_t* item = dynamic_cast<scrollitem_t*>(v);
			if(  item  ) {
				item->selected = item==new_focus;
			}
		}
	}
	else if(  IS_CONTROL_PRESSED(&ev)  ) {
		// control key is pressed. select or deselect the focused one.
		new_focus->selected = !new_focus->selected;
	}
	else if(  IS_SHIFT_PRESSED(&ev)  ) {
		// shift key is pressed.
		sint32 old_idx = item_list.index_of(old_focus);
		sint32 new_idx = item_list.index_of(new_focus);
		if(  old_idx==-1  ||  new_idx==-1  ) {
			// out of index!?
			return;
		}
		const bool sel = !new_focus->selected;
		for(  sint32 i=min(old_idx,new_idx);  i<=max(old_idx,new_idx);  i++  ) {
			scrollitem_t* item = dynamic_cast<scrollitem_t*>(item_list[i]);
			if(  item  &&  i!=old_idx  ) {
				item->selected = sel;
			}
		}
	}
}


void gui_scrolled_list_t::cleanup_elements()
{
	bool reset = false;
	for(  vector_tpl<gui_component_t*>::iterator iter = item_list.begin();  iter != item_list.end();  ) {
		if (scrollitem_t* const item = dynamic_cast<scrollitem_t*>( *iter ) ) {
			if(  !item->is_valid()  ) {
				container.remove_component(item);
				reset = true;
			}
			else {
				++iter;
			}
		}
	}
	if (reset) {
		reset_container_size();
	}
}


void gui_scrolled_list_t::draw(scr_coord offset)
{
	// set focus
	scrollitem_t* focus = dynamic_cast<scrollitem_t*>( comp->get_focus() );
	if(  focus  ) {
		for(gui_component_t* v : item_list) {
			scrollitem_t* item = dynamic_cast<scrollitem_t*>(v);
			if(  item  ) {
				item->focused = item->selected  &&  win_get_focus()==focus;
			}
		}
	}
	cleanup_elements();

	if (item_list.get_count() > 0) {
		scr_rect rect(pos + offset, get_size());
		switch(type) {
			case windowskin:
				display_img_stretch( gui_theme_t::windowback, rect);
				break;
			case listskin:
				display_img_stretch( gui_theme_t::listbox, rect);
				break;
			case transparent:
				break;

		}
	}

	gui_scrollpane_t::draw(offset);
}
