/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */


#include "../dataobj/environment.h"
#include "../display/simimg.h"
#include "../display/simgraph.h"
#include "../player/simplay.h"
#include "../utils/simstring.h"
#include "../world/simworld.h"
#include "../tool/simmenu.h"
#include "../simskin.h"
#include "gui_frame.h"
#include "simwin.h"
#include "tool_selector.h"

#define MIN_WIDTH (80)


tool_selector_t::tool_selector_t(const char* title, const char *help_file, uint32 toolbar_id, bool allow_break) :
	gui_frame_t( translator::translate(title) ), tools(0)
{
	set_table_layout(0,0); // we do our own positioning of icons (for now)
	this->toolbar_id = toolbar_id;
	this->allow_break = allow_break;
	this->help_file = help_file;
	this->tool_icon_disp_start = 0;
	this->tool_icon_disp_end = 0;
	this->title = title;
	has_prev_next = false;
	is_dragging = false;
	offset = scr_coord( 0, 0 );
	set_windowsize( scr_size(max(env_t::iconsize.w,MIN_WIDTH), D_TITLEBAR_HEIGHT) );
	dirty = true;
}


/**
 * Add a new tool with values and tooltip text.
 * tool_in must be created by new tool_t(copy_tool)!
 */
void tool_selector_t::add_tool_selector(tool_t *tool_in)
{
	image_id tool_img = tool_in->get_icon(welt->get_active_player());
	if(  tool_img == IMG_EMPTY  &&  tool_in!=tool_t::dummy  ) {
		return;
	}

	// only for non-empty icons ...
	tools.append(tool_in);

	int ww = max(2,(display_get_width()/env_t::iconsize.w)-2); // to avoid zero or negative ww on posix (no graphic) backends
	tool_icon_width = tools.get_count();
DBG_DEBUG4("tool_selector_t::add_tool()","ww=%i, tool_icon_width=%i",ww,tool_icon_width);
	if(  allow_break  &&  (ww<tool_icon_width
		||  (env_t::toolbar_max_width>0  &&  env_t::toolbar_max_width<tool_icon_width)
		||  (env_t::toolbar_max_width<0  &&  (ww+env_t::toolbar_max_width)<tool_icon_width))
		) {
		//break them
		int rows = (tool_icon_width/ww)+1;
DBG_DEBUG4("tool_selector_t::add_tool()","ww=%i, rows=%i",ww,rows);
		// assure equal distribution if more than a single row is needed
		tool_icon_width = (tool_icon_width+rows-1)/rows;
		if(  env_t::toolbar_max_width != 0  ) {
			// At least, 3 rows is needed to drag toolbar
			tool_icon_width = min( tool_icon_width, max(env_t::toolbar_max_width, 3) );
		}
	}
	tool_icon_height = max( (display_get_height()/env_t::iconsize.h)-3, 1 );
	if(  env_t::toolbar_max_height > 0  ) {
		tool_icon_height = min(tool_icon_height, env_t::toolbar_max_height);
	}
	dirty = true;
	set_windowsize( scr_size( tool_icon_width*env_t::iconsize.w, min(tool_icon_height, ((tools.get_count()-1)/tool_icon_width)+1)*env_t::iconsize.h+D_TITLEBAR_HEIGHT ) );
	tool_icon_disp_start = 0;
	tool_icon_disp_end = min( tool_icon_disp_start+tool_icon_width*tool_icon_height, tools.get_count() );
	has_prev_next = ((uint32)tool_icon_width*tool_icon_height < tools.get_count());

DBG_DEBUG4("tool_selector_t::add_tool()", "at position %i (width %i)", tools.get_count(), tool_icon_width);
}


// reset the tools to empty state
void tool_selector_t::reset_tools()
{
	tools.clear();
	gui_frame_t::set_windowsize( scr_size(max(env_t::iconsize.w,MIN_WIDTH), D_TITLEBAR_HEIGHT) );
	tool_icon_width = 0;
	tool_icon_disp_start = 0;
	tool_icon_disp_end = 0;
	offset = scr_coord( 0, 0 );
}


