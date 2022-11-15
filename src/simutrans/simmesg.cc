/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "macros.h"
#include "simdebug.h"
#include "simmesg.h"
#include "simticker.h"
#include "display/simgraph.h"
#include "simcolor.h"
#include "gui/simwin.h"
#include "world/simworld.h"

#include "dataobj/loadsave.h"
#include "dataobj/environment.h"
#include "dataobj/pakset_manager.h"
#include "player/simplay.h"
#include "utils/simstring.h"
#include "tpl/slist_tpl.h"
#include "gui/messagebox.h"
#include <string.h>


#define MAX_SAVED_MESSAGES (2000)

static karte_ptr_t welt;

uint32 message_node_t::get_type_shifted() const
{
	return 1<<(type & message_t::MESSAGE_TYPE_MASK);
}

void message_node_t::rdwr(loadsave_t *file)
{
	file->rdwr_str( msg, lengthof(msg) );
	file->rdwr_long( type );

	if (file->is_version_less(123, 2)) {
		// only 2d coordinates
		koord k = pos.get_2d();
		k.rdwr(file);
		if (file->is_loading()) {
			pos = koord3d::invalid;
			if (k != koord::invalid) {
				// query ground tile (world is loaded at this point)
				if (grund_t *gr = welt->lookup_kartenboden(k)) {
					pos = gr->get_pos();
				}
			}
		}
	}
	else {
		pos.rdwr(file);
	}

	if (file->is_version_less(120, 5)) {
		// color was 16bit, with 0x8000 indicating player colors
		uint16 c = color & PLAYER_FLAG ? 0x8000 + (color&(~PLAYER_FLAG)) : MN_GREY0;
		file->rdwr_short(c);
		color = c & 0x8000 ? PLAYER_FLAG + (c&(~0x8000)) : color_idx_to_rgb(c);
	}
	else {
		file->rdwr_long( color );
	}
	file->rdwr_long( time );
	if(  file->is_loading()  ) {
		image = IMG_EMPTY;
	}
}


FLAGGED_PIXVAL message_node_t::get_player_color(karte_t *welt) const
{
	// correct for player color
	FLAGGED_PIXVAL colorval = color;
	if(  color&PLAYER_FLAG  ) {
		player_t *player = welt->get_player(color&(~PLAYER_FLAG));
		colorval = player ? PLAYER_FLAG+color_idx_to_rgb(player->get_player_color1()+env_t::gui_player_color_dark) : color_idx_to_rgb(MN_GREY0);
	}
	return colorval;
}


void message_node_t::open_msg_window(bool open_as_autoclose) const
{
	// show window with the complete warning
	if (strcmp(msg, pakset_manager_t::get_doubled_warning_message()) == 0) {
		pakset_manager_t::open_doubled_warning_window();
		return;
	}
	// message window again
	news_window* news;
	if(  pos==koord3d::invalid  ) {
		news = new news_img( msg, image, get_player_color(welt) );
	}
	else {
		news = new news_loc( msg, pos, get_player_color(welt) );
	}

	create_win(news, open_as_autoclose ? w_time_delete /* autoclose window */ : w_info /* normal window */, magic_none);
}


