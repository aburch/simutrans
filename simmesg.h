#ifndef simmesg_h
#define simmesg_h

#include "simtypes.h"
#include "simcolor.h"
#include "simimg.h"
#include "dataobj/koord.h"
#include "tpl/slist_tpl.h"

class karte_t;

/* class for a simple message
 * this way they are stored in a list
 * @author prissi
 */
class message_t
{
public:
	struct node {
		char msg[256];
		koord pos;
		PLAYER_COLOR_VAL color;
		image_id bild;
		long time;
	};

	enum msg_typ {general=0, ai=1, city=2, convoi=3, industry=4, tourist=5, new_vehicle=6, full=7, problems=8, warnings=9, MAX_MESSAGE_TYPE };

	void add_message( const char *text, koord pos, msg_typ where, PLAYER_COLOR_VAL color=COL_BLACK, image_id bild=IMG_LEER );

	static message_t * get_instance();

	/* determines, which message is displayed where */
	void get_message_flags( sint32 *t, sint32 *w, sint32 *a, sint32  *i);
	void set_message_flags( sint32, sint32, sint32, sint32 );

	message_t(karte_t *welt);
	~message_t();

private:
	karte_t	*welt;

	// bitfields that contains the messages
	sint32 ticker_flags;
	sint32 win_flags;
	sint32 auto_win_flags;
	sint32 ignore_flags;

	slist_tpl<node> list;

public:
	const slist_tpl<node> &get_list() const { return list; }

	void clear() { list.clear(); }

	void rotate90( sint16 size_w );
};

#endif
