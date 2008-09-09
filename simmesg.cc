/*
 * Manages all gameplay-related messages of the games
 *
 * Copyright (c) 2005 Markus Pristovsek
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "macros.h"
#include "simdebug.h"
#include "simmesg.h"
#include "simticker.h"
#include "simgraph.h"
#include "simcolor.h"
#include "simwin.h"
#include "utils/simstring.h"
#include "tpl/slist_tpl.h"
#include "gui/messagebox.h"



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
	list.clear();
}


/* get flags for message routing */
void message_t::get_message_flags( int *t, int *w, int *a, int  *i)
{
	*t = ticker_flags;
	*w = win_flags;
	*a = auto_win_flags;
	*i = ignore_flags;
}



/* set flags for message routing */
void message_t::set_message_flags( int t, int w, int a, int i)
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
 * @param bild images assosiated with message
 * @author prissi
 */
void message_t::add_message(const char *text, koord pos, msg_typ what, uint8 color, image_id bild )
{
DBG_MESSAGE("message_t::add_msg()","%40s (at %i,%i)", text, pos.x, pos.y );

	int art = (1<<what);
	if(art&ignore_flags) {
		// wants us to ignore this completely
		return;
	}

	// correct for player color
	PLAYER_COLOR_VAL colorval=color;
	if(color<8) {
		colorval = PLAYER_FLAG|(welt->gib_spieler(color)->get_player_color1()+0);
	}

	// should we send this message to a ticker? (always done)
	if(art&ticker_flags) {
		ticker::add_msg(text, pos, colorval);
	}

	// we will not add messages two times to the list if it was within the last 20 messages or within last three months
	sint32 now = welt->get_current_month()-2;
	for (uint i=max(20,list.get_count())-20; i<list.get_count();  i++  ) {
		const node& n = list[i];
		if (n.time >= now &&
				strcmp(n.msg, text) == 0 &&
				(n.pos.x & 0xFFF0) == (pos.x & 0xFFF0) && // positions need not 100% match ...
				(n.pos.y & 0xFFF0) == (pos.y & 0xFFF0)) {
			// we had exactly this message already
			return;
		}
	}

	// we do not allow messages larger than 256 bytes
	node n;

	tstrncpy(n.msg, text, lengthof(n.msg));
	n.pos = pos;
	n.color = colorval;
	n.time = welt->get_current_month();
	n.bild = bild;

	// insert at the top
	list.insert_at(0,n);
	char* p = list[0].msg;
	// should we open an autoclose windows?
	if(art & auto_win_flags) {
		news_window* news;
		if (pos == koord::invalid) {
			news = new news_img(p, bild, colorval);
		} else {
			news = new news_loc(welt, p, pos, colorval);
		}
		create_win(  news, w_time_delete, magic_none );
	}

	// should we open a normal windows?
	if (art & win_flags) {
		news_window* news;
		if (pos == koord::invalid) {
			news = new news_img(p, bild, colorval);
		} else {
			news = new news_loc(welt, p, pos, colorval);
		}
		create_win(-1, -1, news, w_info, magic_none);
	}
}
