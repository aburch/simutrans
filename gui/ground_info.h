/*
 * ground_info.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_ground_info_h
#define gui_ground_info_h

#ifndef simdebug_h
#include "../simdebug.h"
#endif

#ifndef tpl_vector_h
#include "../tpl/vector_tpl.h"
#endif

#ifndef boden_grund_h
#include "../boden/grund.h"
#endif

#ifndef gui_button_h
#include "button.h"
#endif

#ifndef gui_infowin_h
#include "infowin.h"
#endif

/**
 * An adapter class to display info windows for ground (floor) objects
 *
 * @author Hj. Malthaner
 * @date 20-Nov-2001
 */
class grund_info_t : public infowin_t
{
public:
    /**
     * The ground we observe. The ground will delete this object
     * if self deleted.
     * @author Hj. Malthaner
     */
    grund_t *gr;


    /**
     * This function is needed for the pointermap_tpl in
     * grund_t::grund_infos.
     * @author V. Meyer
     */
    grund_t *gib_key_obj() const
    {
	return gr;
    }

public:

    /**
     * Basic constructor.
     * @author Hj. Malthaner
     */
    grund_info_t(karte_t *welt, grund_t *gr) : infowin_t(welt) {
	this->gr = gr;
    }


    /**
     * destructor.
     * @author V. Meyer
     */
    ~grund_info_t()
    {
	gr->entferne_grund_info();
    }


    /**
     * @return window title
     *
     * @author Hj. Malthaner
     * @see simwin
     */
    const char * gib_name() const {
	return gr->gib_name();
    }


    /**
     * @return the text to display in the info window
     *
     * @author Hj. Malthaner
     * @see simwin
     */
    virtual void info(cbuffer_t & buf) const {
	gr->info(buf);
    }


    /**
     * @return a pointer to the player who owns this thing
     *
     * @author Hj. Malthaner
     */
    virtual spieler_t* gib_besitzer() const {
	return gr->gib_besitzer();
    }


    /**
     * @return the current map position
     *
     * @author Hj. Malthaner
     */
    virtual koord3d gib_pos() const {
	return gr->gib_pos();
    }
};

#endif
