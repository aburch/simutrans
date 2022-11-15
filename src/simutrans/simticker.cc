/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "dataobj/koord3d.h"
#include "dataobj/environment.h"
#include "simticker.h"
#include "display/simgraph.h"
#include "simcolor.h"
#include "tpl/slist_tpl.h"
#include "utils/simstring.h"
#include "gui/gui_theme.h"
#include "world/simworld.h"
#include "simmesg.h"
#include "display/viewport.h"

#define X_DIST    (2)   // how much scrolling per update call?
#define X_SPACING (18)  // spacing between messages, in pixels

static karte_ptr_t welt;

uint16 win_get_statusbar_height(); // simwin.h

struct node : public message_node_t
{
	node() {}
	node(const message_node_t& msg) : message_node_t(msg), xpos(0), w(0) {}
	sint16 xpos;
	sint32 w;
};


static slist_tpl<node> list;
static bool redraw_all = false;            ///< true, if also trigger background need redraw
static int next_pos;                       ///< Next x offset of new message. Always greater or equal to display_width
static int dx_since_last_draw = 0;         ///< Increased during update(); positive values move messages to the left


bool ticker::empty()
{
	return list.empty();
}


void ticker::clear_messages()
{
	list.clear();
	set_redraw_all(true);
}


void ticker::set_redraw_all(bool redraw)
{
	redraw_all = redraw;
}


void ticker::add_msg(const char* txt, koord3d pos, FLAGGED_PIXVAL color)
{
	const int count = list.get_count();

	if(count==0) {
		redraw_all = true;
		next_pos = display_get_width();
	}

	// don't store more than 4 messages, it's useless.
	if(count >= 4) {
		return;
	}
	// Don't repeat messages
	else if (count == 0 || !strstart(list.back().msg, txt)) {
		node n;
		int i=0;

		// remove breaks
		for(  int j=0;  i<250  &&  txt[j]!=0;  j++ ) {
			if(  txt[j]=='\n'  ) {
				if(  i==0  ||  n.msg[i-1]==' '  ) {
					continue;
				}
				n.msg[i++] = ' ';
			}
			else {
				if(  txt[j]==' '  &&  (i==0  ||  n.msg[i-1]==' ')  ) {
					// avoid double or leading spaces
					continue;
				}
				n.msg[i++] = txt[j];
			}
		}
		n.msg[i++] = 0;

		n.pos = pos;
		n.color = color;

		n.xpos = next_pos;
		n.w = proportional_string_width(n.msg);

		// set to default values
		n.type  = message_t::general;
		n.pos   = pos;
		n.time  = 0;
		n.image = IMG_EMPTY;

		next_pos += n.w + X_SPACING;
		list.append(n);
	}
}


void ticker::add_msg_node(const message_node_t& msg)
{
	if (list.empty()) {
		redraw_all = true;
		next_pos = display_get_width();
	}

	node n(msg);
	n.xpos = next_pos;
	n.w = proportional_string_width(n.msg);
	next_pos += n.w + X_SPACING;
	list.append(n);
}


void ticker::update()
{
	const int dx = X_DIST;
	const int display_width = display_get_width();

	for(node & n : list) {
		n.xpos -= dx;
	}

	dx_since_last_draw += dx;
	next_pos = std::max(next_pos - dx, display_width);

	// remove old news
	while (!list.empty()  &&  list.front().xpos + list.front().w < 0) {
		list.remove_first();
	}

	if (list.empty()) {
		set_redraw_all(true);
	}
}


void ticker::draw()
{
	const int start_y = env_t::menupos == MENU_BOTTOM ? win_get_statusbar_height() : display_get_height() - TICKER_HEIGHT - win_get_statusbar_height();
	if (redraw_all) {
		redraw();
		return;
	}
	else if (list.empty()) {
		// ticker not visible

		// mark everything at the bottom as dirty to clear also tooltips and compass
		mark_rect_dirty_wc(0, env_t::menupos == MENU_BOTTOM ? 0 : start_y - 128, display_get_width(), start_y + 128 + TICKER_HEIGHT);
		return;
	}

	const int width = display_get_width();
	if (width <= 0) {
		return;
	}

	// do partial redraw
	display_scroll_band( start_y, dx_since_last_draw, TICKER_HEIGHT );
	display_fillbox_wh_rgb(width-dx_since_last_draw-6, start_y, dx_since_last_draw+6, TICKER_HEIGHT, SYSCOL_TICKER_BACKGROUND, true);

	// ok, ready for the text
	PUSH_CLIP( 0, start_y, width - 1, TICKER_HEIGHT );
	for(node & n : list) {
		if (n.xpos < width) {
			display_proportional_clip_rgb(n.xpos, start_y + TICKER_V_SPACE, n.msg, ALIGN_LEFT, n.color, true);
		}
	}
	POP_CLIP();

	dx_since_last_draw = 0;
}


void ticker::redraw()
{
	set_redraw_all(false);
	dx_since_last_draw = 0;
	const int start_y = env_t::menupos == MENU_BOTTOM ? win_get_statusbar_height() : display_get_height() - TICKER_HEIGHT - win_get_statusbar_height();

	if (list.empty()) {
		// mark everything at the bottom as dirty to clear also tooltips and compass
		mark_rect_dirty_wc(0, env_t::menupos == MENU_BOTTOM ? 0 : start_y - 128, display_get_width(), start_y + 128 + TICKER_HEIGHT);
		world()->set_background_dirty();
		return;
	}

	const int width = display_get_width();

	// just draw the ticker in its colour ... (to be sure ... )
	display_fillbox_wh_rgb(0, start_y, width, TICKER_HEIGHT, SYSCOL_TICKER_BACKGROUND, true);
	for(node & n : list) {
		if (n.xpos < width) {
			display_proportional_clip_rgb(n.xpos, start_y + TICKER_V_SPACE, n.msg, ALIGN_LEFT, n.color, true);
		}
	}
}


void ticker::process_click(int x)
{
	node *clicked = NULL;
	if (list.empty()) {
		return;
	}
	clicked = &list.front();
	if (list.get_count() > 1) {
		for(node & n : list) {
			if (n.xpos <= x  &&  x < n.xpos + n.w ) {
				clicked = &n;
				break;
			}
		}
	}
	if (clicked->pos != koord3d::invalid) {
		if(welt->is_within_limits(clicked->pos.get_2d())) {
			welt->get_viewport()->change_world_position(clicked->pos);
		}
	}
	else {
		clicked->open_msg_window(false);
	}

}
