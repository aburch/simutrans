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


#include "../../railblocks.h"
#include "weg.h"


/**
 * Klasse für runwayn in Simutrans.
 * Auf den runwayn koennen Züge fahren.
 * Jede runway gehört zu einer Blockstrecke
 *
 * @author Hj. Malthaner
 * @version 1.0
 */
class runway_t : public weg_t
{
private:

    /**
     * Rail block handle. Should be always bound to a rail block,
     * except a short time after creation. Everything else is an error!
     *
     * @author Hj. Malthaner
     */
    blockhandle_t bs;


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


    /**
     * manche runwayn sind eingangs/ausgangsrunwayn für blcokstrecken
     * und informieren die blockstrecken, wenn jemand das feld betreten hat
     * @author Hj. Malthaner
     */
    void betrete(vehikel_basis_t *v);


    /**
     * Gegenstueck zu betrete()
     * @author Hj. Malthaner
     */
    void verlasse(vehikel_basis_t *v);


    /**
     * Sets the rail block to which this rail track belongs
     *
     * @author Hj. Malthaner
     */
    void setze_blockstrecke(blockhandle_t bs);


    /**
     * gibt ein Handle für die Blockstercke zu der die runway
     * gehört zurück.
     * @author Hj. Malthaner
     */
    inline const blockhandle_t & gib_blockstrecke() const {return bs;};


    /**
     * kann blockstrecke betreten werden ?
     * da die eigne blockstrecke immer von einem selbst belegt ist
     * reicht es, zu prüfen, ob die andere blockstrecke frei ist
     * @author Hj. Malthaner
     */
    bool ist_frei() const;


    void rdwr(loadsave_t *file);
};

#endif
