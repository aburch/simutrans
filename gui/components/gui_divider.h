/*
 * gui_divider.h
 *
 * Copyright (c) 2001 Hansjörg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_components_gui_divider_h
#define gui_components_gui_divider_h

#include "../../ifc/gui_komponente.h"
#include "../../simgraph.h"
#include "../../simcolor.h"


/**
 * Eine einfache Trennlinie
 *
 * @date 30-Oct-01
 * @author Markus Weber
 */
class gui_divider_t : public gui_komponente_t
{
public:
    /**
     * Zeichnet die Komponente
     * @author Markus Weber
     */
    void zeichnen(koord offset) { display_ddd_box_clip(pos.x+offset.x, pos.y+offset.y, groesse.x, groesse.y, MN_GREY0, MN_GREY4); }
};

#endif
