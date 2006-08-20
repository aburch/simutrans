/*
 * messagelist_stats_t.cc
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "message_stats_t.h"

#include "../simgraph.h"
#include "../simcolor.h"
#include "../simcity.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../simmesg.h"

#include "../gui/stadt_info.h"

#include "../utils/cbuffer_t.h"





message_stats_t::message_stats_t(karte_t *w)
{
	welt = w;
	msg = message_t::get_instance();
	last_count = 0;
	setze_groesse(koord(600,msg->gib_count()*14 + 30));
}



/**
 * Click on message => go to position
 * @author Hj. Malthaner
 */
void message_stats_t::infowin_event(const event_t * ev)
{
	if(IS_LEFTRELEASE(ev)) {
		const unsigned int line = (ev->cy - 15) / 14;
		message_t::node *n=msg->get_node(line);
		if(ev->cy>14  &&  n!=NULL  &&  welt->ist_in_kartengrenzen(n->pos)) {
			welt->setze_ij_off(n->pos + koord(-5,-5));
		}
	}
	unsigned count = msg->gib_count();
	if(last_count!=count) {
		last_count = count;
		setze_groesse(koord(600,last_count*14 + 30));
	}
}



/**
 * Now draw the list
 * @author Hj. Malthaner
 */
void message_stats_t::zeichnen(koord offset) const
{
	struct clip_dimension cd = display_gib_clip_wh();
	unsigned count = msg->gib_count();
	for(  int i=0;  i<count;  i++  ) {
		if(offset.y+i*14<cd.y) {
			// reached the top
			continue;
		}
		if(offset.y+i*14+15>cd.yy) {
			// reached the bottom
			break;
		}

		message_t::node *n=msg->get_node(i);
		if(n==NULL  ||  n->msg==NULL) {
			// paranoia
			break;
		}
		char buf[256];
		for( int j=0;  j<256;  j++  ) {
			buf[j] = (n->msg[j]=='\n')?' ':n->msg[j];
			if(buf[j]==0) {
				break;
			}
		}
		// display text with clipping
		display_proportional_clip(offset.x+10, 15+offset.y+i*14, buf, ALIGN_LEFT, n->color,false);
	}
}
