#ifndef label_h
#define label_h

#include "../simdings.h"
#include "../simimg.h"

class label_t : public ding_t
{
public:
	label_t(karte_t *welt, loadsave_t *file);
	label_t(karte_t *welt, koord3d pos, spieler_t *sp, const char *text);
	~label_t();

	void laden_abschliessen();

	enum ding_t::typ get_typ() const { return ding_t::label; }

	image_id get_bild() const;
};

#endif
