/*
 * Scrollable list.
 * Displays list, scrollbuttons up/down, dragbar.
 * Has a min and a max size, and can be displayed with any size in between
 * Does ONLY cater for vertical offset (yet).
 * two possible types:
 * -list.      simply lists some items.
 * -selection. is a list, but additionally, one item can be selected.
 * @author Niels Roest, additions by Hj. Malthaner
 */

#include <algorithm>
#include <stdio.h>

#include "gui_scrollbar.h"
#include "gui_scrolled_list.h"

#include "../simwin.h"

#include "../../display/simgraph.h"
#include "../../simcolor.h"
#include "../../descriptor/skin_desc.h"
#include "../../simskin.h"


// help for sorting
bool  gui_scrolled_list_t::const_text_scrollitem_t::compare( scrollitem_t *aa, scrollitem_t *bb )
{
	const_text_scrollitem_t* a = dynamic_cast<const_text_scrollitem_t*>(aa);
	const_text_scrollitem_t* b = dynamic_cast<const_text_scrollitem_t*>(bb);
	assert(a != NULL  &&  b != NULL);
	return strcmp(a->get_text(), b->get_text() );
}


bool  gui_scrolled_list_t::const_text_scrollitem_t::sort( vector_tpl<scrollitem_t *>&v, int offset, void * ) const
{
	vector_tpl<scrollitem_t *>::iterator start = v.begin();
	while(  offset-->0  ) {
		++start;
	}
	std::sort( start, v.end(), const_text_scrollitem_t::compare );
	return true;
}


// draws a single line of text
scr_coord_val gui_scrolled_list_t::const_text_scrollitem_t::draw( scr_coord pos, scr_coord_val w, bool selected, bool focus )
{
	if(selected) {
		// selected element
		display_fillbox_wh_clip_rgb( pos.x+3, pos.y-1, w-5, get_height()+1, (focus ? SYSCOL_LIST_BACKGROUND_SELECTED_F : SYSCOL_LIST_BACKGROUND_SELECTED_NF), true);
		return display_proportional_clip_rgb( pos.x+7, pos.y, get_text(), ALIGN_LEFT, (focus ? SYSCOL_LIST_TEXT_SELECTED_FOCUS : SYSCOL_LIST_TEXT_SELECTED_NOFOCUS), true);
	}
	else {
		// normal text
		return display_proportional_clip_rgb( pos.x+7, pos.y, get_text(), ALIGN_LEFT, get_color(), true);
	}
}


gui_scrolled_list_t::gui_scrolled_list_t(enum type type) :
	gui_component_t(true),
	sb(scrollbar_t::vertical)
{
	this->type = type;
	selection = -1; // nothing
	size = scr_size(0,0);
	pos = scr_coord(0,0);
	offset = 0;
	border = 0;
	if(  type==windowskin  ) {
		border = 1;
	}
	else {
		assert(  type==listskin  );
		border = 2;
	}
	sb.add_listener(this);
	sb.set_knob_offset(0);
	sb.set_visible_mode( scrollbar_t::show_auto );

	clear_elements();
}


bool gui_scrolled_list_t::action_triggered( gui_action_creator_t * /* comp */, value_t extra)
{
	// search/replace all offsets with sb.get_offset() is also an option
	offset = extra.i;
	return true;
}


// set the scrollbar offset, so that the selected item is visible
void gui_scrolled_list_t::show_selection(int sel)
{
	if(  (unsigned)sel<item_list.get_count()  ) {
		int s = 0;
		for(  int i=0;  i<sel;  s += item_list[i]->get_height(), i++  ) {
		}
DBG_MESSAGE("gui_scrolled_list_t::show_selection()","sel=%d, offset=%d, size.h=%d",s,offset,size.h);
		if(  s < offset  ||  (s+item_list[sel]->get_height()) > offset+size.h  ) {
			// outside range => reposition
			sb.set_knob_offset( max(0,s-(size.h/2) ) );
			offset = sb.get_knob_offset();
		}
		selection = sel;
	}
}


void gui_scrolled_list_t::clear_elements()
{
	for(  uint32 i=0;  i<item_list.get_count();  i++  ) {
		delete item_list[i];
	}
	item_list.clear();
	selection = -1;
	offset = 0;
	total_vertical_size = 0;
	adjust_scrollbar();
}


void gui_scrolled_list_t::append_element( scrollitem_t *item )
{
	item_list.append( item );
	total_vertical_size += item->get_height();
	adjust_scrollbar();
}


void gui_scrolled_list_t::insert_element( scrollitem_t *item )
{
	item_list.insert_at( 0, item );
	if(  selection >=0 ) {
		selection ++;
	}
	total_vertical_size += item->get_height();
	adjust_scrollbar();
}


void gui_scrolled_list_t::sort( int offset, void *sort_param )
{
	if(  item_list.get_count() > 1  ) {
		scrollitem_t *sel = NULL;
		if(  selection>=offset  ) {
			if(  selection >=0  &&  (uint32)selection < item_list.get_count()  ) {
				sel = item_list[selection];
			}
			else {
				selection = -1;
			}
		}
		if (offset >=0  &&  (uint32)offset < item_list.get_count()) {
			item_list[offset]->sort( item_list, offset, sort_param );
		}
		// now we may need to update the selection
		if(  sel  ) {
			for(  uint32 i=offset;  i<item_list.get_count();  i++  ) {
				if(  item_list[i] == sel  ) {
					selection = i;
					show_selection(selection);
					return;
				}
			}
		}
	}
}


// minimum vertical size
// no less than 3, must be room for scrollbuttons
#define YMIN ((LINESPACE*3)+2)

