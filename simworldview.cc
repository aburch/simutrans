/*
 * simworldview.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>
#include <math.h>

#include "simdebug.h"
#include "simworldview.h"
#include "simworld.h"
#include "simplan.h"
#include "simplay.h"
#include "simdisplay.h"
#include "simdings.h"
#include "simskin.h"
#include "simwin.h"
#include "simgraph.h"
#include "simimg.h"
#include "simcolor.h"
#include "boden/grund.h"
#include "dataobj/umgebung.h"
#include "besch/grund_besch.h"

#include "simticker.h"


karte_vollansicht_t::karte_vollansicht_t(karte_t *welt) : karte_ansicht_t(welt)
{
    this->welt = welt;
//    create_win(0, display_get_height()-32, -1, new ticker_view_t(welt), w_frameless);
};


int karte_vollansicht_t::gib_anzeige_breite()
{
    return display_get_width();
}


int karte_vollansicht_t::gib_anzeige_hoehe()
{
    return display_get_height();
}



/**
 * Grund von i,j an Bildschirmkoordinate xpos,ypos zeichnen.
 * @author Hj. Malthaner
 */
void karte_vollansicht_t::display_boden(const int i, const int j, const int xpos, const int ypos, const bool dirty)
{
    const planquadrat_t * const plan = welt->lookup(koord(i,j));

    if(plan) {
	grund_t *gr = plan->gib_kartenboden();

	if(gr) {
	    const bool pos_dirty = welt->ist_dirty(gr->gib_pos());
	    gr->display_boden(xpos, ypos, dirty || pos_dirty);
	    if(pos_dirty) {
		mark_rect_dirty_wc(xpos-8*scale,
				   ypos-32*scale-height_scaling(gr->gib_hoehe()*scale),
		                   xpos+80*scale,
				   ypos+76*scale-height_scaling(gr->gib_hoehe()*scale));

		welt->markiere_clean(gr->gib_pos());
	    }
	}
    } else {
	display_img(grund_besch_t::ausserhalb->gib_bild(hang_t::flach),
		    xpos,
		    ypos - tile_raster_scale_y( welt->gib_grundwasser(), get_tile_raster_width() ),
		    dirty);
    }
}


/**
 * Objekte von i,j an Bildschirmkoordinate xpos,ypos zeichnen.
 * @author Hj. Malthaner
 */
void karte_vollansicht_t::display_dinge(const int i, const int j, const int xpos, const int ypos, bool dirty)
{
    const planquadrat_t * const plan = welt->lookup(koord(i,j));

    if(plan) {
	grund_t *gr = plan->gib_boden_bei(0u);

	if(gr) {
#if 0
		if(gr->gib_typ()==grund_t::fundament) {
			// there may be slopes ...
			gr->display_boden(xpos, ypos, dirty || welt->ist_dirty(gr->gib_pos()));
		}
#endif
 	    gr->display_dinge(xpos, ypos, dirty || welt->ist_dirty(gr->gib_pos()));
	}

	for(unsigned int i = 1; i < plan->gib_boden_count(); i++) {
	    grund_t *gr = plan->gib_boden_bei(i);
	    koord3d pos ( gr->gib_pos() );

	    gr->display_boden(xpos, ypos, dirty || welt->ist_dirty(pos));
 	    gr->display_dinge(xpos, ypos, dirty || welt->ist_dirty(pos));

	    if(welt->ist_dirty(pos)) {
		mark_rect_dirty_wc(xpos-8, ypos-32-height_scaling(gr->gib_hoehe()*scale),
		                   xpos+80, ypos+76-height_scaling(gr->gib_hoehe()*scale));

		welt->markiere_clean(pos);
	    }
	}
    }

#if 0
	ding_t *zeiger = welt->gib_zeiger();
	if(zeiger->gib_pos().gib_2d() == koord(i,j)) {
		zeiger->display(xpos,
			ypos-zeiger->gib_pos().z  * (get_tile_raster_width() >> 6),
			dirty);
		zeiger->clear_flag(ding_t::dirty);
	}
#endif
	// Debugging
    // if(welt->ist_markiert(koord3d(pos.x, pos.y, -32))) {
    //	display_img(70, xpos, ypos - welt->gib_grundwasser(), true);
    // }
}



// draw the map
void karte_vollansicht_t::display(bool dirty)
{
	const int width = display_get_width();
	const int height = display_get_height()-32-16-(ticker_t::get_instance()->count()>0?16:0);

	display_setze_clip_wh( 0, 32, width, height );

	karte_ansicht_t::display(dirty);

	for(int x=0; x<8; x++) {
		welt->gib_spieler(x)->display_messages(welt->gib_x_off(),welt->gib_y_off(),width);
	}
}



/* mousepointer from karte_t
 * @author Hj. Malthaner
 */
ding_t *karte_vollansicht_t::gib_zeiger()
{
	return welt->gib_zeiger();
}
