/*
 * thing_info.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_thing_info_h
#define gui_thing_info_h

#include "../simdebug.h"
#include "../tpl/vector_tpl.h"
#include "../simdings.h"
#include "components/gui_button.h"
#include "infowin.h"

#ifdef _MSC_VER
#pragma warning(disable:4250)
#endif

/**
 * An adapter class to display info windows for things (objects)
 *
 * @author Hj. Malthaner
 * @date 22-Nov-2001
 */
class ding_info_t : virtual public gui_fenster_t
{
public:

    /**
     * The thing we observe. The thing will delete this object
     * if self deleted.
     * @author Hj. Malthaner
     */
    ding_t *ding;


    /**
     * This function is needed for the pointermap_tpl in
     * ding_t::ding_infos.
     * @author V. Meyer
     */
    ding_t *gib_key_obj() const
    {
	return ding;
    }

public:


    /**
     * Basic constructor.
     * @author Hj. Malthaner
     */
    ding_info_t(ding_t *ding) {
	this->ding = ding;
    };


    /**
     * destructor.
     * @author V. Meyer
     */
    ~ding_info_t()
    {
	ding->entferne_ding_info();
    }


    /**
     * @return window title
     *
     * @author Hj. Malthaner
     * @see simwin
     */
    const char * gib_name() const {
	return ding->gib_name();
    }


    /**
     * @return the text to display in the info window
     *
     * @author Hj. Malthaner
     * @see simwin
     */
    virtual void info(cbuffer_t & buf) const {
	ding->info(buf);
    }


    /**
     * @return a pointer to the player who owns this thing
     *
     * @author Hj. Malthaner
     */
    virtual spieler_t* gib_besitzer() const {
	return ding->gib_besitzer();
    }


    /**
     * @return the current map position
     *
     * @author Hj. Malthaner
     */
    virtual koord3d gib_pos() const {
	return ding->gib_pos();
    }
};


/**
 * The "normal" ding_infowin_t is this. Only townhalls currently have other
 * ding_info_t-windows.
 *
 * @author V. Meyer
 */
class ding_infowin_t : public ding_info_t, public infowin_t {
public:
    ding_infowin_t(karte_t *welt, ding_t *ding) : ding_info_t(ding), infowin_t(welt) {}

    /*
     * Für den Aufruf der richtigen Methoden sorgen!
     */
    virtual void info(cbuffer_t & buf) const { ding_info_t::info(buf); }
    virtual spieler_t* gib_besitzer() const { return ding_info_t::gib_besitzer(); }
    virtual koord3d gib_pos() const { return ding_info_t::gib_pos(); }
    virtual const char * gib_name() const { return ding_info_t::gib_name(); }
};

#endif
