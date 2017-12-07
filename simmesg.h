#ifndef simmesg_h
#define simmesg_h

#include "simtypes.h"
#include "gui/gui_theme.h"
#include "display/simimg.h"
#include "dataobj/koord.h"
#include "tpl/slist_tpl.h"

class world_t;
class karte_ptr_t;

/* class for a simple message
 * this way they are stored in a list
 * @author prissi
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

		uint32 get_type_shifted() const { return 1<<(type & ~local_flag); }

		FLAGGED_PIXVAL get_player_color(world_t*) const;
	};

	enum msg_typ {
		general=0,
		ai=1,
		city=2,
		problems=3,
		industry=4,
		chat=5,
		new_vehicle=6,
		full=7,
		warnings=8,
		traffic_jams=9,
		scenario=10,
		MAX_MESSAGE_TYPE,
		local_flag = 0x8000u
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
