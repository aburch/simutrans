/*
 * gui_image.h
 *
 * just displays an image
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_image_h
#define gui_image_h

#include "../../simimg.h"
#include "../../simgraph.h"
#include "../../ifc/gui_komponente.h"


class gui_image_t : public gui_komponente_t
{
private:
	image_id id;

public:
    /**
     * Konstruktor
     * @author Hansjörg Malthaner
     */
    gui_image_t(image_id i=IMG_LEER) { id = i; }

    /**
     * set the text without translation
     * @author Hansjörg Malthaner
     */
    void set_image(image_id i) { id = i; }

    /**
     * Zeichnet die Komponente
     * @author Hj. Malthaner
     */
    void zeichnen(koord offset) const { display_color_img( id, pos.x+offset.x, pos.y+offset.y, 0, false, true ); }
};

#endif
