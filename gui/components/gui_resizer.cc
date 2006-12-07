/*
 * gui_resizer.cc
 *
 * Copyright (c) 2001 Hansjörg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../../simdebug.h"
#include "gui_resizer.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../simwin.h"



/**
 * Konstruktor
 *
 * @author Markus Weber
 */
gui_resizer_t::gui_resizer_t()
{
    followmouse = false;
}



/**
 * Zeichnet die Komponente
 * @author Markus Weber
 */
void gui_resizer_t::zeichnen(koord offset)
{
    int top  = pos.y+offset.y;
    int left = pos.x+offset.x;

//    display_fillbox_wh_clip(left+1, top+1, groesse.x-2, groesse.y-2, MN_GREY1, false);
//    display_ddd_box(left, top, groesse.x, groesse.y, MN_GREY0, MN_GREY4);

    display_ddd_box(left, top, groesse.x, groesse.y, MN_GREY4, MN_GREY0);
    display_ddd_box(left+1, top+1, groesse.x-2, groesse.y-2, MN_GREY3, MN_GREY1);
}




/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Markus Weber
 */
void gui_resizer_t::infowin_event(const event_t *ev)
{
  int x = ev->cx;
  int y = ev->cy;

  //if (getroffen(x, y)) {
     if(IS_LEFTDRAG(ev)) {

        vresize = 0;
        hresize = 0;

        switch (type) {
        case horizonal_resize:
           hresize = ev->mx - x;
        break;
        case vertical_resize:
           vresize = ev->my - y;
        break;
        case diagonal_resize:
           vresize = ev->my - y;
           hresize = ev->mx - x;

        default:
        break;
        }

		value_t p;
          call_listeners(p);
		if (followmouse)
          {
                setze_pos(koord(pos.x+hresize,pos.y+vresize));
          }
          change_drag_start(hresize, vresize);
     }
}


/**
 * Cancel the current resize operation
 * @author Markus Weber
 */
void gui_resizer_t::cancelresize()
{
    change_drag_start(0, 0);
}
