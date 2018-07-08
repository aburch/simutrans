/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include <string.h>
#include "dataobj/koord.h"
#include "simdebug.h"
#include "simticker.h"
#include "display/simgraph.h"
#include "simcolor.h"
#include "tpl/slist_tpl.h"
#include "utils/simstring.h"
#include "gui/gui_frame.h"


// how much scrolling per call?
#define X_DIST 2

uint16 win_get_statusbar_height(); // simwin.h

uint16 TICKER_YPOS_BOTTOM;

struct node {
	char msg[256];
	koord pos;
	FLAGGED_PIXVAL color;
	sint16 xpos;
	sint32 w;
};


static slist_tpl<node> list;
static koord default_pos = koord::invalid;
static bool redraw_all; // true, if also trigger background need redraw
static int next_pos;


bool ticker::empty()
{
  return list.empty();
}


void ticker::clear_ticker()
{
	if (!list.empty()) {
		const int height = display_get_height();
		const int width  = display_get_width();
		mark_rect_dirty_wc(0, height-TICKER_YPOS_BOTTOM, width, height);
	}
	list.clear();
}

void ticker::set_redraw_all(const bool b)
{
	redraw_all=b;
}

void ticker::add_msg(const char* txt, koord pos, FLAGGED_PIXVAL color)
{
	// don't store more than 4 messages, it's useless.
	const int count = list.get_count();

	if(count==0) {
		redraw_all = true;
		next_pos = display_get_width();
		default_pos = koord::invalid;
	}

	if(count < 4) {
		// Don't repeat messages
		if (count == 0 || !strstart(list.back().msg, txt)) {
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

			next_pos += n.w + 18;
			list.append(n);
		}
	}
}


koord ticker::get_welt_pos()
{
	return default_pos;
}


void ticker::draw()
{
	TICKER_YPOS_BOTTOM = TICKER_HEIGHT + win_get_statusbar_height();

	if (!list.empty()) {

		const int start_y=display_get_height()-TICKER_YPOS_BOTTOM;
		const int width = display_get_width();

		// redraw whole ticker
		if(redraw_all) {
			redraw_ticker();
		}
		// redraw ticker partially
		else {
			display_scroll_band( start_y+1, X_DIST, TICKER_HEIGHT-1 );
			display_fillbox_wh_rgb(width-X_DIST-6, start_y+1, X_DIST+6, TICKER_HEIGHT-1, SYSCOL_TICKER_BACKGROUND, true);
			// ok, ready for the text
			PUSH_CLIP( 0, start_y + 1, width - 1, TICKER_HEIGHT-1 );
			FOR(slist_tpl<node>, & n, list) {
				n.xpos -= X_DIST;
				if (n.xpos < width) {
					display_proportional_clip_rgb(n.xpos, start_y + 2, n.msg, ALIGN_LEFT, n.color, true);
					default_pos = n.pos;
				}
			}
			POP_CLIP();
		}

		// remove old news
		while (!list.empty()  &&  list.front().xpos + list.front().w < 0) {
			list.remove_first();
		}
		if (list.empty()) {
			// old: mark_rect_dirty_wc(0, start_y, width, start_y + TICKER_HEIGHT);
			// now everything at the bottom to clear also tooltips and compass
			mark_rect_dirty_wc(0, start_y-128, width, start_y + 128 +TICKER_HEIGHT);
		}
		if(next_pos>width) {
			next_pos -= X_DIST;
		}

		redraw_all = false;
	}
}



// complete redraw (after resizing)
void ticker::redraw_ticker()
{
	TICKER_YPOS_BOTTOM = TICKER_HEIGHT + win_get_statusbar_height();

	if (!list.empty()) {
		const int start_y=display_get_height()-TICKER_YPOS_BOTTOM;
		const int width = display_get_width();

		// just draw the ticker in its colour ... (to be sure ... )
		display_fillbox_wh_rgb(0, start_y+1, width, TICKER_HEIGHT-1, SYSCOL_TICKER_BACKGROUND, true);
		FOR(slist_tpl<node>, & n, list) {
			n.xpos -= X_DIST;
			if (n.xpos < width) {
				display_proportional_clip_rgb(n.xpos, start_y + 2, n.msg, ALIGN_LEFT, n.color, true);
				default_pos = n.pos;
			}
		}
	}
}
