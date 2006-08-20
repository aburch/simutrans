/*
 * leitung.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "leitung2.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../simplay.h"
#include "../simimg.h"
#include "../simfab.h"
#include "../simskin.h"

#include "../simgraph.h"

#include "../utils/cbuffer_t.h"

#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/powernet.h"
#include "../boden/grund.h"
#include "../boden/wege/weg.h"
#include "../besch/weg_besch.h"
#include "../bauer/wegbauer.h"


#define PROD 1000


static const char * measures[] =
{
    "fail",
    "weak",
    "good",
    "strong",
};


static bool ist_leitung(karte_t *welt, koord pos)
{
  bool result = false;
  const planquadrat_t * plan = welt->lookup(pos);

  if(plan) {
    grund_t * gr = plan->gib_kartenboden();

    result = (gr && gr->suche_obj(ding_t::leitung) != 0);
  }

  return result;
}


fabrik_t *
leitung_t::suche_fab_4(const koord pos)
{
    for(int k=0; k<4; k++) {

	const grund_t *gr = welt->lookup(pos+koord::nsow[k])->gib_kartenboden();

	const int n = gr->gib_top();

	for(int i=0; i<n; i++) {
	    const ding_t * dt = gr->obj_bei(i);
	    if(dt != NULL && dt->fabrik() != NULL) {
		return dt->fabrik();
	    }
	}
    }
    return NULL;
}


void leitung_t::sync_prepare()
{
  // unused
};


bool leitung_t::sync_step(long /*delta_t*/)
{
  return true;
};


leitung_t::leitung_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
  net = 0;

  rdwr(file);
  verbinde();
}


leitung_t::leitung_t(karte_t *welt,
		     koord3d pos,
		     spieler_t *sp) : ding_t(welt, pos)
{
  net = 0;

  setze_besitzer( sp );
  verbinde();
}


leitung_t::~leitung_t()
{
    entferne(gib_besitzer());
}

static int gimme_neighbours(karte_t *welt, koord base_pos, leitung_t **conn)
{
  int count = 0;

  for(int i=0; i<4; i++) {
    const koord pos = base_pos + koord::nsow[i];
    const planquadrat_t * plan = welt->lookup(pos);
    grund_t * gr = plan ? plan->gib_kartenboden() : 0;

    if(gr) {
      leitung_t * line = dynamic_cast<leitung_t *> (gr->suche_obj(ding_t::leitung));
//printf("gimme_neighbours() leitung %p on pos (%i,%i)\n",line,pos.x,pos.y);
      if(line == 0) {
	line = dynamic_cast<leitung_t *> (gr->suche_obj(ding_t::pumpe));
//printf("gimme_neighbours() pumpe %p on pos (%i,%i)\n",line,pos.x,pos.y);
      }

      if(line == 0) {
	line = dynamic_cast<leitung_t *> (gr->suche_obj(ding_t::senke));
//printf("gimme_neighbours() senke %p on pos (%i,%i)\n",line,pos.x,pos.y);
      }


      if(line) {
	conn[count ++] = line;
      }
    }
  }

  return count;
}


void leitung_t::replace(koord base_pos, powernet_t *alt, powernet_t *neu)
{
  leitung_t * conn[4];
  const int count = gimme_neighbours(welt, base_pos, conn);

  for(int i=0; i<count; i++) {
    if(conn[i]->net == alt && alt != neu) {
      conn[i]->net = neu;

      replace(conn[i]->gib_pos().gib_2d(), alt, neu);
    }
  }
}


/**
 * Connect this piece of powerline to its neighbours
 * -> this can merge power networks
 * @author Hj. Malthaner
 */
void leitung_t::verbinde()
{
  leitung_t * conn[4];
  const int count = gimme_neighbours(welt, gib_pos().gib_2d(), conn);


  if(count == 0) {
    net = new powernet_t();
    welt->sync_add(net);

  } else if(count == 1) {
    net = conn[0]->net;

  } else {

    net = new powernet_t();
    welt->sync_add(net);

    for(int i=0; i<count; i++) {
      // memory leak!

      replace(conn[i]->gib_pos().gib_2d(), conn[i]->net, net);
    }
  }
}


