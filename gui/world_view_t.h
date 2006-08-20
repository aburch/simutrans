/*
 * world_view_t.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef world_view_t_h
#define world_view_t_h

#include "../ifc/gui_komponente.h"

class karte_t;
struct event_t;

/**
 * Displays a little piece of the world
 *
 * @autor Hj. Malthaner
 */
class world_view_t : public gui_komponente_t
{
private:

    /**
     * The location to display.
     * @autor Hj. Malthaner
     */
    koord location;


    /**
     * The world to display.
     * @autor Hj. Malthaner
     */
    karte_t *welt;



public:

    world_view_t(karte_t *welt, koord location);

    /**
     * Sets the location to be displayed.
     * @author Hj. Malthaner
     */
    void set_location(koord l) {location=l;};


    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    void infowin_event(const event_t *);


    /**
     * Zeichnet die Komponente
     * @author Hj. Malthaner
     */
    virtual void zeichnen(koord offset) const;
};

#endif