// sets size: if requested is too large, then the size is adjusted
// use this only for variable sized lists
scr_size gui_scrolled_list_t::request_size(scr_size request)
{
	scr_size size = get_size();

	size.w = request.w;
	int y = request.h;

	if(  y > total_vertical_size  ) {
		y = total_vertical_size;
	}

	if(  y < YMIN  ) {
		y = YMIN;
	}

	size.h = y + border;

	set_size( size );

	return size;
}


void gui_scrolled_list_t::set_size(scr_size size)
{
	gui_component_t::set_size(size);
	adjust_scrollbar();
	offset = sb.get_knob_offset();
}


/* resizes scrollbar */
void gui_scrolled_list_t::adjust_scrollbar()
{
	sb.set_pos( scr_coord(size.w-D_SCROLLBAR_WIDTH,0) );
	sb.set_size( scr_size( D_SCROLLBAR_WIDTH, (int)size.h + border - 1) );
	sb.set_knob( size.h - border, total_vertical_size );
}


bool gui_scrolled_list_t::infowin_event(const event_t *ev)
{
	const int x = ev->mx;
	const int y = ev->my;

	const int boarder_w = border / 2;

	if(  sb.is_visible()  &&  sb.getroffen(x,y)  ) {
		event_t new_ev = *ev;
		translate_event( &new_ev, -sb.get_pos().x, -sb.get_pos().y );
		if (sb.infowin_event( &new_ev ) ) {
			// nothing to update for parent ...
			return true;
		}
	}

	// size without scrollbar
	const int w = size.w - D_SCROLLBAR_WIDTH+2;
	const int h = size.h;
	if(x <= w) { // inside list
		bool notify = true;
		if(  x>=boarder_w && x<(w-boarder_w) &&  y>=boarder_w && y<(h-boarder_w)  ) {
			// translate y to list space
			const int list_y = y - boarder_w + offset;

			int new_selection = -1;

			if(  list_y >= 0  ) {
				// locate the item which was pressed by searching down the list
				const int list_count = (int)item_list.get_count();
				int selection_h = 0;
				int i = 0;
				for(  ; i < list_count ; i++  ) {
					selection_h += item_list[i]->get_height();
					if(  selection_h > list_y  ) {
						break;
					}
				}

				// if an item was pressed then select and trigger it
				if(  i < list_count  ) {
					new_selection = i;
					event_t new_ev = *ev;
					translate_event( &new_ev, -boarder_w, offset + item_list[new_selection]->get_height() - selection_h - boarder_w);
					notify = !item_list[new_selection]->infowin_event( &new_ev );
				}
			}

			// update selection on release
			if(  IS_LEFTRELEASE(ev)  ) {
				selection = new_selection;
				if(  notify  ) {
					// not handled oneself
					call_listeners((long)new_selection);
				}
				return true;
			}
		}
	}

	// goto next/previous choice
	if(  ev->ev_class == EVENT_KEYBOARD  &&  (ev->ev_code==SIM_KEY_UP  ||  ev->ev_code==SIM_KEY_DOWN)  ) {
		int new_selection = (ev->ev_code==SIM_KEY_DOWN) ? min(item_list.get_count()-1, selection+1) : max(0, selection-1);
		selection = new_selection;
		show_selection(selection);

		event_t new_ev;
		new_ev.ev_class = EVENT_KEYBOARD;
		new_ev.ev_code = 0;
		new_ev.button_state = 0;
		if(  new_selection >= 0  ||  !item_list[new_selection]->infowin_event( &new_ev )  ) {
			call_listeners((long)new_selection);
		}
		return true;
	}

	if(  sb.is_visible()  &&  (IS_WHEELUP(ev)  ||  IS_WHEELDOWN(ev))  ) {
		event_t ev2 = *ev;
		translate_event(&ev2, -sb.get_pos().x, -sb.get_pos().y);
		return sb.infowin_event(&ev2);
	}

	return false;
}


void gui_scrolled_list_t::draw(scr_coord pos)
{
	pos += this->pos;

	const scr_size size = get_size();

	const int show_scrollbar = offset>0  ||  (size.h<=total_vertical_size-border);
	const int x = pos.x;
	const int y = pos.y;
	const int w = size.w-D_SCROLLBAR_WIDTH*show_scrollbar;
	const int h = size.h;

	switch(type) {
		case windowskin:
			display_img_stretch( gui_theme_t::windowback, scr_rect( x, y, w, h ) );
			break;
		case listskin:
			display_img_stretch( gui_theme_t::listbox, scr_rect( x, y, w, h ) );
			break;
	}

	PUSH_CLIP(x+1,y+1,w-2,h-2);

	int ycum = y+2-offset; // y cumulative
	int i=0;
	const bool focus = win_get_focus()==this;
	scr_coord_val max_w = 0;
	for(  vector_tpl<scrollitem_t*>::iterator iter = item_list.begin();  iter != item_list.end();  ) {
		scrollitem_t* const item = *iter;
		if(  !item->is_valid()  ) {
			iter = item_list.erase(iter);
			delete item;
			if(i == selection) {
				selection = -1;
			}
			else if(  i<selection  ) {
				selection --;
			}
		}
		else {
			scr_coord_val this_w = item->draw( scr_coord( x, ycum), w, i == selection, focus );
			if(  this_w > max_w  ) {
				max_w = this_w;
			}
			ycum += item->get_height();
			++iter;
			i++;
		}
	}
	POP_CLIP();

	if(  show_scrollbar  ) {
		sb.draw(pos);
	}
}
