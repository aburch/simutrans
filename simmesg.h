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
#include "tpl/slist_tpl.h"

class karte_t;
class karte_ptr_t;

/**
 * class for a simple message
 * this way they are stored in a list
 */
class message_t
{
public:
	class node {
	public:
		char msg[256];
		sint32 type;
		koord pos;
		FLAGGED_PIXVAL color;
		image_id image;
		sint32 time;

		void rdwr(loadsave_t *file);

		uint32 get_type_shifted() const { return 1<<(type & MESSAGE_TYPE_MASK); }

		FLAGGED_PIXVAL get_player_color(karte_t*) const;
	};

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

		expire_after_one_month_flag = 1 << 13,
		do_not_rdwr_flag            = 1 << 14,
		playermsg_flag              = 1 << 15
	};

	void add_message( const char *text, koord pos, uint16 what, FLAGGED_PIXVAL color=SYSCOL_TEXT, image_id image=IMG_EMPTY );

	/* determines, which message is displayed where */
	void get_message_flags( sint32 *t, sint32 *w, sint32 *a, sint32  *i);
	void set_message_flags( sint32, sint32, sint32, sint32 );

	message_t();
	~message_t();

private:
	static karte_ptr_t welt;

	// bitfields that contains the messages
	sint32 ticker_flags;
	sint32 win_flags;
	sint32 auto_win_flags;
	sint32 ignore_flags;

	slist_tpl<node *> list;

public:
	const slist_tpl<node *> &get_list() const { return list; }

	void clear();

	void rotate90( sint16 size_w );

	void rdwr( loadsave_t *file );

	/**
	 * Returns first valid coordinate from text (or koord::invalid if none is found).
	 * syntax: either @x,y or (x,y)
	 */
	static koord get_coord_from_text(const char* text);
};

#endif
