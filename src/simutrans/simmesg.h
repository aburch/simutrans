/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMMESG_H
#define SIMMESG_H


#include "simtypes.h"
#include "gui/gui_theme.h"
#include "display/simimg.h"
#include "dataobj/koord.h"
#include "dataobj/koord3d.h"
#include "tpl/slist_tpl.h"
#include "utils/plainstring.h"

class karte_t;
class karte_ptr_t;

/**
 * class for a simple message
 * this way they are stored in a list
 */
struct message_node_t
{
	char msg[256];
	sint32 type;
	koord3d pos;
	FLAGGED_PIXVAL color;
	image_id image;
	sint32 time;

	void rdwr(loadsave_t *file);

	uint32 get_type_shifted() const; // { return 1<<(type & MESSAGE_TYPE_MASK); }

	FLAGGED_PIXVAL get_player_color(karte_t*) const;

	void open_msg_window(bool open_as_autoclose) const;
};

class message_t
{
public:
	enum msg_typ {
		general      = 0,
		ai           = 1,
		city         = 2,
		problems     = 3,
		industry     = 4,
		chat         = 5,
		new_vehicle  = 6,
		full         = 7,
		warnings     = 8,
		traffic_jams = 9,
		scenario     = 10,
		MAX_MESSAGE_TYPE,
		MESSAGE_TYPE_MASK = 0xf,

		EXPIRE_AFTER_ONE_MONTH_MSG = 1 << 13,
		DO_NOT_SAVE_MSG            = 1 << 14,
		PLAYER_MSG                 = 1 << 15
	};

	void add_message( const char *text, koord3d pos, uint16 what, FLAGGED_PIXVAL color=SYSCOL_TEXT, image_id image=IMG_EMPTY );

	/* determines, which message is displayed where */
	void get_message_flags( sint32 *t, sint32 *w, sint32 *a, sint32  *i);
	void set_message_flags( sint32, sint32, sint32, sint32 );

	message_t();
	~message_t();

private:
	// bitfields that contains the messages
	sint32 ticker_flags;
	sint32 win_flags;
	sint32 auto_win_flags;
	sint32 ignore_flags;

	slist_tpl<message_node_t *> list;

public:
	const slist_tpl<message_node_t *> &get_list() const { return list; }

	void clear();

	void rotate90( sint16 size_w );

	void rdwr( loadsave_t *file );

	/**
	 * Returns first valid coordinate from text (or koord::invalid if none is found).
	 * syntax: either @x,y or (x,y)
	 */
	static koord3d get_coord_from_text(const char* text);
};


class chat_message_t
{
public:

	class chat_node {
	public:
		char msg[256];
		uint8 flags;
		sint8 player_nr;
		sint8 channel_nr=-1;     // player(company) number, -1 = public chat
		plainstring sender;      // NULL = system message
		plainstring destination; // for whisper
		koord pos;
		sint32 time; // game date
		time_t local_time;

		void rdwr(loadsave_t* file);
	};

	enum {
		NONE            = 0,
		DO_NO_LOG_MSG   = 1<<1, // send system message via tool, but not logged chat window, send only ticker
		DO_NOT_SAVE_MSG = 1<<2  // send system message via tool, logged but not saved
		// TODO: We can consider flags such as messages pinned to the top by an admin, messages that remain for a certain amount of time, etc.
	};

	void add_chat_message(const char* text, sint8 channel=-1, sint8 sender_player_nr=1 /*PUBLIC_PLAYER_NR*/, plainstring sender=NULL, plainstring recipient=NULL, koord pos = koord::invalid, uint8 flags=chat_message_t::NONE);

	chat_message_t() {};
	~chat_message_t();

	static const vector_tpl<plainstring> &get_online_nicks();

private:
	slist_tpl<chat_node*> list;

public:
	const slist_tpl<chat_node*>& get_list() const { return list; }

	// for converting old log
	void convert_old_message(const char* text, const char* name, sint8 player_nr, sint32 time, koord pos);

	// call from nwc_nick_t
	void rename_client(plainstring old_name, plainstring new_name);

	void clear();

	void rotate90(sint16 size_w);

	void rdwr(loadsave_t* file);

};

#endif
