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
#include "gui/simwin.h"
#include "gui/chat_frame.h"
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


void ticker::add_msg_node(const message_node_t& msg)
{
	const int count = list.get_count();

	if(count==0) {
		redraw_all = true;
		next_pos = gfx->get_screen_size().w;
	}

	const char* txt = msg.msg;
	// don't store more than 4 messages, it's useless.
	if(count >= 4) {
		return;
	}
	// Don't repeat messages
	else if (count == 0 || !strstart(list.back().msg, txt)) {
		node n(msg);
		int i=0;

		// copy message text
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

		n.xpos = next_pos;
		n.w = gfx->calc_text_width(n.msg);

		next_pos += n.w + X_SPACING;
		list.append(n);
	}
}


void ticker::add_msg(const char* txt, koord3d pos, FLAGGED_PIXVAL color, int type)
{
	node n;
	tstrncpy(n.msg, txt, lengthof(n.msg));
	n.pos = pos;
	n.color = color;
	// set to default values
	n.type = message_t::general;
	n.time = 0;
	n.image = IMG_EMPTY;
	n.type = type;

	add_msg_node(n);
}


void ticker::update()
{
	const int dx = X_DIST;
	const scr_size screen = gfx->get_screen_size();

	for(node & n : list) {
		n.xpos -= dx;
	}

	dx_since_last_draw += dx;
	next_pos = std::max(next_pos - dx, screen.w);

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
	const scr_size screen = gfx->get_screen_size();
	const int start_y = env_t::menupos == MENU_BOTTOM ? win_get_statusbar_height() : screen.h - TICKER_HEIGHT - win_get_statusbar_height();
	if (redraw_all) {
		redraw();
		return;
	}
	else if (list.empty()) {
		// ticker not visible

		// mark everything at the bottom as dirty to clear also tooltips and compass
		gfx->mark_rect_dirty_wc(0, env_t::menupos == MENU_BOTTOM ? 0 : start_y - 128, screen.w, start_y + 128 + TICKER_HEIGHT);
		redraw();
		return;
	}
	if (dx_since_last_draw <=0) {
		return;
	}

	if (screen.w <= 0) {
		return;
	}

	// do partial redraw
	gfx->move_scroll_band(start_y, dx_since_last_draw, TICKER_HEIGHT);

	gfx->draw_rect(screen.w-dx_since_last_draw, start_y, dx_since_last_draw, TICKER_HEIGHT, SYSCOL_TICKER_BACKGROUND, true);

	gfx->mark_rect_dirty_wc(0, start_y, screen.w, start_y + TICKER_HEIGHT);

	// ok, ready for the text
	PUSH_CLIP( screen.w-dx_since_last_draw, start_y, dx_since_last_draw, TICKER_HEIGHT );
	for(node & n : list) {
		if (n.xpos < screen.w) {
			gfx->draw_text_clipped(n.xpos, start_y + TICKER_V_SPACE, n.msg, ALIGN_LEFT, n.get_player_color(welt), true);
		}
	}
	POP_CLIP();

	dx_since_last_draw = 0;
}


void ticker::redraw()
{
	const scr_size screen = gfx->get_screen_size();
	set_redraw_all(false);
	dx_since_last_draw = 0;
	const int start_y = env_t::menupos == MENU_BOTTOM ? win_get_statusbar_height() : screen.h - TICKER_HEIGHT - win_get_statusbar_height();

	if (list.empty()) {
		// mark everything at the bottom as dirty to clear also tooltips and compass
		gfx->mark_rect_dirty_wc(0, env_t::menupos == MENU_BOTTOM ? 0 : start_y - 128, screen.h, start_y + 128 + TICKER_HEIGHT);
		world()->set_background_dirty();
		return;
	}

	// just draw the ticker in its colour ... (to be sure ... )
	gfx->draw_rect(0, start_y, screen.w, TICKER_HEIGHT, SYSCOL_TICKER_BACKGROUND, true);
	for(node & n : list) {
		if (n.xpos < screen.w) {
			gfx->draw_text_clipped(n.xpos, start_y + TICKER_V_SPACE, n.msg, ALIGN_LEFT, n.get_player_color(welt), true);
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
	if ((clicked->type & message_t::MESSAGE_TYPE_MASK) >= message_t::chat_common  &&  (clicked->type & message_t::MESSAGE_TYPE_MASK) <= message_t::chat_private) {
		chat_frame_t* si = (chat_frame_t*)win_get_magic(magic_chatframe);
		if (si == NULL) {
			si = new chat_frame_t();
			create_win({ 0, 200 }, si, w_info, magic_chatframe);
		}
		si->activate_tab((clicked->type & message_t::MESSAGE_TYPE_MASK) - message_t::chat_common);
	}
	else if (clicked->pos != koord3d::invalid) {
		if(welt->is_within_limits(clicked->pos.get_2d())) {
			welt->get_viewport()->change_world_position(clicked->pos);
		}
	}
	else {
		clicked->open_msg_window(false);
	}

}
