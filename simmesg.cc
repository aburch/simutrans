/*
 * Manages all gameplay-related messages of the games
 *
 * Copyright (c) 2005 Markus Pristovsek
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include "macros.h"
#include "simdebug.h"
#include "simmesg.h"
#include "simticker.h"
#include "simgraph.h"
#include "simcolor.h"
#include "simwin.h"
#include "simworld.h"

#include "dataobj/loadsave.h"
#include "dataobj/umgebung.h"
#include "player/simplay.h"
#include "utils/simstring.h"
#include "tpl/slist_tpl.h"
#include "gui/messagebox.h"
#include <string.h>


void message_t::node::rdwr(loadsave_t *file)
{
	file->rdwr_str( msg, lengthof(msg) );
	file->rdwr_long( type );
	pos.rdwr( file );
	file->rdwr_short( color );
	file->rdwr_long( time );
	if(  file->is_loading()  ) {
		bild = IMG_LEER;
	}
}


PLAYER_COLOR_VAL message_t::node::get_player_color(karte_t *welt) const
{
	// correct for player color
	PLAYER_COLOR_VAL colorval = color;
	if(  color&PLAYER_FLAG  ) {
		spieler_t *sp = welt->get_spieler(color&(~PLAYER_FLAG));
		colorval = sp ? PLAYER_FLAG+sp->get_player_color1()+1 : MN_GREY0;
	}
	return colorval;
}


message_t::message_t(karte_t *w)
{
	welt = w;
	ticker_flags = 0xFF7F;	// everything on the ticker only
	win_flags = 0;
	auto_win_flags = 0;
	ignore_flags = 0;
	if(w) {
		win_flags = 256+8;
		auto_win_flags = 128+512;
	}
}


message_t::~message_t()
{
	clear();
}


void message_t::clear()
{
	while (!list.empty()) {
		delete list.remove_first();
	}
	ticker::clear_ticker();
}


/* get flags for message routing */
void message_t::get_message_flags( sint32 *t, sint32 *w, sint32 *a, sint32 *i)
{
	*t = ticker_flags;
	*w = win_flags;
	*a = auto_win_flags;
	*i = ignore_flags;
}



/* set flags for message routing */
void message_t::set_message_flags( sint32 t, sint32 w, sint32 a, sint32 i)
{
	ticker_flags = t;
	win_flags = w;
	auto_win_flags = a;
	ignore_flags = i;
}


/**
 * Add a message to the message list
 * @param pos    position of the event
 * @param color  message color
 * @param where type of message
 * @param bild image associated with message (will be ignored if pos!=koord::invalid)
 * @author prissi
 */
void message_t::add_message(const char *text, koord pos, uint16 what_flags, PLAYER_COLOR_VAL color, image_id bild )
{
DBG_MESSAGE("message_t::add_msg()","%40s (at %i,%i)", text, pos.x, pos.y );

	sint32 what = what_flags & ~local_flag;
	sint32 art = (1<<what);
	if(  art&ignore_flags  ) {
		// wants us to ignore this completely
		return;
	}

	/* we will not add traffic jam messages two times to the list
	 * if it was within the last 20 messages
	 * or within last months
	 * and is not a general (BLACK) message
	 */
	if(  what == traffic_jams  ) {
		sint32 now = welt->get_current_month()-2;
		uint32 i = 0;
		FOR(slist_tpl<node*>, const iter, list) {
			node const& n = *iter;
			if (n.time >= now &&
					strcmp(n.msg, text) == 0 &&
					(n.pos.x & 0xFFF0) == (pos.x & 0xFFF0) && // positions need not 100% match ...
					(n.pos.y & 0xFFF0) == (pos.y & 0xFFF0)) {
				// we had exactly this message already
				return;
			}
			if (++i == 20) break;
		}
	}

	// if no coordinate is provided, there is maybe one in the text message?
	// syntax: either @x,y or (x,y)
	if (pos == koord::invalid) {
		const char *str = text;
		// scan until either @ or ( are found
		while( *(str += strcspn(str, "@(")) ) {
			str += 1;
			int x=-1, y=-1;
			if (sscanf(str, "%d,%d", &x, &y) == 2) {
				if (welt->ist_in_kartengrenzen(x,y)) {
					pos.x = x;
					pos.y = y;
					break; // success
				}
			}
		}
	}

	// we do not allow messages larger than 256 bytes
	node *const n = new node();

	tstrncpy(n->msg, text, lengthof(n->msg));
	n->type = what_flags;
	n->pos = pos;
	n->color = color;
	n->time = welt->get_current_month();
	n->bild = bild;

	PLAYER_COLOR_VAL colorval = n->get_player_color(welt);
	// should we send this message to a ticker?
	if(  art&ticker_flags  ) {
		ticker::add_msg(text, pos, colorval);
	}

	// insert at the top
	list.insert(n);
	char* p = list.front()->msg;

	// if local flag is set and we are not current player, do not open windows
	if(  (art&(1<<ai))==0  &&   (color & PLAYER_FLAG) != 0  &&  welt->get_active_player_nr() != (color&(~PLAYER_FLAG))  ) {
		return;
	}
	// check if some window has focus
	gui_frame_t *old_top = win_get_top();
	gui_komponente_t *focus = win_get_focus();

	// should we open a window?
	if (  art & (auto_win_flags | win_flags)  ) {
		news_window* news;
		if (pos == koord::invalid) {
			news = new news_img(p, bild, colorval);
		} else {
			news = new news_loc(welt, p, pos, colorval);
		}
		wintype w_t = art & win_flags ? w_info /* normal window */ : w_time_delete /* autoclose window */;

		create_win(  news, w_t, magic_none );
	}

	// restore focus
	if(  old_top  &&  focus  ) {
		top_win( old_top, true );
	}
}


void message_t::rotate90( sint16 size_w )
{
	FOR(slist_tpl<node*>, const i, list) {
		i->pos.rotate90(size_w);
	}
}


void message_t::rdwr( loadsave_t *file )
{
	uint16 msg_count;
	if(  file->is_saving()  ) {
		if(  umgebung_t::server  ) {
			// on server: do not save local messages
			msg_count = 0;
			FOR(slist_tpl<node*>, const i, list) {
				if (!(i->type & local_flag)) {
					if (++msg_count == 2000) break;
				}
			}
			file->rdwr_short( msg_count );
			FOR(slist_tpl<node*>, const i, list) {
				if (msg_count == 0) break;
				if (!(i->type & local_flag)) {
					i->rdwr(file);
					msg_count --;
				}
			}
			assert( msg_count == 0 );
		}
		else {
			msg_count = min( 2000u, list.get_count() );
			file->rdwr_short( msg_count );
			FOR(slist_tpl<node*>, const i, list) {
				i->rdwr(file);
				if (--msg_count == 0) break;
			}
		}
	}
	else {
		// loading
		clear();
		file->rdwr_short(msg_count);
		while(  (msg_count--)>0  ) {
			node *n = new node();
			n->rdwr(file);
			list.append(n);
		}
	}
}