bool tool_selector_t::is_hit(int x, int y)
{
	int dx = (x-offset.x)/env_t::iconsize.w;
	int dy = (y-D_TITLEBAR_HEIGHT-offset.y)/env_t::iconsize.h;

	// either click in titlebar or on an icon
	if(  x>=0   &&  y>=0  &&  ( (y<D_TITLEBAR_HEIGHT  &&  x<get_windowsize().w)  ||  (dx<tool_icon_width  &&  dy<tool_icon_height) )  ) {
		return y < D_TITLEBAR_HEIGHT || dx + tool_icon_width * dy + tool_icon_disp_start < (int)tools.get_count();
	}
	return false;
}


bool tool_selector_t::infowin_event(const event_t *ev)
{
	if(  has_prev_next  &&  (IS_LEFTDRAG(ev)  ||  is_dragging)  ) {
		if( !is_dragging ) {
			old_offset = offset;
		}
		// currently only drag in x directions
		is_dragging = true;
		offset = old_offset + (tool_icon_height == 1 ? scr_coord(ev->mx - ev->cx, 0) : scr_coord(0, ev->my - ev->cy));
		int xy = tool_icon_width*tool_icon_height;
		if(  tool_icon_height == 1  &&  tool_icon_disp_start + xy >= (int)tools.get_count()  ) {
			// we have to take into account that the height is not a full icon
			tool_icon_disp_start = max(0, (int)tools.get_count() - xy);
			offset.x = -min(-offset.x, env_t::iconsize.w-(get_windowsize().w % env_t::iconsize.w));
		}
		if(  tool_icon_width == 1  &&  tool_icon_disp_start + xy >= (int)tools.get_count()  ) {
			// we have to take into account that the height is not a full icon
			tool_icon_disp_start = max(0, (int)tools.get_count() - xy);
			offset.y = -min( -offset.y, (get_windowsize().h-D_TITLEBAR_HEIGHT) % env_t::iconsize.h);
		}
		if(  tool_icon_disp_start == 0  &&  (offset.x > 0  ||  offset.y > 0)  ) {
			offset.x = 0;
			offset.y = 0;
		}
		if (offset.x > 0) {
			// we must change the old offset, since the mouse starting point changed!
			old_offset.x -= env_t::iconsize.w;
			offset.x -= env_t::iconsize.w;
			tool_icon_disp_start--;
		}
		if (offset.x <= -env_t::iconsize.w) {
			old_offset.x += env_t::iconsize.w;
			offset.x += env_t::iconsize.w;
			tool_icon_disp_start++;
		}
		if (offset.y > 0) {
			// we must change the old offset, since the mouse starting point changed!
			old_offset.y -= env_t::iconsize.h;
			offset.y -= env_t::iconsize.h;
			tool_icon_disp_start--;
		}
		if (offset.y <= -env_t::iconsize.h) {
			old_offset.y += env_t::iconsize.h;
			offset.y += env_t::iconsize.h;
			tool_icon_disp_start++;
		}
		if(  !IS_LEFTRELEASE(ev)  &&  ev->button_state != 1 ) {
			is_dragging = false;
		}
	}

	// offsets sanity check
	while (offset.x < -env_t::iconsize.w) {
		offset.x += env_t::iconsize.w;
	}
	while (offset.x > 0) {
		offset.x -= env_t::iconsize.w;
	}
	while (offset.y < -env_t::iconsize.h) {
		offset.y += env_t::iconsize.h;
	}
	while (offset.y > 0) {
		offset.y -= env_t::iconsize.h;
	}
	if( tool_icon_disp_start > tool_icon_disp_end ) {
		tool_icon_disp_start = 0;
		offset.x = 0;
		offset.y = 0;
	}
	int xy = tool_icon_width*tool_icon_height;
	tool_icon_disp_end = (tool_icon_height == 1) ? min(tool_icon_disp_start+xy+(offset.x!=0), tools.get_count()) : min(tool_icon_disp_start + xy + (offset.y != 0), tools.get_count());

	if(IS_LEFTRELEASE(ev)  ||  IS_RIGHTRELEASE(ev)) {
		if( is_dragging ) {
			is_dragging = false;
			if( abs(old_offset.x - offset.x) > 2   ||  abs(old_offset.y - offset.y) > 2  ||  ev->cx-ev->mx != offset.x  ||  ev->cx - ev->my != offset.y  ) {
				// we did dragg sucesfully before, so no tool selection!
				return true;
			}
		}

		// No dragging => Next check tooltips
		const int x = (ev->mx-offset.x) / env_t::iconsize.w;
		const int y = (ev->my-offset.y-D_TITLEBAR_HEIGHT) / env_t::iconsize.h;

		const int wz_idx = x+(tool_icon_width*y)+tool_icon_disp_start;
		if( wz_idx>=0  &&  wz_idx < (int)tools.get_count()  ) {
			// change tool
			tool_t *tool = tools[wz_idx].tool;
			if(IS_LEFTRELEASE(ev)) {
				if(  env_t::reselect_closes_tool  &&  tool  &&  tool->is_selected() ) {
					// ->exit triggers tool_selector_t::infowin_event in the closing toolbar,
					// which resets active tool to query tool
					if( tool->exit( welt->get_active_player() ) ) {
						welt->set_tool( tool_t::general_tool[TOOL_QUERY], welt->get_active_player() );
					}
				}
				else {
					welt->set_tool( tool, welt->get_active_player() );
				}
			}
			else {
				// right-click on toolbar icon closes toolbars and dialogues. Resets selectable simple and general tools to the query-tool
				if(  tool  &&  tool->is_selected()  ) {
					// ->exit triggers tool_selector_t::infowin_event in the closing toolbar,
					// which resets active tool to query tool
					if(  tool->exit(welt->get_active_player())  ) {
						welt->set_tool( tool_t::general_tool[TOOL_QUERY], welt->get_active_player() );
					}
				}
			}
			return true;
		}
	}
	// this resets to query-tool, when closing toolsbar - but only for selected general tools in the closing toolbar
	else if(ev->ev_class==INFOWIN &&  ev->ev_code==WIN_CLOSE) {
		for(tool_data_t const i : tools) {
			if (i.tool->is_selected() && i.tool->get_id() & GENERAL_TOOL) {
				welt->set_tool( tool_t::general_tool[TOOL_QUERY], welt->get_active_player() );
				break;
			}
		}
	}
	// reset title, language may have changed
	else if(ev->ev_class==INFOWIN  &&  (ev->ev_code==WIN_TOP  ||  ev->ev_code==WIN_OPEN) ) {
		set_name( translator::translate(title) );
	}

	if(IS_WINDOW_CHOOSE_NEXT(ev)) {
		int xy = tool_icon_width*tool_icon_height;
		if(ev->ev_code==NEXT_WINDOW) {
			assert( xy >= tool_icon_width );
			tool_icon_disp_start += xy;
			if(  tool_icon_disp_start + xy > (int)tools.get_count() ) {
				tool_icon_disp_start = tools.get_count() - xy -1;
			}
			offset = scr_coord( 0, 0 );
		}
		else {
			if(  tool_icon_disp_start > xy  ) {
				tool_icon_disp_start -= xy;
			}
			else {
				tool_icon_disp_start = 0;
			}
			offset = scr_coord( 0, 0 );
		}
		tool_icon_disp_end = min(tool_icon_disp_start+xy, tools.get_count());
		dirty = true;
	}
	return false;
}


