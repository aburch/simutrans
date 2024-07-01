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
#include "dataobj/translator.h"
#include "dataobj/environment.h"
#include "dataobj/pakset_manager.h"
#include "network/network_socket_list.h"
#include "player/simplay.h"
#include "utils/simstring.h"
#include "tpl/slist_tpl.h"
#include "gui/messagebox.h"
#include <string.h>
#include <time.h>
#include "simsound.h"
#include "descriptor/sound_desc.h"

#define MAX_SAVED_MESSAGES (2000)

static karte_ptr_t welt;

static vector_tpl<plainstring>clients;


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
	clients.clear();
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


void message_t::rdwr(loadsave_t* file)
{
	uint16 msg_count;
	if (file->is_saving()) {
		msg_count = 0;
		vector_tpl<message_node_t*>msg_to_save;
		msg_to_save.reserve(list.get_count());
		if (env_t::server) {
			// do not save local messages and expired messages
			uint32 current_time = world()->get_current_month();
			for (message_node_t* const i : list) {
				if (i->type & DO_NOT_SAVE_MSG || (i->type & EXPIRE_AFTER_ONE_MONTH_MSG && current_time - i->time > 1)) {
					continue;
				}
				msg_to_save.append(i);
			}
		}
		else {
			// do not save discardable messages (like changing player)
			for (message_node_t* const i : list) {
				if (!(i->type & DO_NOT_SAVE_MSG)) {
					msg_to_save.append(i);
				}
			}
		}
		// save only the last MAX_SAVED_MESSAGES
		msg_count = (uint16)min(MAX_SAVED_MESSAGES, msg_to_save.get_count());
		file->rdwr_short(msg_count);
		for (uint32 i = msg_to_save.get_count() - msg_count; i < msg_to_save.get_count(); i++) {
			msg_to_save[i]->rdwr(file);
		}
	}
	else {
		// loading
		clear();
		file->rdwr_short(msg_count);
		if (file->is_version_less(124, 1)) {
			welt->get_chat_message()->clear();
		}
		while ((msg_count--) > 0) {
			message_node_t* n = new message_node_t();
			n->rdwr(file);
			// maybe add this rather to the chat
			if (file->is_version_less(124, 1)) {
				if (n->get_type_shifted() == (1 << message_t::chat)) {
					char name[256];
					char msg_no_name[256];
					sint8 player_nr = 0;

					if (char* c = strchr(n->msg, ']')) {
						*c = 0;
						strcpy(name, n->msg+1);
						strcpy(msg_no_name, c+1);
					}
					else {
						strcpy(name, "Unknown");
						tstrncpy(msg_no_name, n->msg,256);
					}
					if (n->color & PLAYER_FLAG) {
						player_t* player = welt->get_player(n->color & (~PLAYER_FLAG));
						if (player) {
							player_nr = player->get_player_nr();
						}
					}
					welt->get_chat_message()->convert_old_message(msg_no_name, name, player_nr, n->time, n->pos.get_2d());
					continue; // remove the chat message
				}
			}
			list.append(n);
		}
	}
}




chat_message_t::~chat_message_t()
{
	clear();
}


void chat_message_t::clear()
{
	while (!list.empty()) {
		delete list.remove_first();
	}
}

void chat_message_t::convert_old_message(const char* text, const char* name, sint8 player_nr, sint32 time, koord pos)
{
	chat_node* const n = new chat_node();
	tstrncpy(n->msg, text, lengthof(n->msg));

	n->flags = chat_message_t::NONE;
	n->pos = pos;
	n->player_nr = player_nr;
	n->channel_nr = -1;
	n->sender = name;
	n->destination = 0;
	n->time = time;
	n->local_time = 0;
	list.append(n);
}

