/*
 * gui_divider.cc
 *
 * Copyright (c) 2001 Hansjörg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "gui_divider.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../simwin.h"



/**
 * Konstruktor
 *
 * @author Markus Weber
 */
gui_divider_t::gui_divider_t()
{
    //max = 0;
    //text = NULL;
}

/**
 * Destruktor
 *
 * @author Markus Weber
 */
gui_divider_t::~gui_divider_t()
{
//    text = NULL;
}




/**
 * Zeichnet die Komponente
 * @author Markus Weber
 */
void gui_divider_t::zeichnen(koord offset) const
{


    display_ddd_box(pos.x+offset.x, pos.y+offset.y,
                       groesse.x, groesse.y,
 		       MN_GREY0, MN_GREY4);



/*    if(text) {
	const int width = proportional_string_width(text);

	display_proportional(pos.x+offset.x+2, pos.y+offset.y-2,
                             text,
			     ALIGN_LEFT, SCHWARZ, true);

    }*/
}
