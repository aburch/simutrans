/*
 * car_t.cpp
 *
 * Copyright (c) 2003 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include "car_t.h"
#include "car_group_t.h"
#include "../simworld.h"
#include "../simtools.h"
#include "../dataobj/loadsave.h"
#include "../simdebug.h"
#include "../boden/wege/schiene.h"
#include "../simimg.h"


bool car_t::ist_blockwechsel(koord3d pos1, koord3d pos2)
{
  const grund_t *gr1 = welt->lookup(pos1);
  const grund_t *gr2 = welt->lookup(pos2);


  const schiene_t * sch1 =
    dynamic_cast<const schiene_t *>(gr1->gib_weg(weg_t::schiene));
  const schiene_t * sch2 =
    dynamic_cast<const schiene_t *>(gr2->gib_weg(weg_t::schiene));

  return sch1->gib_blockstrecke() != sch2->gib_blockstrecke();
}


weg_t::typ car_t::gib_wegtyp() const {
  return weg_t::schiene;
}


void car_t::calc_bild()
{
  if(welt->lookup(gib_pos())->ist_tunnel()) {
    setze_bild(0, IMG_LEER);
  } else {
    setze_bild(0, kind->gib_bild_nr(ribi_t::gib_dir(gib_fahrtrichtung()),
				    true));
  }
}


void car_t::verlasse_feld()
{
    vehikel_basis_t::verlasse_feld();

    if(ist_blockwechsel(gib_pos(), next_pos)) {
	welt->lookup(gib_pos())->verlasse(this);
    }
}


void car_t::betrete_feld()
{
    vehikel_basis_t::betrete_feld();

    if(prev_pos == koord3d::invalid ||
       ist_blockwechsel(prev_pos, gib_pos())) {
	welt->lookup(gib_pos())->betrete(this);
    }
}


/**
 * Calculates driving direction. Also sets direction vetor.
 * Needs valid prev_pos and next_pos
 * @author Hj. Malthaner
 */
void car_t::calc_fahrtrichtung()
{
  // calcs new direction
  if(prev_pos != next_pos && prev_pos != koord3d::invalid) {
    // go on ...
    fahrtrichtung = calc_richtung(prev_pos.gib_2d(), next_pos.gib_2d(), dx, dy);
  } else {
    // we turned ...
    fahrtrichtung = calc_richtung(gib_pos().gib_2d(), next_pos.gib_2d(), dx, dy);
  }
}


void car_t::hop()
{
  verlasse_feld();

  prev_pos = gib_pos();
  setze_pos(next_pos);

  next_pos = next_next_pos;

  betrete_feld();

  calc_fahrtrichtung();

  calc_bild();
}


/**
 * Sets the driving direction
 * @author Hj. Malthaner
 */
void car_t::set_next_pos(koord3d npos)
{
  next_pos = npos;
}

void car_t::set_prev_pos(koord3d npos)
{
  prev_pos = npos;
}

void car_t::set_next_next_pos(koord3d npos)
{
  next_next_pos = npos;
}


void car_t::set_kind(const vehikel_besch_t *vb)
{
  kind = vb;
}


car_t::car_t(karte_t *welt) : vehikel_basis_t(welt)
{
  setze_besitzer( welt->gib_spieler(0) );
}



car_t::car_t(karte_t *welt, koord3d pos, car_group_t *grp) :
   vehikel_basis_t(welt, pos)
{
  group = grp;

  prev_pos = pos;
  next_pos = pos;
  next_next_pos = pos;

  // Hajo: this always drives north
  dx = 2;
  dy = -1;
  fahrtrichtung = ribi_t::nord;
  hoff = 0;

  setze_besitzer( welt->gib_spieler(0) );
}


/**
 * Swaps state with another car
 * @author Hj. Malthaner
 */
void car_t::swap_state(car_t *other)
{
  int i;
  koord3d tmp;
  ribi_t::ribi f;

  tmp = other->gib_pos();
  other->setze_pos(gib_pos());
  setze_pos(tmp);

  tmp = other->prev_pos;
  other->prev_pos = prev_pos;
  prev_pos = tmp;

  tmp = other->next_pos;
  other->next_pos = next_pos;
  next_pos = tmp;

  tmp = other->next_next_pos;
  other->next_next_pos = next_next_pos;
  next_next_pos = tmp;

  i = other->dx;
  other->dx = dx;
  dx = i;

  i = other->dy;
  other->dy = dy;
  dy = i;

  i = other->hoff;
  other->hoff = hoff;
  hoff = i;


  i = other->gib_xoff();
  other->setze_xoff(gib_xoff());
  setze_xoff(i);

  i = other->gib_yoff();
  other->setze_yoff(gib_yoff());
  setze_yoff(i);


  f = other->fahrtrichtung;
  other->fahrtrichtung = fahrtrichtung;
  fahrtrichtung = f;

}


/**
 * Turn car by 180°
 * @author Hj. Malthaner
 */
void car_t::turn()
{
  koord3d tmp;

  tmp = prev_pos;
  prev_pos = next_pos;
  next_pos = tmp;

  dx = -dx;
  dy = -dy;

  fahrtrichtung = ribi_t::rueckwaerts(fahrtrichtung);

  calc_bild();
}


/**
 * Sets the car at its current position (gib_pos()) onto the map
 * @author Hj. Malthaner
 */
void car_t::add_to_map()
{
  betrete_feld();

  // set rail block waggon counter
  welt->lookup(gib_pos())->betrete(this);
}



const char * car_t::gib_name() const
{
  return "car_t";
}


enum ding_t::typ car_t::gib_typ() const
{
  return ding_t::car;
}


/**
 * Move one step
 * @author Hj. Malthaner
 */
void car_t::step()
{
  // dbg->message("verkehrsteilnehmer_t::sync_step()", "%p called", this);


  // Funktionsaufruf vermeiden, wenn möglich
  // if ist schneller als Aufruf!
  if(hoff) {
    setze_yoff( gib_yoff() - hoff );
  }


  fahre();


  hoff = calc_height();

  // Funktionsaufruf vermeiden, wenn möglich
  // if ist schneller als Aufruf!
  if(hoff) {
    setze_yoff( gib_yoff() + hoff );
  }
}


/**
 * Öffnet ein neues Beobachtungsfenster für das Objekt.
 * @author Hj. Malthaner
 */
void car_t::zeige_info()
{
  ding_t::zeige_info();
}


/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 * @see simwin
 */
char * car_t::info(char *buf) const
{
  buf = group->info(buf);
  return buf;
}


void car_t::rdwr(loadsave_t *file)
{
  vehikel_basis_t::rdwr(file);

  file->rdwr_int(dx, " ");
  file->rdwr_int(dy, "\n");
  file->rdwr_enum(fahrtrichtung, " ");
  file->rdwr_int(hoff, "\n");

  prev_pos.rdwr(file);
  next_pos.rdwr(file);
}


/**
 * Debug info
 */
void car_t::dump() const
{
  dbg->message("car_t::dump()", "x=%d y=%d dx=%d, dy=%d, dir=%d\n",
	       gib_pos().x, gib_pos().y, dx, dy, fahrtrichtung);
}
