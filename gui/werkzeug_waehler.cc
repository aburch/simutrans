#include <stdlib.h>

#include "../simcolor.h"
#include "../simimg.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../simwin.h"
#include "../simgraph.h"
#include "../simwerkz.h"
#include "../besch/skin_besch.h"

#include "werkzeug_waehler.h"


werkzeug_waehler_t::werkzeug_waehler_t(karte_t *welt,
                                       const skin_besch_t *bilder,
				       char *titel)

{
    this->welt = welt;
    this->bild1_nr = bilder->gib_bild_nr(0);
    this->bild2_nr = bilder->gib_bild_nr(1);
    this->titel  = titel;
    this->hilfe_datei = NULL;

    for(int i=0; i<8; i++) {
	setze_werkzeug(i, NULL, 0, 0, 0, 0);
	tooltips[i] = 0;
    }
}

void werkzeug_waehler_t::setze_werkzeug(int index,
                                        int (* wz1)(spieler_t *, karte_t *, koord),
                                        int zeiger_bild,
                                        int versatz,
					int sound_ok,
					int sound_ko)
{
  setze_werkzeug(index, (int (*)(spieler_t *, karte_t *, koord, value_t))wz1,
		 zeiger_bild, versatz, 0l, sound_ok, sound_ko);
}

void werkzeug_waehler_t::setze_werkzeug(int index,
                                        int (* wz1)(spieler_t *, karte_t *, koord, value_t),
                                        int zeiger_bild,
                                        int versatz,
					value_t param,
					int sound_ok,
					int sound_ko)
{
    if(index >= 0 && index < 8) {
	wz[index] = wz1;
	wz_zeiger[index] = zeiger_bild;
	wz_versatz[index] = versatz;
	wz_param[index] = param;

	wz_sound_ok[index] = sound_ok;
	wz_sound_ko[index] = sound_ko;


    }
}


/**
 * Sets the tooltip text for a tool.
 * Text must already be translated.
 * @author Hj. Malthaner
 */
void werkzeug_waehler_t::set_tooltip(int tipnr, const char * text)
{
  if(tipnr >= 0 && tipnr <= 8) {
    tooltips[tipnr] = text;
  }
}


int werkzeug_waehler_t::zeige_info(int magic)
{
    // an mauskoordinate anzeigen
    return create_win(-1, -1, -1, this, w_autodelete, magic);
}

const char *werkzeug_waehler_t::gib_name() const
{
    return titel;
}

/**
 * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
 * zurück
 */
fensterfarben werkzeug_waehler_t::gib_fensterfarben() const
{
  fensterfarben f;

  f.titel  = WIN_TITEL;
  f.hell   = GRAU6;
  f.mittel = GRAU5;
  f.dunkel = GRAU3;

  return f;
}

void werkzeug_waehler_t::infowin_event(const event_t *ev)
{
    if(IS_LEFTCLICK(ev)) {

	const int x = ev->mx/32;
	const int y = (ev->my-16) / 32;

	const int wz_idx = y*4 + x;

	//    printf("%d\n", wz_idx);

	if(wz[wz_idx] != NULL) {
	    welt->setze_maus_funktion(wz[wz_idx],
				      wz_zeiger[wz_idx],
				      wz_versatz[wz_idx],
				      wz_param[wz_idx],
				      wz_sound_ok[wz_idx],
				      wz_sound_ko[wz_idx]);
	}
    } else if(ev->ev_class == INFOWIN &&
	      ev->ev_code == WIN_CLOSE) {
	    welt->setze_maus_funktion(wkz_abfrage,
	                              skinverwaltung_t::fragezeiger->gib_bild_nr(0),
                                      karte_t::Z_PLAN,
				      0,
				      0);
    }
}

void werkzeug_waehler_t::zeichnen(koord pos, koord)
{
    display_color_img(bild1_nr, pos.x, pos.y+16, 0, false, true);
    display_color_img(bild2_nr, pos.x+64, pos.y+16, 0, false, true);

    const int xdiff = (gib_maus_x() - pos.x) >> 5;
    const int ydiff = (gib_maus_y() - pos.y - 16) >> 5;

    // print("%d %d\n", xdiff, ydiff);

    if(xdiff >= 0 && xdiff <= 3 && ydiff >= 0 && ydiff <= 1) {
      const int tipnr = ydiff * 4 + xdiff;

      win_set_tooltip(gib_maus_x() + 16,
		      gib_maus_y() - 16,
		      tooltips[tipnr]);
    }


}