void chat_message_t::add_chat_message(const char* text, sint8 channel, sint8 sender_nr, plainstring sender_, plainstring recipient, koord pos, uint8 flags)
{
	cbuffer_t buf;  // Output which will be printed to ticker
	player_t* player = world()->get_player(sender_nr);
	// send this message to a ticker if public channel message
	if (channel == -1) {
		if (sender_nr >= 0 && sender_nr != PLAYER_UNOWNED) {
			if (player) {
				if (player != world()->get_active_player()) {
					buf.printf("%s: %s", sender_.c_str(), text);
					ticker::add_msg(buf, koord3d::invalid, SYSCOL_TEXT);
					env_t::chat_unread_public++;
					sound_play(sound_desc_t::message_sound, 255, TOOL_SOUND);
				}
			}
		}
	}
	if (channel == -2) {
		// text contains the current nicks, separated by TAB
		clients.clear();
		char* nick = strtok((char*)text, "\t");
		while (nick) {
			clients.append(nick);
			nick = strtok(NULL, "\t");
		}
		flags |= DO_NO_LOG_MSG|DO_NOT_SAVE_MSG; // not saving this message
	}
	else if (recipient && strcmp(recipient, env_t::nickname.c_str()) == 0  &&  sender_nr != world()->get_active_player_nr()) {
		buf.printf("%s>> %s", sender_.c_str(), text);
		ticker::add_msg(buf, koord3d::invalid, color_idx_to_rgb(COL_RED));
		env_t::chat_unread_whisper++;
		sound_play(sound_desc_t::message_sound, 255, TOOL_SOUND);
	}
	else if (channel == world()->get_active_player_nr()  &&  sender_nr != world()->get_active_player_nr()) {
		PIXVAL text_color = player ? color_idx_to_rgb(player->get_player_color1() + env_t::gui_player_color_dark) : SYSCOL_TEXT;
		buf.printf("%s: %s", sender_.c_str(), text);
		ticker::add_msg(buf, koord3d::invalid, text_color);
		env_t::chat_unread_company++;
		sound_play(sound_desc_t::message_sound, 255, TOOL_SOUND);
	}

	if (!(flags & DO_NO_LOG_MSG)) {
		// we do not allow messages larger than 256 bytes
		chat_node* const n = new chat_node();
		tstrncpy(n->msg, text, lengthof(n->msg));
		n->flags = flags;
		n->pos = pos;

		n->player_nr = sender_nr;
		n->channel_nr = channel;
		n->sender = sender_;
		n->destination = recipient;

		n->time = world()->get_current_month();
		time_t ltime;
		time(&ltime);
		n->local_time = ltime;

		list.append(n);
	}
}

void chat_message_t::rename_client(plainstring old_name, plainstring new_name)
{
	for (chat_node* i : list) {
		if (i->sender && strcmp(i->sender, old_name) == 0) {
			i->sender = new_name;
		}
		if (i->destination && strcmp(i->destination, old_name) == 0) {
			i->destination = new_name;
		}
	}
}

void chat_message_t::chat_node::rdwr(loadsave_t* file)
{
	file->rdwr_str(msg, lengthof(msg));
	file->rdwr_byte(flags);
	file->rdwr_byte(player_nr);
	file->rdwr_byte(channel_nr);
	file->rdwr_str(sender);
	file->rdwr_str(destination);
	file->rdwr_long(time);
	sint64 tmp = local_time;
	file->rdwr_longlong(tmp);
	local_time = tmp;
	pos.rdwr(file);
}

void chat_message_t::rotate90(sint16 size_w)
{
	for (chat_node* const i : list) {
		i->pos.rotate90(size_w);
	}
}

void chat_message_t::rdwr(loadsave_t* file)
{
	uint16 msg_count;
	if (file->is_saving()) {
		vector_tpl<chat_node*>msg_to_save;
		msg_to_save.reserve(list.get_count());
		// do not save discardable messages
		for (chat_node* const i : list) {
			if (!(i->flags & DO_NOT_SAVE_MSG)) {
				msg_to_save.append(i);
			}
		}
		// save only the last MAX_SAVED_MESSAGES
		msg_count = (uint16)min(MAX_SAVED_MESSAGES, msg_to_save.get_count());
		file->rdwr_short(msg_count);
		for (uint32 i = msg_to_save.get_count() - msg_count; i < msg_to_save.get_count(); i++) {
			msg_to_save[i]->rdwr(file);
		}
	}
	else {
		// loading
		clear();

		file->rdwr_short(msg_count);
		while ((msg_count--) > 0) {
			chat_node* n = new chat_node();
			n->rdwr(file);
			list.append(n);
		}
	}
}


// return the number of connected clients
const vector_tpl<plainstring>& chat_message_t::get_online_nicks()
{
	/*
	if (env_t::server) {
		return socket_list_t::get_playing_clients();
	}
	else if (env_t::networkmode) {
		return clients_active;
	}
	return 0;
	*/
	return clients;
}
