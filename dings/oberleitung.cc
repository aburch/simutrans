/*
 * oberleitung.cc
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include "oberleitung.h"

#include "../simworld.h"
#include "../simskin.h"
#include "../simplay.h"
#include "../besch/skin_besch.h"
#include "../boden/grund.h"
#include "../boden/wege/weg.h"
#include "../boden/wege/schiene.h"
#include "../simdebug.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/umgebung.h"

oberleitung_t::oberleitung_t(karte_t *welt, koord3d pos, spieler_t *besitzer) : ding_t(welt, pos)
{
  setze_besitzer(besitzer);
//  step_frequency = 127;
  step_frequency = 0;	// do not step ...

  if(gib_besitzer()) {
    gib_besitzer()->add_maintenance(umgebung_t::maint_overhead);
  }
}


oberleitung_t::oberleitung_t(karte_t *welt, loadsave_t *file) : ding_t (welt)
{
  rdwr(file);
//  step_frequency = 127;
  step_frequency = 0;	// do not step ...

  if(gib_besitzer()) {
    gib_besitzer()->add_maintenance(umgebung_t::maint_overhead);
  }
}


oberleitung_t::~oberleitung_t()
{
	if(gib_besitzer()) {
	gib_besitzer()->add_maintenance(-umgebung_t::maint_overhead);
	}

	// remove status electrified from rail
	koord3d	pos = gib_pos();
	grund_t *gr=welt->lookup(pos);
	if(gr) {
		schiene_t *sch = dynamic_cast<schiene_t *> (gr->gib_weg(weg_t::schiene));
		if(sch) {
			sch->setze_elektrisch( false );
		}
	}
}


/**
 * Falls etwas nach den Vehikeln gemalt werden muß.
 * @author V. Meyer
 */
int oberleitung_t::gib_after_bild() const
{
  return front;
}


/**
 * Oberleitung öffnet kein Info-Fenster
 * @author Hj. Malthaner
 */
void oberleitung_t::zeige_info()
{
  // do nothing
}


void oberleitung_t::calc_bild()
{
  int back;


  // set default values, juts in case the following code misses some if or
  // switch case
  back = skinverwaltung_t::oberleitung->gib_bild_nr(0);
  front = skinverwaltung_t::oberleitung->gib_bild_nr(8);


  grund_t *gr = welt->lookup(gib_pos());


  hang_t::typ hang = gr->gib_weg_hang();
  const ribi_t::ribi ribi = gr->gib_weg_ribi_unmasked(weg_t::schiene);


  if(hang != gr->gib_grund_hang() &&
     gr->gib_grund_hang() != 0) {
    // Hajo: rampe oder so
    setze_yoff( height_scaling( -16) );
  }

  if(hang != hang_t::flach) {

    switch(hang) {
    case hang_t::sued:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(6);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(16);
      break;
    case hang_t::ost:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(7);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(17);
      break;
    case hang_t::nord:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(6);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(18);
      break;
    case hang_t::west:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(7);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(19);
      break;
    default:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(0);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(8);
    }

  } else {

    switch(ribi) {
    case ribi_t::nord:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(0);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(20);
      break;
    case ribi_t::ost:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(1);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(23);
      break;
    case ribi_t::sued:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(0);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(22);
      break;
    case ribi_t::west:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(1);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(21);
      break;
    case ribi_t::nordsued:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(0);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(8);
      break;
    case ribi_t::ostwest:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(1);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(9);
      break;

    case ribi_t::nordwest:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(-1);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(14);
      break;

    case ribi_t::suedost:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(-1);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(15);
      break;

    case ribi_t::nordost:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(-1);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(12);
      break;

    case ribi_t::suedwest:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(-1);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(13);
      break;

    case ribi_t::nordsuedost:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(3);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(24);
      break;

    case ribi_t::nordostwest:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(3);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(25);
      break;

    case ribi_t::nordsuedwest:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(3);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(26);
      break;

    case ribi_t::suedostwest:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(3);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(27);
      break;

    case ribi_t::alle:
      back = skinverwaltung_t::oberleitung->gib_bild_nr(3);
      front = skinverwaltung_t::oberleitung->gib_bild_nr(11);
      break;
    }
  }

  if(gr->ist_tunnel()) {
    front = -1;
    back = -1;
  }


  setze_bild(0, back);
}


/**
 * Speichert den Zustand des Objekts.
 *
 * @param file Zeigt auf die Datei, in die das Objekt geschrieben werden
 * soll.
 * @author Hj. Malthaner
 */
void oberleitung_t::rdwr(loadsave_t *file)
{
  ding_t::rdwr(file);
}


/**
 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void oberleitung_t::laden_abschliessen()
{
  calc_bild();
}