message_t::message_t()
{
	ticker_flags = 0xFF7F; // everything on the ticker only
	win_flags = 0;
	auto_win_flags = 0;
	ignore_flags = 0;
	win_flags = 256+8;
	auto_win_flags = 128+512;
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
	ticker::clear_messages();
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
 * @param pos        position of the event
 * @param color      message color
 * @param what_flags type of message
 * @param image      image associated with message (will be ignored if pos!=koord3d::invalid)
 */
void message_t::add_message(const char *text, koord3d pos, uint16 what_flags, FLAGGED_PIXVAL color, image_id image )
{
DBG_MESSAGE("message_t::add_msg()","%40s (at %i,%i,%i)", text, pos.x, pos.y, pos.z );

	sint32 what_bit = 1<<(what_flags & MESSAGE_TYPE_MASK);
	if(  what_bit&ignore_flags  ) {
		// wants us to ignore this completely
		return;
	}

	/* we will not add traffic jam messages two times to the list
	 * if it was within the last 20 messages
	 * or within last months
	 * and is not a general (BLACK) message
	 */
	if(  what_bit == (1<<traffic_jams)  ) {
		sint32 now = welt->get_current_month()-2;
		uint32 i = 0;
		for(message_node_t* const iter : list) {
			message_node_t const& n = *iter;
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

	// filter out AI messages for a similar area to recent activity messages
	if(  what_bit == (1<<ai)  &&  pos != koord3d::invalid  &&  env_t::networkmode  ) {
		uint32 i = 0;
		for(message_node_t* const iter : list) {
			message_node_t const& n = *iter;
			if ((n.pos.x & 0xFFE0) == (pos.x & 0xFFE0) &&
				(n.pos.y & 0xFFE0) == (pos.y & 0xFFE0)) {
				return;
			}
			if (++i == 20) break;
		}
	}

	// if no coordinate is provided, there is maybe one in the text message?
	// syntax: either @x,y or (x,y)
	if (pos == koord3d::invalid) {
		pos = get_coord_from_text(text);
	}

	// we do not allow messages larger than 256 bytes
	message_node_t *const n = new message_node_t();

	tstrncpy(n->msg, text, lengthof(n->msg));
	n->type = what_flags;
	n->pos = pos;
	n->color = color;
	n->time = welt->get_current_month();
	n->image = image;

	// should we send this message to a ticker?
	if(  what_bit&ticker_flags  ) {
		ticker::add_msg_node(*n);
	}

	// insert at the top
	list.insert(n);

	// if we are not current player, do not open windows
	if(  (what_bit&(1<<ai))==0  &&   (color & PLAYER_FLAG) != 0  &&  welt->get_active_player_nr() != (color&(~PLAYER_FLAG))  ) {
		return;
	}
	// check if some window has focus
	gui_frame_t *old_top = win_get_top();

	// should we open a window?
	if (  what_bit & (auto_win_flags | win_flags)  ) {
		n->open_msg_window( (what_bit & win_flags)==0 );
	}

	// restore focus
	if(  old_top    ) {
		top_win( old_top, true );
	}
}


koord3d message_t::get_coord_from_text(const char* text)
{
	koord3d pos(koord3d::invalid);
	const char *str = text;
	// scan until either @ or ( are found
	while( *(str += strcspn(str, "@(")) ) {
		str += 1;
		int x=-1, y=-1, z=0;
		int read_coords = sscanf(str, "%d,%d,%d", &x, &y, &z);
		if (read_coords >= 2  &&  welt->is_within_limits(x,y)) {
			if (read_coords == 3) {
				pos.x = x;
				pos.y = y;
				pos.z = z;
			}
			else {
				pos = welt->lookup_kartenboden(x,y)->get_pos();
			}
			break; // success
		}
	}
	return pos;
}


void message_t::rotate90( sint16 size_w )
{
	for(message_node_t* const i : list) {
		i->pos.rotate90(size_w);
	}
}


void message_t::rdwr( loadsave_t *file )
{
	uint16 msg_count;
	if(  file->is_saving()  ) {
		msg_count = 0;
		if( env_t::server ) {
			// do not save local messages and expired messages
			uint32 current_time = world()->get_current_month();
			for(message_node_t* const i : list) {
				if( i->type & do_not_rdwr_flag  ||  (i->type & expire_after_one_month_flag  &&  current_time - i->time > 1)  ) {
					continue;
				}
				if (++msg_count == MAX_SAVED_MESSAGES) break;
			}
			file->rdwr_short( msg_count );
			for(message_node_t* const i : list) {
				if (msg_count == 0) break;
				if(  i->type & do_not_rdwr_flag  || (i->type & expire_after_one_month_flag  &&  current_time - i->time > 1)  ) {
					continue;
				}
				i->rdwr(file);
				msg_count --;
			}
		}
		else {
			// do not save discardable messages (like changing player)
			for(message_node_t* const i : list) {
				if (!(i->type & do_not_rdwr_flag)) {
					if (++msg_count == MAX_SAVED_MESSAGES) break;
				}
			}
			file->rdwr_short( msg_count );
			for(message_node_t* const i : list) {
				if (msg_count == 0) break;
				if(  !(i->type & do_not_rdwr_flag)  ) {
					i->rdwr(file);
					msg_count --;
				}
			}
		}
	}
	else {
		// loading
		clear();
		file->rdwr_short(msg_count);
		while(  (msg_count--)>0  ) {
			message_node_t *n = new message_node_t();
			n->rdwr(file);
			list.append(n);
		}
	}
}
