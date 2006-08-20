/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 *
 * Usage for Iso-Angband is granted. Usage in other projects is not
 * permitted without the agreement of the author.
 */

#ifndef ifc_karte_modell_h
#define ifc_karte_modell_h

#include "../dataobj/koord3d.h"


/**
 * Schnittstelle zwischen Weltansicht und Weltmodell.
 *
 * @author Hj. Malthaner
 * @version $Revsision$
 */
class karte_modell_t
{
private:
    /**
     * manche actionen machen eine neuanzeige der ganzen karte notwendig
     * diese aktionen müssen dirty auf true setzen
     */
    bool dirty;

    /* to rotate the view (without the buildings)
     * @author prissi
     */
    int rotation;

    koord ij_off;

public:
	virtual ~karte_modell_t() {}

    /**
     * Setzt das globale dirty flag.
     * @author Hj. Malthaner
     */
    void setze_dirty() {dirty=true;}


    /**
     * Löscht das globale dirty flag.
     * @author Hj. Malthaner
     */
    void setze_dirty_zurueck() {dirty=false;}


    /**
     * Frägt das globale dirty flag ab.
     * @author Hj. Malthaner
     */
    bool ist_dirty() const {return dirty;}


    /**
     * Ermittelt ij-Koordinate des Blickpunkts auf der Karte.
     * @author Hj. Malthaner
     */
    virtual koord gib_ij_off() const {return ij_off;}


    /**
     * Setzt i-Koordinate des Blickpunkts auf der Karte.
     * @author Hj. Malthaner
     */
    void setze_ij_off(koord ij) {ij_off=ij; dirty=true;}


    /**
     * Ermittelt x-Offset gescrollter Karte
     * @author Hj. Malthaner
     */
    virtual int gib_x_off() const = 0;


    /**
     * Ermittelt y-Offset gescrollter Karte
     * @author Hj. Malthaner
     */
    virtual int gib_y_off() const = 0;


    /**
     * Holt den Grundwasserlevel der Karte
     *
     * @author Hj. Malthaner
     */
    virtual int gib_grundwasser() const = 0;
};

#endif