/**
 * Disconencts this piece of powerline from its neighbours.
 * -> This may case a network split and new networks must be introduced.
 */
void leitung_t::trenne()
{
}


void leitung_t::entferne(const spieler_t *)
{
  grund_t *gr = welt->lookup(gib_pos());

  if(gr) {

    gr->obj_remove(this, gib_besitzer());

    leitung_t * conn[4];
    const int count = gimme_neighbours(welt, gib_pos().gib_2d(), conn);

    for(int i=0; i<count; i++) {
      // possible memory leak!
      powernet_t *net = new powernet_t();
      welt->sync_add(net);

      replace(conn[i]->gib_pos().gib_2d(), conn[i]->net, net);

      conn[i]->calc_neighbourhood();
    }
  }


  this->net = 0;

  setze_pos(koord3d::invalid);
}


void leitung_t::display(int xpos, int ypos, bool dirty) const
{
  ding_t::display(xpos, ypos, dirty);

  // display_fillbox_wh(xpos+20, ypos, 24, 6, ((int) net) & 0x7FFF, dirty);
}


void leitung_t::calc_bild()
{
	const koord pos = gib_pos().gib_2d();
	ribi_t::ribi ribi =
			(ist_leitung(welt, pos + koord(0,-1)) ? ribi_t::nord : ribi_t::keine) |
			(ist_leitung(welt, pos + koord(0, 1)) ? ribi_t::sued : ribi_t::keine) |
			(ist_leitung(welt, pos + koord( 1,0)) ? ribi_t::ost  : ribi_t::keine) |
			(ist_leitung(welt, pos + koord(-1,0)) ? ribi_t::west : ribi_t::keine);
/*
// dbg->message("leitung_t::calc_bild()", "ribi=%d", ribi);
setze_bild(0, wegbauer_t::leitung_besch->gib_bild_nr(ribi));
*/

	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *gr = welt->lookup(pos)->gib_kartenboden();
	hang_t::typ hang = gr->gib_grund_hang();
	if(hang != hang_t::flach) {
		setze_bild(0, wegbauer_t::leitung_besch->gib_hang_bild_nr(hang));
//	 printf("Hangbild %i=>%i\n",hang,wegbauer_t::leitung_besch->gib_hang_bild_nr(hang));
	}
	else {
		if(gr->gib_weg(weg_t::strasse)!=NULL   ||  gr->gib_weg(weg_t::schiene)!=NULL) {
			// crossing with road or rail
			if(ribi_t::ist_gerade_ns(ribi)) {
				setze_bild(0, wegbauer_t::leitung_besch->gib_diagonal_bild_nr(ribi_t::nord|ribi_t::ost));
			}
			else {
				setze_bild(0, wegbauer_t::leitung_besch->gib_diagonal_bild_nr(ribi_t::sued|ribi_t::west));
			}
		}
		else {
			setze_bild(0, wegbauer_t::leitung_besch->gib_bild_nr(ribi));
		}
	}
// printf("Hangbild %i=>%i\n",hang,wegbauer_t::leitung_besch->gib_hang_bild_nr(hang));
}


/**
 * Recalculates the images of all neighbouring
 * powerlines and the powerline itself
 *
 * @author Hj. Malthaner
 */
void leitung_t::calc_neighbourhood()
{
  for(int i=0; i<4; i++) {
    const koord pos = gib_pos().gib_2d() + koord::nsow[i];
    const planquadrat_t * plan = welt->lookup(pos);
    grund_t * gr = plan ? plan->gib_kartenboden() : 0;

    if(gr) {
      leitung_t * line = dynamic_cast<leitung_t *> (gr->suche_obj(ding_t::leitung));
      if(line) {
	line->calc_bild();
      }
    }
  }

  calc_bild();
}


/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void leitung_t::info(cbuffer_t & buf) const
{
  buf.append(translator::translate("Power"));
  buf.append(": ");
  buf.append(net->get_capacity());
  buf.append("\nNet: ");
  buf.append((int)net);
}