void tool_selector_t::draw(scr_coord pos, scr_size sz)
{
	player_t *player = welt->get_active_player();

	if( toolbar_id == 0 ) {
		// checks for main menu (since it can change during changing layout)
		if(env_t::menupos==MENU_TOP || env_t::menupos == MENU_BOTTOM) {
			offset.y = 0;
			allow_break = false;
			tool_icon_width = (display_get_width() + env_t::iconsize.w - 1) / env_t::iconsize.w;
			tool_icon_height = 1; // only single row for title bar
			set_windowsize(sz);
			// check for too large values (acter changing width etc.)
			if (  display_get_width() >= (int)tools.get_count() * env_t::iconsize.w  ) {
				tool_icon_disp_start = 0;
				offset.x = 0;
			}
			else {
				scr_coord_val wx = (tools.get_count() - tool_icon_disp_start + 1) * env_t::iconsize.w + offset.x;
				if (wx < display_get_width()) {
					tool_icon_disp_start = tool_icon_disp_end < tool_icon_height ? 0 : tool_icon_disp_end - tool_icon_width;
					offset.x = display_get_width() - (tools.get_count() - tool_icon_disp_start) * env_t::iconsize.w;
				}
			}
			has_prev_next = (int)tools.get_count() * env_t::iconsize.w > sz.w;
		}
		else {
			offset.x = 0;
			allow_break = false;
			tool_icon_width = 1;
			// only single column for title bar
			tool_icon_height = (display_get_height() - win_get_statusbar_height() + env_t::iconsize.h - 1) / env_t::iconsize.h;
			set_windowsize(scr_size(env_t::iconsize.w, display_get_height() - win_get_statusbar_height()));

			if ( display_get_height() >= (int)tools.get_count() * env_t::iconsize.h  ) {
				tool_icon_disp_start = 0;
				offset.y = 0;
			}
			else {
				scr_coord_val hx = (tools.get_count() - tool_icon_disp_start + 1) * env_t::iconsize.h + offset.y;
				if (hx < display_get_height()) {
					tool_icon_disp_end = tools.get_count();
					tool_icon_disp_start = tool_icon_disp_end < tool_icon_height ? 0 : tool_icon_disp_end - tool_icon_height;
					offset.y = display_get_height() - (tools.get_count() - tool_icon_disp_start) * env_t::iconsize.h;
				}
			}

			has_prev_next = (int)tools.get_count() * env_t::iconsize.h > sz.h;
		}
	}

	for(  uint i = tool_icon_disp_start;  i < tool_icon_disp_end;  i++  ) {
		const image_id icon_img = tools[i].tool->get_icon(player);
#if COLOUR_DEPTH != 0
		const scr_coord_val additional_xoffset = ( (i-tool_icon_disp_start)%(tool_icon_width+(offset.x!=0)) )*env_t::iconsize.w;
		const scr_coord_val additional_yoffset = D_TITLEBAR_HEIGHT+( (i-tool_icon_disp_start)/(tool_icon_width+(offset.x!=0)) )*env_t::iconsize.h;
#else
		const scr_coord_val additional_xoffset = 0;
		const scr_coord_val additional_yoffset = 0;
#endif
		const scr_coord draw_pos = pos + offset + scr_coord(additional_xoffset, additional_yoffset);
		const char *param = tools[i].tool->get_default_param();

		// we don't draw in main menu as it is already made in simwin.cc
		// no background if separator starts with "-b" and has an icon defined
		if(  toolbar_id>0  &&  !(strstart((param==NULL)? "" : param, "-b"))  ) {
			if(  skinverwaltung_t::toolbar_background  &&  skinverwaltung_t::toolbar_background->get_image_id(toolbar_id) != IMG_EMPTY  ) {
				const image_id back_img = skinverwaltung_t::toolbar_background->get_image_id(toolbar_id);
				display_fit_img_to_width( back_img, env_t::iconsize.w );
				display_color_img( back_img, draw_pos.x, draw_pos.y, welt->get_active_player_nr(), false, true );
			}
			else {
				display_fillbox_wh_clip_rgb( draw_pos.x, draw_pos.y, env_t::iconsize.w, env_t::iconsize.h, color_idx_to_rgb(MN_GREY2), false );
			}
		}

		// if there's no image we simply skip, button will be transparent showing toolbar background
		if(  icon_img != IMG_EMPTY  ) {
			bool tool_dirty = dirty  ||  (tools[i].tool->is_selected() ^ tools[i].selected);
			display_fit_img_to_width( icon_img, env_t::iconsize.w );
			display_color_img(icon_img, draw_pos.x, draw_pos.y, player->get_player_nr(), false, tool_dirty);
			tools[i].tool->draw_after( draw_pos, tool_dirty);
			// store whether tool was selected
			tools[i].selected = tools[i].tool->is_selected();
		}
	}

	if( is_dragging ) {
		mark_rect_dirty_wc(pos.x, pos.y+D_TITLEBAR_HEIGHT, pos.x+sz.w, pos.y+sz.h+D_TITLEBAR_HEIGHT );
	}
	else if(  dirty  &&  (tool_icon_disp_end-tool_icon_disp_start < tool_icon_width*tool_icon_height)  ) {
		// mark empty space empty
		mark_rect_dirty_wc(pos.x, pos.y, pos.x + tool_icon_width*env_t::iconsize.w, pos.y + tool_icon_height*env_t::iconsize.h);
	}

	if(  offset.x != 0  &&  tool_icon_disp_start > 0  ) {
		display_color_img(gui_theme_t::arrow_button_left_img[0], pos.x, pos.y + D_TITLEBAR_HEIGHT, 0, false, false);
	}
	if(  offset.y != 0  &&  tool_icon_disp_start > 0  ) {
		display_color_img(gui_theme_t::arrow_button_up_img[0], pos.x, pos.y + D_TITLEBAR_HEIGHT, 0, false, false);
	}
	if(  tool_icon_height == 1  &&  (tool_icon_disp_start+tool_icon_width < tools.get_count()  ||  (-offset.x) < env_t::iconsize.w*tool_icon_width-get_windowsize().w)  ) {
		display_color_img( gui_theme_t::arrow_button_right_img[0], pos.x+sz.w-D_ARROW_UP_WIDTH, pos.y+D_TITLEBAR_HEIGHT, 0, false, false );
	}
	if(  tool_icon_width == 1  &&  (tool_icon_disp_start+tool_icon_height < tools.get_count()  ||  (-offset.y) < env_t::iconsize.h*tool_icon_height-get_windowsize().h)  ) {
		display_color_img(gui_theme_t::arrow_button_down_img[0], pos.x+sz.w-D_ARROW_DOWN_WIDTH, pos.y+D_TITLEBAR_HEIGHT+sz.h-D_ARROW_DOWN_HEIGHT, 0, false, false);
	}

	if(  !is_dragging  ) {
		// tooltips?
		const sint16 mx = get_mouse_x();
		const sint16 my = get_mouse_y();
		if(  is_hit(mx-pos.x, my-pos.y)  &&  (my-pos.y > D_TITLEBAR_HEIGHT)) {
			const sint16 xdiff = (mx - pos.x - offset.x) / env_t::iconsize.w;
			const sint16 ydiff = (my - pos.y-D_TITLEBAR_HEIGHT - offset.y) / env_t::iconsize.h;
			if(  xdiff>=0  &&  xdiff<tool_icon_width  &&  ydiff>=0  ) {
				const int tipnr = xdiff+(tool_icon_width*ydiff)+tool_icon_disp_start;
				if(  tipnr < (int)tool_icon_disp_end  ) {
					const char* tipstr = tools[tipnr].tool->get_tooltip(welt->get_active_player());
					win_set_tooltip(mx+TOOLTIP_MOUSE_OFFSET_X, my+TOOLTIP_MOUSE_OFFSET_Y, tipstr, tools[tipnr].tool, this);
				}
			}
		}
	}

	dirty = false;
	//as we do not call gui_frame_t::draw, we reset dirty flag explicitly
	unset_dirty();
}



bool tool_selector_t::empty(player_t *player) const
{
	for(tool_data_t w : tools) {
		if (w.tool->get_icon(player) != IMG_EMPTY) {
			return false;
		}
	}
	return true;
}
