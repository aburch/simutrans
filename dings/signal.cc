/*
 * signal.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../boden/wege/schiene.h"
#include "../boden/grund.h"
#include "../simimg.h"
#include "../dataobj/loadsave.h"
#include "../simskin.h"
#include "../besch/skin_besch.h"
#include "../utils/cbuffer_t.h"
#include "../blockmanager.h"

#include "signal.h"


signal_t::signal_t(karte_t *welt, loadsave_t *file) : ding_t (welt)
{
  rdwr(file, true);

  step_frequency = 255;
}


signal_t::signal_t(karte_t *welt, koord3d pos, ribi_t::ribi dir) :
  ding_t(welt, pos)
{
  this->dir = dir;

  zustand = rot;
  blockend = false;

  calc_bild();

  step_frequency = 255;
}


signal_t::~signal_t()
{
    weg_t *weg = welt->lookup(gib_pos())->gib_weg(weg_t::schiene);

    if(weg) {
	// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
	dynamic_cast<schiene_t *>(weg)->setze_ribi_maske(ribi_t::keine);

	blockhandle_t bs = dynamic_cast<schiene_t *>(weg)->gib_blockstrecke();

        blockmanager::gib_manager()->entferne_signal(this, bs);

    } else {
        dbg->error("signal_t::~signal_t()",
	           "Signal %p was deleted but ground was not an railroad track!");
    }

}


/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void signal_t::info(cbuffer_t & buf) const
{
  schiene_t * sch = dynamic_cast<schiene_t *>(welt->lookup(gib_pos())->gib_weg(weg_t::schiene));

  if(sch) {
    blockhandle_t bs = sch->gib_blockstrecke();

    bs->info(buf);

    buf.append("\n");
    buf.append(bs.get_id());
    buf.append("\n");

    buf.append("Direction: ");
    buf.append(dir);
    buf.append("\n");

  } else {
    ding_t::info(buf);
  }
}


void signal_t::calc_bild()
{
  if(blockend) {
    setze_bild(0, IMG_LEER);

    // Hajo: ein leeres Bild refreshed nicht, deshalb müssen wir manuell
    // das Feld neuzeichnen lassen
    welt->markiere_dirty(gib_pos());
  } else {

    switch(dir) {
    case ribi_t::nord:
      setze_xoff(-2);
      setze_yoff(-12);
      setze_bild(0, skinverwaltung_t::signale->gib_bild_nr(1+zustand*4));
      break;

    case ribi_t::sued:
      setze_xoff(2);
      setze_yoff(12);
      setze_bild(0, skinverwaltung_t::signale->gib_bild_nr(0+zustand*4));
      break;

    case ribi_t::ost:
      setze_xoff(24);
      setze_yoff(0);
      setze_bild(0, skinverwaltung_t::signale->gib_bild_nr(2+zustand*4));
      break;

    case ribi_t::west:
      setze_xoff(-24);
      setze_yoff(0);
      setze_bild(0, skinverwaltung_t::signale->gib_bild_nr(3+zustand*4));
      break;

    default:
      setze_xoff(0);
      setze_yoff(0);
      setze_bild(0, skinverwaltung_t::signale->gib_bild_nr(0));
    }
  }
}

void
signal_t::rdwr(loadsave_t *file)
{
    if(file->is_loading()) {
        ding_t::rdwr(file);

	// Hajo: should never be called ?
	abort();
	/*
        file->rdwr_bool(blockend, " ");
        file->rdwr_enum(zustand, " ");
        file->rdwr_int(nach_rechts, "\n");
	*/

    } else {
        // signale werden von der blockstrecke gespeichert und geladen
        file->wr_obj_id(-1);
    }
}

void
signal_t::rdwr(loadsave_t *file, bool force)
{
    assert(force == true);
    ding_t::rdwr(file);

    file->rdwr_char(blockend, " ");
    file->rdwr_char(zustand, " ");
    file->rdwr_char(dir, "\n");
}


/**
 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void signal_t::laden_abschliessen()
{
  calc_bild();
}

void presignal_t::calc_bild()
{
  if(blockend) {
    setze_bild(0, IMG_LEER);

    // Hajo: ein leeres Bild refreshed nicht, deshalb müssen wir manuell
    // das Feld neuzeichnen lassen
    welt->markiere_dirty(gib_pos());
  } else {

    switch(dir) {
    case ribi_t::nord:
      setze_xoff(-2);
      setze_yoff(-12);
      setze_bild(0, skinverwaltung_t::presignals->gib_bild_nr(1+zustand*4));
      break;

    case ribi_t::sued:
      setze_xoff(2);
      setze_yoff(12);
      setze_bild(0, skinverwaltung_t::presignals->gib_bild_nr(0+zustand*4));
      break;

    case ribi_t::ost:
      setze_xoff(24);
      setze_yoff(0);
      setze_bild(0, skinverwaltung_t::presignals->gib_bild_nr(2+zustand*4));
      break;

    case ribi_t::west:
      setze_xoff(-24);
      setze_yoff(0);
      setze_bild(0, skinverwaltung_t::presignals->gib_bild_nr(3+zustand*4));
      break;

    default:
      setze_xoff(0);
      setze_yoff(0);
      setze_bild(0, skinverwaltung_t::presignals->gib_bild_nr(0));
    }
  }
}
