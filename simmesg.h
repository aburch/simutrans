#ifndef simmesg_h
#define simmesg_h

#include "simtypes.h"
#include "simcolor.h"
#include "simworld.h"
#include "simimg.h"

#include "dataobj/koord.h"

// forward decl
template <class T> class slist_tpl;

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

	enum msg_typ {general=0, ai=1, city=2, convoi=3, industry=4, tourist=5, new_vehicle=6, full=7, problems=8 };

	void add_message( const char *text, koord pos, msg_typ where, uint8 color=COL_BLACK, image_id bild=IMG_LEER );

    static message_t * get_instance();

	int gib_count() const;

	/* determines, which message is displayed where */
	void get_flags( int *t, int *w, int *a, int  *i);
	void set_flags( int, int, int, int );

	message_t(karte_t *welt);
	~message_t();

private:
	karte_t	*welt;

	// bitfields that contains the messages
	int	ticker_flags;
	int win_flags;
	int auto_win_flags;
	int ignore_flags;

	slist_tpl <node> * list;

	static message_t * single_instance;

public:
	node*	get_node(unsigned i);
};

#endif
