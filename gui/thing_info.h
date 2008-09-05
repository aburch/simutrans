/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_thing_info_h
#define gui_thing_info_h

#include "../simdebug.h"
#include "../simdings.h"
#include "gui_frame.h"
#include "components/gui_world_view_t.h"
#include "../utils/cbuffer_t.h"

/**
 * An adapter class to display info windows for things (objects)
 *
 * @author Hj. Malthaner
 * @date 22-Nov-2001
 */
class ding_infowin_t : public gui_frame_t
{
protected:
    world_view_t view;

    static cbuffer_t buf;

    /**
     * The thing we observe. The thing will delete this object
     * if self deleted.
     * @author Hj. Malthaner
     */
    const ding_t* ding;

		KOORD_VAL calc_draw_info( koord, bool ) const;

public:
    ding_infowin_t(const ding_t* ding);

    /**
     * @return window title
     *
     * @author Hj. Malthaner
     * @see simwin
     */
    virtual const char * gib_name() const { return ding->gib_name(); }

    /**
     * @return the text to display in the info window
     *
     * @author Hj. Malthaner
     * @see simwin
     */
    virtual void info(cbuffer_t & buf) const { ding->info(buf); }

    /**
     * @return a pointer to the player who owns this thing
     *
     * @author Hj. Malthaner
     */
    virtual spieler_t* gib_besitzer() const { return ding->gib_besitzer(); }

    /**
     * @return the current map position
     *
     * @author Hj. Malthaner
     */
    virtual koord3d gib_pos() const { return ding->gib_pos(); }

	/**
	* komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	* das Fenster, d.h. es sind die Bildschirmkoordinaten des Fensters
	* in dem die Komponente dargestellt wird.
	*/
	virtual void zeichnen(koord pos, koord gr);
};


#endif
