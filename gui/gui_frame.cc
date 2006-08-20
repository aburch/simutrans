/*
 * gui_frame.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#include <stdio.h>

#include "gui_frame.h"
#include "../simcolor.h"
#include "../simgraph.h"

#include "../simskin.h"
#include "../besch/skin_besch.h"



/**
 * Konstruktor
 * @param name Fenstertitel
 * @param color Besitzerfarbe
 * @author Hj. Malthaner
 */
gui_frame_t::gui_frame_t(const char *name, const spieler_t *sp)
{
    this->name = name;
    groesse = koord(200, 100);
    owner = sp;
    opaque = false;
    container.setze_pos(koord(0,16));
    set_resizemode (no_resize); //25-may-02	markus weber	added
    dirty = true;
}



/**
 * Fügt eine Komponente zum Fenster hinzu.
 * @author Hj. Malthaner
 */
void gui_frame_t::add_komponente(gui_komponente_t *komp)
{
    container.add_komponente(komp);
}


/**
 * Entfernt eine Komponente aus dem Container.
 * @author Hj. Malthaner
 */
void gui_frame_t::remove_komponente(gui_komponente_t *komp)
{
    container.remove_komponente(komp);
}



/**
 * Der Name wird in der Titelzeile dargestellt
 * @return den nicht uebersetzten Namen der Komponente
 * @author Hj. Malthaner
 */
const char * gui_frame_t::gib_name()  const
{
    return name;
};


/**
 * setzt den Namen (Fenstertitel)
 * @author Hj. Malthaner
 */
void gui_frame_t::setze_name(const char *name)
{
    this->name = name;
}


/**
 * setzt die Transparenz
 * @author Hj. Malthaner
 */
void gui_frame_t::setze_opaque(bool janein)
{
    opaque = janein;
}

/**
 * @return gibt wunschgroesse für das Darstellungsfenster zurueck
 * @author Hj. Malthaner
 */
koord gui_frame_t::gib_fenstergroesse() const
{
    return groesse;
}



/**
 * @return returns the usable width and heigth of the window
 * @author Markus Weber
 * @date   11-May-2002
 */
koord gui_frame_t::get_client_windowsize() const
{
  // return groesse-koord(dragger_x_size,16+dragger_y_size);
  return groesse-koord(0, 16);
}


/**
 * Setzt die Fenstergroesse
 * @author Hj. Malthaner
 */
void gui_frame_t::setze_fenstergroesse(koord groesse)
{
    // %DB0 printf( "\nMessage: gui_frame_t::setze_fenstergroesse( koord groesse ): Fenster|Window %p : Koord is (%d,%d)", (void*)this, groesse.x, groesse.y );

    // minimal width //25-may-02	markus weber	added
    if (groesse.x < min_windowsize.x) {
        groesse.x = min_windowsize.x;
    }

    // minimal heigth //25-may-02	markus weber	added
    if (groesse.y < min_windowsize.y) {
        groesse.y = min_windowsize.y;
    }

    this->groesse = groesse;
    dirty = true;
}



/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void gui_frame_t::infowin_event(const event_t *ev)
{
    // %DB0 printf( "\nMessage: gui_frame_t::infowin_event( event_t const * ev ) : Fenster|Window %p : Event is %d", (void*)this, ev->ev_class );

    if(IS_WINDOW_RESIZE(ev)) {
      koord delta (ev->mx - ev->cx, ev->my - ev->cy);
      resize(delta);
    } else if(IS_WINDOW_MAKE_MIN_SIZE(ev)) {
	  setze_fenstergroesse( koord(0,0) ) ;
	  resize( koord(0,0) ) ;
	}
	else if(ev->ev_class==INFOWIN  &&  (ev->ev_code==WIN_CLOSE  ||  ev->ev_code==WIN_OPEN)) {
		dirty = true;
	}

    event_t ev2 = *ev;
    translate_event(&ev2, 0, -16);
    container.infowin_event(&ev2);
}


/**
* Set minimum size of the window
* @author Markus Weber
* @date   11-May-2002
*/
void gui_frame_t::set_min_windowsize(koord size)
{
    this->min_windowsize = size;
}



/**
 * resize window in response to a resize event
 * @author Markus Weber, Hj. Malthaner
 * @date 11-may-02
 */
void gui_frame_t::resize(const koord delta)
{
    koord size_change = delta;
    koord new_size = groesse + delta;

    // resize window to the minimal width
    if (new_size.x < min_windowsize.x) {
      new_size.x = min_windowsize.x;
      size_change.x = 0;
    }

    // resize window to the minimal heigth
    if (new_size.y < min_windowsize.y) {
      new_size.y = min_windowsize.y;
      size_change.y = 0;
    }

    // resize window
    setze_fenstergroesse(new_size);

    // change drag start
    change_drag_start(size_change.x, size_change.y);
}


/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void gui_frame_t::zeichnen(koord pos, koord gr)
{
	// ok, resized, move or draw for the first time
	if(dirty) {
		mark_rect_dirty_wc(pos.x,pos.y,pos.x+gr.x,pos.y+gr.y);
		dirty = false;
	}

	if(opaque) {
		// Hajo: skinned windows code
		// draw background
		PUSH_CLIP(pos.x+1,pos.y+16,gr.x-2,gr.y-16);

		const int img = skinverwaltung_t::window_skin->gib_bild(0)->gib_nummer();

		for(int j=0; j<gr.y; j+=64) {
			for(int i=0; i<gr.x; i+=64) {
				// the background will not trigger a redraw!
				display_color_img(img, pos.x+1 + i, pos.y+16 + j, 0, false, false);
			}
		}
		POP_CLIP();

		// Hajo: left, right
		display_vline_wh(pos.x, pos.y+16, gr.y-16, MN_GREY4, false);
		display_vline_wh(pos.x+gr.x-1, pos.y+16, gr.y-16, MN_GREY0, false);

		// Hajo: bottom line
		display_fillbox_wh(pos.x, pos.y+gr.y-1, gr.x, 1, MN_GREY0, false);
	}

	container.zeichnen(pos);
}
