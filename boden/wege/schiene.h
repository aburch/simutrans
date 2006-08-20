/*
 * schiene.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef boden_wege_schiene_h
#define boden_wege_schiene_h


#include "../../railblocks.h"
#include "weg.h"


/**
 * Klasse für Schienen in Simutrans.
 * Auf den Schienen koennen Züge fahren.
 * Jede Schiene gehört zu einer Blockstrecke
 *
 * @author Hj. Malthaner
 * @version 1.0
 */
class schiene_t : public weg_t
{
private:

    /**
     * Rail block handle. Should be always bound to a rail block,
     * except a short time after creation. Everything else is an error!
     *
     * @author Hj. Malthaner
     */
    blockhandle_t bs;


    /**
     * Track system type of this track
     * @author Hj. Malthaner
     */
    uint8 is_electrified;


public:

    void setze_elektrisch(bool yesno);
    uint8 ist_elektrisch() const;


    /**
     * File loading constructor.
     *
     * @author Hj. Malthaner
     */
    schiene_t(karte_t *welt, loadsave_t *file);


    /**
     * Basic constructor.
     *
     * @author Hj. Malthaner
     */
    schiene_t(karte_t *welt);

    /**
     * Basic constructor with top_speed
     * @author Hj. Malthaner
     */
    schiene_t(karte_t *welt, int top_speed);


    /**
     * Destruktor. Entfernt etwaige Debug-Meldungen vom Feld
     *
     * @author Hj. Malthaner
     */
    virtual ~schiene_t();


    /**
     * Calculates the image of this pice of railroad track.
     *
     * @author Hj. Malthaner
     */
    virtual int calc_bild(koord3d pos) const;


    inline const char *gib_typ_name() const {return "Schiene";};
    inline typ gib_typ() const {return schiene;};


    /**
     * @return Infotext zur Schiene
     * @author Hj. Malthaner
     */
    void info(cbuffer_t & buf) const;


    /**
     * manche schienen sind eingangs/ausgangsschienen für blcokstrecken
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
     * gibt ein Handle für die Blockstercke zu der die Schiene
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
