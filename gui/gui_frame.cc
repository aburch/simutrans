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
 * @author Hj. Malthaner
 */
gui_frame_t::gui_frame_t(const char *name)
{
    this->name = name;

    groesse = koord(200, 100);

    farben.titel  = WIN_TITEL;
    farben.hell   = MN_GREY4;
    farben.mittel = MN_GREY2;
    farben.dunkel = MN_GREY0;

    opaque = false;

    container.setze_pos(koord(0,16));

    set_resizemode (no_resize); //25-may-02	markus weber	added
}


/**
 * Konstruktor
 * @param name Fenstertitel
 * @param color Besitzerfarbe
 * @author Hj. Malthaner
 */
gui_frame_t::gui_frame_t(const char *name, int color)
{
    this->name = name;

    groesse = koord(200, 100);

    farben.titel  = color;
    farben.hell   = MN_GREY4;
    farben.mittel = MN_GREY2;
    farben.dunkel = MN_GREY0;

    opaque = false;

    container.setze_pos(koord(0,16));

    set_resizemode (no_resize); //25-may-02	markus weber	added
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
 * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
 * zurück
 * @author Hj. Malthaner
 */
fensterfarben gui_frame_t::gib_fensterfarben() const
{
    return farben;
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
  if(opaque) {
    const fensterfarben f = gib_fensterfarben();

    // Hajo: skinned windows code
    // fensterkoerper zeichnen
    PUSH_CLIP(pos.x+1,pos.y+16,gr.x-2,gr.y-16);

    const int img = skinverwaltung_t::window_skin->gib_bild(0)->gib_nummer();

    for(int j=0; j<gr.y; j+=64) {
      for(int i=0; i<gr.x; i+=64) {
	display_color_img(img, pos.x+1 + i, pos.y+16 + j, 0, false, true);
      }
    }

    POP_CLIP();


    // Hajo: left, right
    display_vline_wh(pos.x, pos.y+16, gr.y-16, f.hell, true);
    display_vline_wh(pos.x+gr.x-1, pos.y+16, gr.y-16, f.dunkel, true);

    // Hajo: bottom line
    display_fillbox_wh(pos.x, pos.y+gr.y-1, gr.x, 1, f.dunkel, true);

  }

  container.zeichnen(pos);
}
