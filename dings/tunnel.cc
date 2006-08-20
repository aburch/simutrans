/*
 * tunnel.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simplay.h"
#include "../simwerkz.h"
#include "../blockmanager.h"
#include "../boden/grund.h"
#include "../boden/wege/strasse.h"
#include "../boden/wege/schiene.h"
#include "../simimg.h"
#include "../simcosts.h"
#include "../utils/cbuffer_t.h"
#include "../bauer/tunnelbauer.h"

#include "../dataobj/loadsave.h"

#include "../besch/tunnel_besch.h"

#include "tunnel.h"




tunnel_t::tunnel_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
  besch = 0;
  rdwr(file);

  step_frequency = 0;
}


tunnel_t::tunnel_t(karte_t *welt, koord3d pos,
		   spieler_t *sp, const tunnel_besch_t *besch) :
    ding_t(welt, pos)
{
  this->besch = besch;

  const grund_t *gr = welt->lookup(gib_pos());

  if(besch) {
    setze_bild(0, besch->gib_hintergrund_nr(gr->gib_grund_hang()));
  }
  setze_besitzer( sp );

  step_frequency = 0;
}


/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void tunnel_t::info(cbuffer_t & buf) const
{
  ding_t::info(buf);

  schiene_t *sch = dynamic_cast<schiene_t *>(welt->lookup(gib_pos())->gib_weg(weg_t::schiene));

  if(sch) {

    buf.append("Rail block ");
    buf.append(sch->gib_blockstrecke().get_id());
    buf.append("\n");

    buf.append("Ribi ");
    buf.append(sch->gib_ribi());
    buf.append("\n");
  }
}


int tunnel_t::gib_after_bild() const
{
  const grund_t *gr = welt->lookup(gib_pos());
  return besch->gib_vordergrund_nr(gr->gib_grund_hang());
}


int tunnel_t::gib_bild() const
{
  const grund_t *gr = welt->lookup(gib_pos());
  return besch->gib_vordergrund_nr(gr->gib_grund_hang());
}


void tunnel_t::rdwr(loadsave_t *file)
{
  ding_t::rdwr(file);
}


void tunnel_t::laden_abschliessen()
{
  const grund_t *gr = welt->lookup(gib_pos());

  if(gr->gib_weg(weg_t::strasse)) {
    besch = tunnelbauer_t::strassentunnel;
  } else {
    besch = tunnelbauer_t::schienentunnel;
  }
}
