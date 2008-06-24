/*
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef boden_wege_maglev_h
#define boden_wege_maglev_h


#include "schiene.h"


/**
 * derived from schiene, because signals will behave similar
 */
class maglev_t : public schiene_t
{
public:
	static const weg_besch_t *default_maglev;

	maglev_t(karte_t *welt) : schiene_t(welt) { setze_besch(default_maglev); }

	/**
	 * File loading constructor.
	 * @author prissi
	 */
	maglev_t(karte_t *welt, loadsave_t *file);

	virtual waytype_t gib_waytype() const {return maglev_wt;}

	/**
	 * @return Infotext zur Schiene
	 * @author Hj. Malthaner
	 */
	void info(cbuffer_t & buf) const;

	void rdwr(loadsave_t *file);
};

#endif
