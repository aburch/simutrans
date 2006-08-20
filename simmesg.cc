/*
 * simmesg.cc
 *
 * Manages all gameplay-related messages of the games
 *
 * Copyright (c) 2005 Markus Pristovsek
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include <string.h>
#include <stdlib.h>

#include "simdebug.h"
#include "simmesg.h"
#include "simticker.h"
#include "simtime.h"
#include "simgraph.h"
#include "simcolor.h"
#include "simmem.h"
#include "simwin.h"

#include "tpl/slist_tpl.h"
#include "gui/messagebox.h"



message_t * message_t::single_instance= NULL;



message_t * message_t::get_instance()
{
	if(single_instance==NULL) {
dbg->fatal("message_t::message_t()","Init without world!");
		single_instance = new message_t(NULL);
	}
	return single_instance;
}



message_t::message_t(karte_t *w)
{
	// only a singe instance of this messenger
	if(single_instance!=NULL) {
DBG_MESSAGE("message_t::message_t()","previous instance %p");
		return;
	}
	single_instance = this;
	welt = w;

	ticker_flags = 0xFF7F;	// everything on the ticker
	win_flags = 0;
	auto_win_flags = 0;
	ignore_flags = 0;
	if(w) {
		win_flags = 256+8;
		auto_win_flags = 128;
	}

	list = new slist_tpl <message_t::node>;
}



message_t::~message_t()
{
DBG_MESSAGE("message_t::~message_t()","previous instance %p");
	// free lists
	delete list;
	list = 0;
	single_instance = NULL;
}


/* get flags for message routing */
void message_t::get_flags( int *t, int *w, int *a, int  *i)
{
	*t = ticker_flags;
	*w = win_flags;
	*a = auto_win_flags;
	*i = ignore_flags;
}



/* set flags for message routing */
void message_t::set_flags( int t, int w, int a, int i)
{
	ticker_flags = t;
	win_flags = w;
	auto_win_flags = a;
	ignore_flags = i;
}




int message_t::gib_count() const
{
  return list->count();
}



message_t::node *
message_t::get_node(unsigned i)
{
	if(list->count()>i) {
		return &(list->at(i));
	}
	return NULL;
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
		colorval = PLAYER_FLAG|((welt->gib_spieler(color)->get_player_color()*4)+0);
	}

      // should we send this message to a ticker? (always done)
      if(art&ticker_flags) {
		ticker_t::get_instance()->add_msg(text,pos,colorval);
      }

	// we will not add messages two times to the list if it was within the last 20 messages or within last three months
	sint32 now = welt->get_current_month()-2;
	for(unsigned i=0;  i<list->count()  &&  i<20;  i++) {
		if(  list->at(i).time>=now  &&  list->at(i).pos==pos  &&  strcmp(list->at(i).msg,text)==0  ) {
			// we had exactly this message already
			return;
		}
	}

	// we do not allow messages larger than 256 bytes
      node n;

	strncpy( n.msg, text, 256 );
	n.msg[256] = 0;
	n.pos = pos;
	n.color = colorval;
	n.time = welt->get_current_month();
	n.bild = bild;

	// insert at the top
	list->insert(n);
	char *p = list->at(0).msg;
	// should we open an autoclose windows?
	if(art&auto_win_flags  &&  welt!=NULL) {
		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt,p,bild,pos,colorval), w_autodelete );
	}

	// should we open a normal windows?
	if(art&win_flags  &&  welt!=NULL) {
		create_win(-1, -1,new nachrichtenfenster_t(welt,p,bild,pos,colorval),w_autodelete);
	}
}
