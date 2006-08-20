/*
 * runway.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef boden_wege_runway_h
#define boden_wege_runway_h


#include "weg.h"


/**
 * Klasse für runwayn in Simutrans.
 * speed >250 are for take of (maybe rather use system type in next release?)
 *
 * @author Hj. Malthaner
 * @version 1.0
 */
class runway_t : public weg_t
{
private:

public:
    /**
     * File loading constructor.
     *
     * @author Hj. Malthaner
     */
    runway_t(karte_t *welt, loadsave_t *file);

    /**
     * Basic constructor.
     *
     * @author Hj. Malthaner
     */
    runway_t(karte_t *welt);
    /**
     * Basic constructor with top_speed
     * @author Hj. Malthaner
     */
    runway_t(karte_t *welt, int top_speed);

    /**
     * Destruktor. Entfernt etwaige Debug-Meldungen vom Feld
     *
     * @author Hj. Malthaner
     */
    virtual ~runway_t();

    /**
     * Calculates the image of this pice of runway
     */
    virtual void calc_bild(koord3d) { weg_t::calc_bild(); }


    inline const char *gib_typ_name() const {return "runway";};
    inline typ gib_typ() const {return luft;};

    /**
     * @return Infotext zur runway
     * @author Hj. Malthaner
     */
    void info(cbuffer_t & buf) const;

    void rdwr(loadsave_t *file);
};

#endif
