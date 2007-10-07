#ifndef monorailboden_h
#define monorailboden_h

#include "grund.h"

class monorailboden_t : public grund_t
{
public:
	monorailboden_t(karte_t *welt, loadsave_t *file);
	monorailboden_t(karte_t *welt, koord3d pos,hang_t::typ slope);

	virtual void rdwr(loadsave_t *file);

	const char *gib_name() const {return "Monorailboden";}
	enum grund_t::typ gib_typ() const {return monorailboden;}

	void calc_bild();
};

#endif
