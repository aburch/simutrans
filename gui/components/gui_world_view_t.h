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

#include "../../ifc/gui_komponente.h"
#include "../../dataobj/koord3d.h"
#include "../../tpl/vector_tpl.h"

class ding_t;
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
	koord3d location;

	/**
	* The object to display
	* @autor Hj. Malthaner
	*/
	ding_t *ding;

	// offsets are stored
	vector_tpl<koord>offsets;
	sint16 raster;	// for this rastersize

	/**
	* The world to display.
	* @autor Hj. Malthaner
	*/
	karte_t *welt;



public:
    world_view_t(karte_t *welt, koord3d location);

    world_view_t(karte_t *welt, ding_t *dt);

    /**
     * Sets the location to be displayed.
     * @author Hj. Malthaner
     */
    void set_location(koord3d l) {location=l; ding = 0;}

    /**
     * Sets the location to be displayed.
     * @author Hj. Malthaner
     */
    void set_location(ding_t *dt) {location==koord3d::invalid; ding = dt;}

    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    void infowin_event(const event_t *);

    /**
     * resize window in response to a resize event
     * need to recalculate the list of offsets
     * @author prissi
     */
    virtual void setze_groesse(koord groesse);

    /**
     * Zeichnet die Komponente
     * @author Hj. Malthaner
     */
    virtual void zeichnen(koord offset);
};

#endif
