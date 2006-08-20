/*
 * kennfarbe.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "../simdebug.h"
#include "../simevent.h"
#include "../simimg.h"
#include "../simplay.h"
#include "../simworld.h"
#include "../besch/skin_besch.h"
#include "../simskin.h"
#include "../dataobj/translator.h"
#include "../utils/cbuffer_t.h"
#include "kennfarbe.h"


extern "C" {
#include "../simgraph.h"
}

farbengui_t::farbengui_t(karte_t *welt) : infowin_t(welt)
{
}


/**
 * Manche Fenster haben einen Hilfetext assoziiert.
 * @return den Dateinamen für die Hilfe, oder NULL
 * @author Hj. Malthaner
 */
const char * farbengui_t::gib_hilfe_datei() const
{
  return "color.txt";
}


/**
 * in top-level fenstern wird der Name in der Titelzeile dargestellt
 * @return den nicht uebersetzten Namen der Komponente
 * @author Hj. Malthaner
 */
const char * farbengui_t::gib_name() const
{
  return "Meldung";
}


/**
 * gibt den Besitzer zurück
 *
 * @author Hj. Malthaner
 */
spieler_t* farbengui_t::gib_besitzer() const
{
    return welt->gib_spieler(0);
}


/**
 * Jedes Objekt braucht ein Bild.
 *
 * @author Hj. Malthaner
 * @return Die Nummer des aktuellen Bildes für das Objekt.
 */
int farbengui_t::gib_bild() const
{
    return skinverwaltung_t::farbmenu->gib_bild_nr(0);
}


void farbengui_t::info(cbuffer_t & buf) const
{
  buf.append(translator::translate("COLOR_CHOOSE\n"));
}

koord farbengui_t::gib_fenstergroesse() const
{
    return koord(216, 84);
}


void farbengui_t::infowin_event(const event_t *ev)
{
  infowin_t::infowin_event(ev);

  if(IS_LEFTCLICK(ev)) {
    if(ev->mx >= 136 && ev->mx <= 200) {

      const int x = (ev->mx-136)/32;
      const int y = (ev->my-16)/8;

      gib_besitzer()->kennfarbe = 0;

      const int f = (y + x*8);

      if(f>=0 && f<16) {
	display_set_player_color(f);
      }
    }
  }
}
