/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "components/list_button.h"

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
	set_groesse(koord(600,1));
}



/**
 * Click on message => go to position
 * @author Hj. Malthaner
 */
bool message_stats_t::infowin_event(const event_t * ev)
{
	message_selected = -1;
	if(  ev->button_state>0  &&  ev->cx>=2  &&  ev->cx<=12  ) {
		message_selected = ev->cy/BUTTON_HEIGHT;
	}

	if(IS_LEFTRELEASE(ev)) {
		sint32 line = ev->cy/BUTTON_HEIGHT;
		if(  (uint32)line<msg->get_list().get_count()  ) {
			message_t::node &n=msg->get_list().at(line);
			if(  ev->cx>=2  &&  ev->cx<=12  &&  welt->ist_in_kartengrenzen(n.pos)  ) {
				welt->change_world_position(koord3d(n.pos,welt->min_hgt(n.pos)));
			}
			else {
				// show message window again
				news_window* news;
				if(  n.pos == koord::invalid  ) {
					news = new news_img( n.msg, n.bild, n.color);
				}
				else {
					news = new news_loc( welt, n.msg, n.pos, n.color);
				}
				create_win(-1, -1, news, w_info, magic_none);
			}
		}
	}
	else if (IS_RIGHTRELEASE(ev)) {
		// just reposition
		sint32 line = ev->cy/BUTTON_HEIGHT;
		if(  (uint32)line<msg->get_list().get_count()  ) {
			message_t::node &n=msg->get_list().at(line);
			if(welt->ist_in_kartengrenzen(n.pos)  ) {
				welt->change_world_position(koord3d(n.pos,welt->min_hgt(n.pos)));
			}
		}
	}
	return false;
}



/**
 * Now draw the list
 * @author prissi
 */
void message_stats_t::zeichnen(koord offset)
{
	const image_id arrow_right_normal = skinverwaltung_t::window_skin->get_bild(10)->get_nummer();
	struct clip_dimension cd = display_get_clip_wh();
	sint16 y = offset.y+1;

	for(  slist_tpl<message_t::node>::const_iterator iter = msg->get_list().begin(), end = msg->get_list().end();  iter!=end; ++iter,  y += BUTTON_HEIGHT  ) {

		if(  y<cd.y  ) {
			// below the top
			continue;
		}
		if(  y>cd.yy  ) {
			break;
		}
		const message_t::node &n = *iter;

		// goto information
		if(n.pos!=koord::invalid) {
			if(  message_selected!=((y-offset.y)/BUTTON_HEIGHT)  ) {
				display_color_img(arrow_right_normal, offset.x + 2, y, 0, false, true);
			}
			else {
				// select goto button
				display_color_img(skinverwaltung_t::window_skin->get_bild(11)->get_nummer(), offset.x + 2, y, 0, false, true);
			}
		}

		char buf[256];
		for( int j=0;  j<256;  j++  ) {
			buf[j] = (n.msg[j]=='\n')?' ':n.msg[j];
			if(buf[j]==0) {
				break;
			}
		}
		// display text with clipping
		display_proportional_clip(offset.x+4+10, y, buf, ALIGN_LEFT, n.color,true);
	}

	uint16 count = min(2000,msg->get_list().get_count());
	if(last_count!=count) {
		last_count = count;
		set_groesse(koord(600,last_count*BUTTON_HEIGHT+1));
	}
}