/**
 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void leitung_t::laden_abschliessen()
{
  calc_neighbourhood();
}



/**
 * Speichert den Zustand des Objekts.
 *
 * @param file Zeigt auf die Datei, in die das Objekt geschrieben werden
 * soll.
 * @author Hj. Malthaner
 */
void leitung_t::rdwr(loadsave_t *file)
{
  unsigned long value;

  ding_t::rdwr(file);

  if(file->get_version() <= 82001) {
    net = new powernet_t();
  } else {
    if(file->is_saving()) {
      value = (unsigned long) net;
      file->rdwr_long(value, "\n");
    } else {
      file->rdwr_long(value, "\n");
      net = powernet_t::load_net((powernet_t *) value);
    }
  }
}


pumpe_t::pumpe_t(karte_t *welt, loadsave_t *file) : leitung_t(welt , file)
{
  fab = 0;

  calc_bild();

  welt->sync_add(this);
}


pumpe_t::pumpe_t(karte_t *welt, koord3d pos, spieler_t *sp) : leitung_t(welt , pos, sp)
{
  fab = 0;

  calc_bild();

  welt->sync_add(this);
}


pumpe_t::~pumpe_t()
{
  welt->sync_remove(this);
}


/**
 * Dient zur Neuberechnung des Bildes
 * @author Hj. Malthaner
 */
void pumpe_t::calc_bild()
{
  setze_bild(0, skinverwaltung_t::pumpe->gib_bild_nr(0));
}


void
pumpe_t::sync_prepare()
{
  if(fab == NULL) {
    fab = leitung_t::suche_fab_4(gib_pos().gib_2d());
  }
}


bool
pumpe_t::sync_step(long delta_t)
{
  // dbg->message("pumpe_t::sync_step()", "fab=%p", fab);



  if(fab == 0 || fab->gib_eingang()->get_count() == 0 || fab->gib_eingang()->get(0).menge <= 0)
  {
    // nothing to do ?
  } else {
    get_net()->add_power(delta_t * PROD);
  }

  return true;
}


char *
pumpe_t::info(char *buf) const
{
    if(fab == 0 ||
       fab->gib_eingang()->get_count() == 0 ||
       fab->gib_eingang()->get(0).menge <= 0)
    {
	buf += sprintf(buf, "%s: %s\nnet=%p",
                       translator::translate("Power"),
                       translator::translate(measures[0]),
		       get_net());
    } else {
	buf += sprintf(buf, "%s: %s\nnet=%p",
                       translator::translate("Power"),
                       translator::translate(measures[3]),
		       get_net());
    }

    return buf;
}


senke_t::senke_t(karte_t *welt, loadsave_t *file) : leitung_t(welt , file)
{
  fluss_alt = 0;
  fab = NULL;
  einkommen = 0;

  calc_bild();

  welt->sync_add(this);
}


senke_t::senke_t(karte_t *welt, koord3d pos, spieler_t *sp) : leitung_t(welt , pos, sp)
{
  fluss_alt = 0;
  fab = NULL;
  einkommen = 0;

  calc_bild();

  welt->sync_add(this);
}


/**
 * Dient zur Neuberechnung des Bildes
 * @author Hj. Malthaner
 */
void senke_t::calc_bild()
{
  setze_bild(0, skinverwaltung_t::senke->gib_bild_nr(0));
}


senke_t::~senke_t()
{
  welt->sync_remove(this);
}


/**
 * Methode für asynchrone Funktionen eines Objekts.
 * @author Hj. Malthaner
 */
bool senke_t::step(long /*delta_t*/)
{
    gib_besitzer()->buche(einkommen >> 3, gib_pos().gib_2d(), COST_INCOME);

    einkommen = 0;
    return true;
}


void
senke_t::sync_prepare()
{
    if(fab == NULL) {
	fab = leitung_t::suche_fab_4(gib_pos().gib_2d());
    }
}


bool
senke_t::sync_step(long time)
{
  //dbg->message("senke_t::sync_step()", "called");

  einkommen = get_net()->withdraw_power(time*PROD);

  return true;
}


char *
senke_t::info(char *buf) const
{
    buf += sprintf(buf, "%s: %d\n",
                   translator::translate("Power"),
                   get_net()->get_capacity());

    return buf;
}
