/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "components/gui_button.h"


#include "message_stats_t.h"

#include "messagebox.h"

#include "../simgraph.h"
#include "../simwin.h"
#include "../simworld.h"

#include "../dataobj/umgebung.h"


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
			recalc_size();
		}
		else {
			// case : filter messages belonging to the specified type
			message_list = &filtered_messages;
			filtered_messages.clear();
			FOR(slist_tpl<message_t::node*>, const i, msg->get_list()) {
				if (i->get_type_shifted() & message_type) {
					filtered_messages.append(i);
				}
			}
			recalc_size();
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
		message_selected = ev->cy/(LINESPACE+1);
	}

	if(  IS_LEFTRELEASE(ev)  ) {
		sint32 line = ev->cy/(LINESPACE+1);
		if(  (uint32)line<message_list->get_count()  ) {
			message_t::node &n = *(message_list->at(line));
			if(  ev->cx>=2  &&  ev->cx<=12  &&  welt->is_within_limits(n.pos)  ) {
				welt->change_world_position( koord3d(n.pos, welt->min_hgt(n.pos)) );
			}
			else {
				// show message window again
				news_window* news;
				if(  n.pos==koord::invalid  ) {
					news = new news_img( n.msg, n.bild, n.get_player_color(welt) );
				}
				else {
					news = new news_loc( welt, n.msg, n.pos, n.get_player_color(welt) );
				}
				create_win(-1, -1, news, w_info, magic_none);
			}
		}
	}
	else if(  IS_RIGHTRELEASE(ev)  ) {
		// just reposition
		sint32 line = ev->cy/(LINESPACE+1);
		if(  (uint32)line<message_list->get_count()  ) {
			message_t::node &n = *(message_list->at(line));
			if(  welt->is_within_limits(n.pos)  ) {
				welt->change_world_position( koord3d(n.pos, welt->min_hgt(n.pos)) );
			}
		}
	}
	return false;
}


void message_stats_t::recalc_size()
{
	sint16 x_size = 0;
	sint16 y_size = 0;
	// avoid overflow if too many messages are in the list
	sint16 y_max = 0x7fff - LINESPACE - 1;

	// loop copied from ::zeichnen(), trimmed to minimum for x_size calculation

	FORX(slist_tpl<message_t::node*>, const i, *message_list, y_size += LINESPACE + 1) {
		message_t::node const& n = *i;

		// add time
		char time[64];
		switch (umgebung_t::show_month) {
			case umgebung_t::DATE_FMT_GERMAN:
			case umgebung_t::DATE_FMT_GERMAN_NO_SEASON:
				sprintf(time, "(%d.%d)", (n.time%12)+1, n.time/12 );
				break;

			case umgebung_t::DATE_FMT_MONTH:
			case umgebung_t::DATE_FMT_US:
			case umgebung_t::DATE_FMT_US_NO_SEASON:
				sprintf(time, "(%d/%d)", (n.time%12)+1, n.time/12 );
				break;

			case umgebung_t::DATE_FMT_JAPANESE:
			case umgebung_t::DATE_FMT_JAPANESE_NO_SEASON:
				sprintf(time, "(%d/%d)", n.time/12, (n.time%12)+1 );
				break;

			default:
				time[0] = 0;
		}
		KOORD_VAL left = 14;
		if(  time[0]  ) {
			left += proportional_string_width(time)+8;
		}

		char buf[256];
		for(  int j=0;  j<256;  ++j  ) {
			buf[j] = (n.msg[j]=='\n')?' ':n.msg[j];
			if(  buf[j]==0  ) {
				break;
			}
		}

		left += proportional_string_width(buf);
		if(  left>x_size  ) {
			x_size = left;
		}
		if(  y_size >= y_max) {
			break;
		}
	}

	set_groesse(koord(x_size+4,y_size));
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
			recalc_size();
		}
		else {
			// incrementally add new entries to filtered message list before recalculating component size, and update last count
			uint32 entry_count = new_count - last_count;
			// Knightly : for ensuring correct chronological order of the new messages
			slist_tpl<message_t::node *> temp_list;
			FOR(slist_tpl<message_t::node*>, const i, msg->get_list()) {
				if (entry_count-- == 0) break;
				if (i->get_type_shifted() & message_type) {
					temp_list.insert(i);
				}
			}
			// insert new messages to old messages
			while (!temp_list.empty()) {
				filtered_messages.insert( temp_list.remove_first() );
			}
			last_count = new_count;
			recalc_size();
		}
	}

	clip_dimension const cd = display_get_clip_wh();
	sint16 y = offset.y+2;

	// changes to loop affecting x_size must be copied to ::recalc_size()
	FORX(slist_tpl<message_t::node*>, const i, *message_list, y += LINESPACE + 1) {
		if(  y<cd.y  ) {
			// below the top
			continue;
		}
		if(  y>cd.yy  ) {
			break;
		}
		message_t::node const n = *i;

		// goto information
		if(  n.pos!=koord::invalid  ) {
			// goto button
			display_color_img( message_selected!=((y-offset.y)/(LINESPACE+1)) ? button_t::arrow_right_normal : button_t::arrow_right_pushed, offset.x + 2, y, 0, false, true);
		}

		// correct for player color
		PLAYER_COLOR_VAL colorval = n.get_player_color(welt);

		// add time
		char time[64];
		switch (umgebung_t::show_month) {
			case umgebung_t::DATE_FMT_GERMAN:
			case umgebung_t::DATE_FMT_GERMAN_NO_SEASON:
				sprintf(time, "(%d.%d)", (n.time%12)+1, n.time/12 );
				break;

			case umgebung_t::DATE_FMT_MONTH:
			case umgebung_t::DATE_FMT_US:
			case umgebung_t::DATE_FMT_US_NO_SEASON:
				sprintf(time, "(%d/%d)", (n.time%12)+1, n.time/12 );
				break;

			case umgebung_t::DATE_FMT_JAPANESE:
			case umgebung_t::DATE_FMT_JAPANESE_NO_SEASON:
				sprintf(time, "(%d/%d)", n.time/12, (n.time%12)+1 );
				break;

			default:
				time[0] = 0;
		}
		KOORD_VAL left = 14;
		if(  time[0]  ) {
			left += display_proportional_clip(offset.x+left, y, time, ALIGN_LEFT, colorval, true)+8;
		}

		// display text with clipping
		char buf[256];
		for(  int j=0;  j<256;  ++j  ) {
			buf[j] = (n.msg[j]=='\n')?' ':n.msg[j];
			if(  buf[j]==0  ) {
				break;
			}
		}
		display_proportional_clip(offset.x+left, y, buf, ALIGN_LEFT, colorval, true);
	}
}
