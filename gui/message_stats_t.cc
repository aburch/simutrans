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
#include "../simskin.h"

#include "../gui/stadt_info.h"


message_stats_t::message_stats_t(karte_t *w) :
	welt(w), msg(w->get_message()), message_type(0), last_count(0), message_selected(-1), message_list(NULL)
{
	filter_messages(-1);
}


/**
 * Filter messages by type
 * @return whether there is a change in message filtering
 * @author Knightly
 */
bool message_stats_t::filter_messages(const sint32 msg_type)
{
	// only update if there is a change in message type
	if(  msg_type>=-1  &&  message_type!=msg_type  ) {
		message_type = msg_type;
		last_count = msg->get_list().get_count();
		message_selected = -1;
		if(  msg_type==-1  ) {
			// case : no message filtering
			message_list = &(msg->get_list());
			set_groesse( koord(600, min(2000, last_count) * BUTTON_HEIGHT + 1) );
		}
		else {
			// case : filter messages belonging to the specified type
			message_list = &filtered_messages;
			filtered_messages.clear();
			for(  slist_tpl<message_t::node *>::const_iterator iter=msg->get_list().begin(), end=msg->get_list().end();  iter!=end;  ++iter  ) {
				if(  (*iter)->get_type_shifted() & message_type  ) {
					filtered_messages.append( *iter );
				}
			}
			set_groesse( koord(600, min(2000, filtered_messages.get_count()) * BUTTON_HEIGHT + 1) );
		}
		return true;
	}
	return false;
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

	if(  IS_LEFTRELEASE(ev)  ) {
		sint32 line = ev->cy/BUTTON_HEIGHT;
		if(  (uint32)line<message_list->get_count()  ) {
			message_t::node &n = *(message_list->at(line));
			if(  ev->cx>=2  &&  ev->cx<=12  &&  welt->ist_in_kartengrenzen(n.pos)  ) {
				welt->change_world_position( koord3d(n.pos, welt->min_hgt(n.pos)) );
			}
			else {
				// show message window again
				news_window* news;
				if(  n.pos==koord::invalid  ) {
					news = new news_img( n.msg, n.bild, n.color );
				}
				else {
					news = new news_loc( welt, n.msg, n.pos, n.color );
				}
				create_win(-1, -1, news, w_info, magic_none);
			}
		}
	}
	else if(  IS_RIGHTRELEASE(ev)  ) {
		// just reposition
		sint32 line = ev->cy/BUTTON_HEIGHT;
		if(  (uint32)line<message_list->get_count()  ) {
			message_t::node &n = *(message_list->at(line));
			if(  welt->ist_in_kartengrenzen(n.pos)  ) {
				welt->change_world_position( koord3d(n.pos, welt->min_hgt(n.pos)) );
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
	// Knightly : update component size and filtered message list where necessary
	const uint32 new_count = msg->get_list().get_count();
	if(  last_count<new_count  ) {
		if(  message_type==-1  ) {
			// no message filtering -> only update last count and component size
			last_count = new_count;
			set_groesse( koord(600, min(2000, last_count) * BUTTON_HEIGHT + 1) );
		}
		else {
			// incrementally add new entries to filtered message list before recalculating component size, and update last count
			uint32 entry_count = new_count - last_count;
			// Knightly : for ensuring correct chronological order of the new messages
			slist_tpl<message_t::node *> temp_list;
			for(  slist_tpl<message_t::node *>::const_iterator iter=msg->get_list().begin(), end=msg->get_list().end();  iter!=end, entry_count>0;  ++iter, --entry_count  ) {
				if(  (*iter)->get_type_shifted() & message_type  ) {
					temp_list.insert(*iter);
				}
			}
			// insert new messages to old messages
			while(  temp_list.get_count()>0  ) {
				filtered_messages.insert( temp_list.remove_first() );
			}
			last_count = new_count;
			set_groesse( koord(600, min(2000, filtered_messages.get_count()) * BUTTON_HEIGHT + 1) );
		}
	}

	struct clip_dimension cd = display_get_clip_wh();
	sint16 y = offset.y+1;

	for(  slist_tpl<message_t::node *>::const_iterator iter=message_list->begin(), end=message_list->end();  iter!=end;  ++iter, y+=BUTTON_HEIGHT  ) {

		if(  y<cd.y  ) {
			// below the top
			continue;
		}
		if(  y>cd.yy  ) {
			break;
		}
		const message_t::node &n = *(*iter);

		// goto information
		if(  n.pos!=koord::invalid  ) {
			// goto button
			display_color_img( message_selected!=((y-offset.y)/BUTTON_HEIGHT) ? button_t::arrow_right_normal : button_t::arrow_right_pushed, offset.x + 2, y, 0, false, true);
		}

		char buf[256];
		for(  int j=0;  j<256;  ++j  ) {
			buf[j] = (n.msg[j]=='\n')?' ':n.msg[j];
			if(  buf[j]==0  ) {
				break;
			}
		}
		// correct for player color
		PLAYER_COLOR_VAL colorval = n.color;
		if(  n.color&PLAYER_FLAG  ) {
			spieler_t *sp = welt->get_spieler(n.color&(~PLAYER_FLAG));
			colorval = sp ? PLAYER_FLAG+sp->get_player_color1()+1 : MN_GREY0;
		}
		// display text with clipping
		display_proportional_clip(offset.x+4+10, y, buf, ALIGN_LEFT, n.color, true);
	}
}
