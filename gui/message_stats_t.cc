/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "message_stats_t.h"

#include "../simgraph.h"
#include "../simcolor.h"
#include "../simcity.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../simmesg.h"

#include "../gui/stadt_info.h"


message_stats_t::message_stats_t(karte_t *w)
{
	welt = w;
	msg = welt->get_message();
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
		const int line = (ev->cy - 15) / 14;
		if(line<msg->gib_count()) {
			message_t::node *n=msg->get_node(line);
			if(ev->cy>14  &&  n!=NULL  &&  welt->ist_in_kartengrenzen(n->pos)) {
				welt->change_world_position(koord3d(n->pos,welt->min_hgt(n->pos)));
			}
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
 * @author prissi
 */
void message_stats_t::zeichnen(koord offset)
{
	struct clip_dimension cd = display_gib_clip_wh();
	const int offsets = (cd.y-offset.y+13)/14;
	int max_message = (cd.yy-offset.y-13)/14;
	if(max_message>msg->gib_count()) {
		max_message = msg->gib_count();
	}
	for( int i=offsets;  i<max_message;  i++  ) {

		if(offset.y+i*14<cd.y) {
			// reached the top
			continue;
		}
		if(offset.y+i*14+15>cd.yy) {
			// reached the bottom
			break;
		}

		message_t::node *n=msg->get_node(i);
		if (n == NULL) {
DBG_MESSAGE("message_stats_t::zeichnen()","invalid message %i",i);
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
		display_proportional_clip(offset.x+10, 15+offset.y+i*14, buf, ALIGN_LEFT, n->color,true);
	}
}
