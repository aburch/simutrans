/*
 * infowin.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


/* infowin.cc
 *
 * Basisklasse fuer Infofenster
 * Hj. Malthaner, 2000
 */


#include "infowin.h"
#include "../simcolor.h"
#include "../simimg.h"
#include "../simgraph.h"
#include "../simwin.h"
#include "../simplay.h"
#include "../simworld.h"
#include "../dataobj/translator.h"
#include "../utils/cbuffer_t.h"
#include "../simskin.h"
#include "../besch/skin_besch.h"


karte_t * infowin_t::welt = NULL;


infowin_t::~infowin_t() {
    destroy_win(this);
}


spieler_t * infowin_t::gib_besitzer() const
{
    if(welt) {
	return welt->gib_spieler(0);
    } else {
	return 0;
    }
}


void infowin_t::unpress_buttons()
{
    vector_tpl<button_t> *buttons = gib_fensterbuttons();

    if(buttons != NULL) {
	for(uint32 i=0; i<buttons->get_count(); i++) {
	    buttons->at(i).pressed = false;
	}
    }
}

void infowin_t::draw_buttons(const koord pos,
			     const int button_farbe)
{
    vector_tpl<button_t> *buttons = gib_fensterbuttons();

    if(buttons != NULL) {
	for(uint32 i=0; i<buttons->get_count(); i++) {
	    buttons->at(i).zeichnen(pos, button_farbe);
	}
    }
}


int
infowin_t::calc_fensterhoehe_aus_info() const
{
  static cbuffer_t buf (8192);

  buf.clear();
  info(buf);

  return MAX(count_char(buf, '\n')*LINESPACE+36, 92);
}


koord
infowin_t::gib_fenstergroesse() const
{
    return koord(224, calc_fensterhoehe_aus_info());
}


/**
 * @return Die aktuelle grund-Koordinate des Objekts
 *
 * @author V. Meyer
 */
koord3d
infowin_t::gib_pos() const
{
  return koord3d::invalid;
};

/**
 * Jedes Objekt braucht ein Bild.
 *
 * @author Hj. Malthaner
 * @return Die Nummer des aktuellen Bildes für das Objekt.
 */
int
infowin_t::gib_bild() const
{
  return IMG_LEER;
}


/**
 * Das Bild kann im Fenster über Offsets plaziert werden
 *
 * @author Hj. Malthaner
 * @return den x,y Offset des Bildes im Infofenster
 */
koord
infowin_t::gib_bild_offset() const
{
  return koord(-16,0);
}


/**
 *
 * @author Hj. Malthaner
 * @return eine NULL-Terminierte Liste von Buttons fuer das
 * Beobachtungsfenster
 */
vector_tpl<button_t>*
infowin_t::gib_fensterbuttons()
{
  return NULL;
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void
infowin_t::infowin_event(const event_t *ev)
{
    //  printf("infowin_t::infowin_event %d, %d\n", ev->ev_class, ev->ev_code);

    if(ev->ev_class == EVENT_RELEASE) {
	unpress_buttons();
    }

    if(IS_LEFTCLICK(ev)) {

	koord k = gib_fenstergroesse();

	if(ev->mx > k.x-72 && ev->mx < k.x-8 &&
	   ev->my > 16 && ev->my < 16+64) {

	    if(welt->ist_in_kartengrenzen(gib_pos().gib_2d())) {
		welt->setze_ij_off(gib_pos().gib_2d() + koord(-5,-5));
	    }
	}
    }
}

static const koord offsets[16] =
{
    koord(-2, -1),
    koord(-32, -48),
    koord(-1, -2),
    koord(32, -48),

    koord(-1, -1),
    koord(0, -32),

    koord(-1, 0),
    koord(-32, -16),
    koord(0, -1),
    koord(32, -16),

    koord(0, 0),
    koord(0, 0),

    koord(0, 1),
    koord(-32, 16),
    koord(1, 0),
    koord(32, 16),
};


/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 */
void infowin_t::zeichnen(koord pos, koord gr)
{
    static cbuffer_t buf (8192);

    const fensterfarben f = gib_fensterfarben();
    const int x = pos.x;
    const int y = pos.y;
    const int w = gr.x;
    const int h = gr.y;


    // Hajo: skinned windows code
    // fensterkoerper zeichnen
    PUSH_CLIP(pos.x+1,pos.y+16,gr.x-2,gr.y-16);

    const int img = skinverwaltung_t::window_skin->gib_bild(0)->gib_nummer();

    for(int j=0; j<gr.y; j+=64) {
      for(int i=0; i<gr.x; i+=64) {
	display_img(img, pos.x+1 + i, pos.y+16 + j, false, true);
      }
    }


    POP_CLIP();


    // Hajo: left, right
    display_vline_wh(x, y+16, h-16, f.hell, true);
    display_vline_wh(x+w-1, y+16, h-16, f.dunkel, true);

    // Hajo: bottom line
    display_fillbox_wh(x, y+h-1, w, 1, f.dunkel, true);

    const planquadrat_t * plan = welt->lookup(gib_pos().gib_2d());

    if(plan) {
	view.set_location(gib_pos().gib_2d());

	display_ddd_box(x+w-77, y+23, 66, 57, f.dunkel, f.hell);
	view.zeichnen(koord(x+w-76, y+24));
    } else {
	const int bild  = gib_bild();
	const koord off = gib_bild_offset();

	if(gib_besitzer()) {
	    display_color_img(bild,x+w-64+off.x,
			      y+16+off.y,
			      gib_besitzer()->kennfarbe,
			      false,
			      true);
	} else {
	    display_img(bild,x+w-64+off.x, y+16+off.y, false, true);
	}
    }

    buf.clear();
    info(buf);

    display_multiline_text(x+11, y+24, buf, SCHWARZ);

    /*
    if(buf.len() == 0) {
        display_proportional(x+4, y+32, translator::translate("Keine Info."), ALIGN_LEFT, SCHWARZ, true);
    } else {
	display_multiline_text(x+11, y+22, buf, SCHWARZ);
    }
    */

    int button_farbe = SCHWARZ;

    if(gib_besitzer() != welt->gib_spieler(0)) {
	button_farbe = GRAU2;
    }

    draw_buttons(pos, button_farbe);
}

/**
 * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
 * zurück
 */
fensterfarben infowin_t::gib_fensterfarben() const
{
  fensterfarben f;

  const spieler_t *sp = gib_besitzer();

  f.titel  = (sp != NULL) ? sp->kennfarbe : WIN_TITEL;
  f.hell   = MN_GREY4;
  f.mittel = MN_GREY2;
  f.dunkel = MN_GREY0;

  return f;
}
