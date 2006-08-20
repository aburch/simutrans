/*
 * bauplatz_sucher.h
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef bauplatz_sucher_t_h
#define bauplatz_sucher_t_h

#include "platzsucher.h"

/**
 * bauplatz_sucher_t:
 *
 * Sucht einen freien Bauplatz mithilfe der Funktion suche_platz().
 *
 * @author V. Meyer
 */
class bauplatz_sucher_t : public platzsucher_t {
public:
    bauplatz_sucher_t(karte_t *welt) : platzsucher_t(welt) {}

    virtual bool ist_platz_ok(koord pos, int b, int h) const {
	return welt->ist_platz_frei(pos, b, h, NULL, true);
    }
};

#endif // bauplatz_sucher_t_h
