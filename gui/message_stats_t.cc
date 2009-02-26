/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "message_stats_t.h"

#include "messagebox.h"

#include "../simgraph.h"
#include "../simcolor.h"
#include "../simcity.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../simmesg.h"
#include "../simskin.h"

#include "../gui/stadt_info.h"


message_stats_t::message_stats_t(karte_t *w)
{
	welt = w;
	msg = welt->get_message();
	last_count = 0;
	message_selected = -1;
	set_groesse(koord(600,msg->get_count()*14 + 30));
}



/**
 * Click on message => go to position
 * @author Hj. Malthaner
 */
void message_stats_t::infowin_event(const event_t * ev)
{
	message_selected = -1;
	if(  ev->button_state>0  &&  ev->cx>=2  &&  ev->cx<=12  ) {
		message_selected = ev->cy/14;
	}

	if(IS_LEFTRELEASE(ev)) {
		sint32 line = ev->cy/14;
		if(  (uint32)line<msg->get_count()  ) {
			message_t::node *n=msg->get_node(line);
			if(n) {
				if(  ev->cx>=2  &&  ev->cx<=12  &&  welt->ist_in_kartengrenzen(n->pos)  ) {
					welt->change_world_position(koord3d(n->pos,welt->min_hgt(n->pos)));
				}
				else {
					// show message window again
					news_window* news;
					if(  n->pos == koord::invalid  ) {
						news = new news_img( n->msg, n->bild, n->color);
					}
					else {
						news = new news_loc( welt, n->msg, n->pos, n->color);
					}
					create_win(-1, -1, news, w_info, magic_none);
				}
			}
		}
	}
	else if (IS_RIGHTRELEASE(ev)) {
		// just reposition
		sint32 line = ev->cy/14;
		if(  (uint32)line<msg->get_count()  ) {
			message_t::node *n=msg->get_node(line);
			if(n  &&  welt->ist_in_kartengrenzen(n->pos)  ) {
				welt->change_world_position(koord3d(n->pos,welt->min_hgt(n->pos)));
			}
		}
	}

	unsigned count = msg->get_count();
	if(last_count!=count) {
		last_count = count;
		set_groesse(koord(600,last_count*14 + 30));
	}
}



/**
 * Now draw the list
 * @author prissi
 */
void message_stats_t::zeichnen(koord offset)
{
	const image_id arrow_right_normal = skinverwaltung_t::window_skin->get_bild(10)->get_nummer();
	struct clip_dimension cd = display_get_clip_wh();
	const int offsets = (cd.y-offset.y)/14;
	sint32 max_message = (cd.yy-offset.y)/14+1;
	if(  max_message>(sint32)msg->get_count()  ) {
		max_message = msg->get_count();
	}

	for( int i=offsets;  i<max_message;  i++  ) {

		const sint16 y = offset.y+i*14;
		if(y+14<cd.y) {
			// below the top
			continue;
		}

		message_t::node *n=msg->get_node(i);
		if (n == NULL) {
DBG_MESSAGE("message_stats_t::zeichnen()","invalid message %i",i);
			// paranoia
			break;
		}

		// goto information
		if(n->pos!=koord::invalid) {
			if(message_selected!=i) {
				display_color_img(arrow_right_normal, offset.x + 2, y, 0, false, true);
			}
			else {
				// select goto button
				display_color_img(skinverwaltung_t::window_skin->get_bild(11)->get_nummer(), offset.x + 2, y, 0, false, true);
			}
		}

		char buf[256];
		for( int j=0;  j<256;  j++  ) {
			buf[j] = (n->msg[j]=='\n')?' ':n->msg[j];
			if(buf[j]==0) {
				break;
			}
		}
		// display text with clipping
		display_proportional_clip(offset.x+4+10, y, buf, ALIGN_LEFT, n->color,true);
	}
}
