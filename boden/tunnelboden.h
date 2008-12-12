#ifndef tunnelboden_h
#define tunnelboden_h

#include "boden.h"


class tunnelboden_t : public boden_t
{
protected:
	void calc_bild_internal();

public:
	tunnelboden_t(karte_t *welt, loadsave_t *file, koord pos );
	tunnelboden_t(karte_t *welt, koord3d pos, hang_t::typ hang_typ) : boden_t(welt, pos, hang_typ) {}

	virtual void rdwr(loadsave_t *file);

	hang_t::typ gib_weg_hang() const { return hang_t::flach; }

	const char *gib_name() const {return "Tunnelboden";}
	enum grund_t::typ gib_typ() const {return tunnelboden;}
};

#endif
