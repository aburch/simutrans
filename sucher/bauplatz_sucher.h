/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef bauplatz_sucher_t_h
#define bauplatz_sucher_t_h

#include "platzsucher.h"
#include "../simworld.h"

/**
 * bauplatz_sucher_t:
 *
 * Sucht einen freien Bauplatz mithilfe der Funktion suche_platz().
 *
 * @author V. Meyer
 */
class bauplatz_sucher_t : public platzsucher_t {
public:
	bauplatz_sucher_t(karte_t *welt, sint16 radius = -1) : platzsucher_t(welt, radius) {}

	virtual bool ist_platz_ok(koord pos, sint16 b, sint16 h, climate_bits cl) const {
		return welt->square_is_free(pos, b, h, NULL, cl);
	}
};

#endif
