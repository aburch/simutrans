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
#include "../simdebug.h"
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

/*
static const char * measures[] =
{
    "fail",
    "weak",
    "good",
    "strong",
};
*/

static bool
ist_leitung(karte_t *welt, koord pos)
{
  bool result = false;
  const planquadrat_t * plan = welt->lookup(pos);

  if(plan) {
    grund_t * gr = plan->gib_kartenboden();

    result = gr && (gr->suche_obj(ding_t::leitung)!=NULL  ||  gr->suche_obj(ding_t::pumpe)!=NULL  ||  gr->suche_obj(ding_t::senke)!=NULL);
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
			const ding_t * dt=gr->obj_bei(i);
			if(dt!=NULL && dt->fabrik()!=NULL) {
				return dt->fabrik();
			}
		}
	}
	return NULL;
}


leitung_t::leitung_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
  step_frequency = 0;
  set_net(NULL);
  rdwr(file);
}


leitung_t::leitung_t(karte_t *welt,
		     koord3d pos,
		     spieler_t *sp) : ding_t(welt, pos)
{
  step_frequency = 0;
  set_net(NULL);
  setze_besitzer( sp );
  verbinde();
}


leitung_t::~leitung_t()
{
    entferne(gib_besitzer());
}



static int
gimme_neighbours(karte_t *welt, koord base_pos, leitung_t **conn)
{
	int count = 0;

	for(int i=0; i<4; i++) {
		const koord pos = base_pos + koord::nsow[i];
		const planquadrat_t * plan = welt->lookup(pos);
		grund_t * gr = plan ? plan->gib_kartenboden() : 0;

		conn[i] = NULL;
		if(gr) {
			leitung_t *p = dynamic_cast<leitung_t *> (gr->suche_obj(ding_t::pumpe));
			if(p) {
				conn[i] = p;
				count ++;
				continue;
			}
			// now handle drain
			leitung_t *s = dynamic_cast<leitung_t *> (gr->suche_obj(ding_t::senke));
			if(s) {
				conn[i] = s;
				count ++;
				continue;
			}
			// and now handle line
			leitung_t *l = dynamic_cast<leitung_t *> (gr->suche_obj(ding_t::leitung));
			if(l) {
				conn[i] = l;
				count ++;
			}
		}
	}
	return count;
}



/* returns the net identifier at this position
 * @author prissi
 */
static bool get_net_at(const planquadrat_t * plan,powernet_t **l_net)
{
	grund_t * gr = plan ? plan->gib_kartenboden() : 0;
	if(gr) {
		// only this way pumps are handled properly
		const pumpe_t *p = dynamic_cast<pumpe_t *> (gr->suche_obj(ding_t::pumpe));
		if(p) {
			*l_net = p->get_net();
			return true;
		}
		// now handle drain
		const senke_t *s = dynamic_cast<senke_t *> (gr->suche_obj(ding_t::senke));
		if(s) {
			*l_net = s->get_net();
			return true;
		}
		// and now handle line
		const leitung_t *l = dynamic_cast<leitung_t *> (gr->suche_obj(ding_t::leitung));
		if(l) {
			*l_net = l->get_net();
//DBG_MESSAGE("get_net_at()","Found net %p",l->get_net());
			return true;
		}
	}
	*l_net = NULL;
	return false;
}



/* sets the net at a position
 * @author prissi
 */
static void set_net_at(const planquadrat_t * plan,powernet_t *new_net)
{
	grund_t * gr = plan ? plan->gib_kartenboden() : 0;
	if(gr) {
		// only this way pumps are handled properly
		pumpe_t *p = dynamic_cast<pumpe_t *> (gr->suche_obj(ding_t::pumpe));
		if(p) {
			p->set_net(new_net);
			return;
		}
		// now handle drain
		senke_t *s = dynamic_cast<senke_t *> (gr->suche_obj(ding_t::senke));
		if(s) {
			s->set_net(new_net);
//DBG_MESSAGE("set_net_at()","Using new net %p",s->get_net());
			return;
		}
		// and now handle line
		leitung_t *l = dynamic_cast<leitung_t *> (gr->suche_obj(ding_t::leitung));
		if(l) {
			l->set_net(new_net);
//DBG_MESSAGE("set_net_at()","Using new net %p",l->get_net());
			return;
		}
	}
}



/* replace networks connection
 * non-trivial to handle transformers correctly
 * @author prissi
 */
void leitung_t::replace(koord base_pos, powernet_t *old_net, powernet_t *new_net)
{
	powernet_t *current;
	if(get_net_at(welt->lookup(base_pos),&current)  &&  current!=new_net) {
		// convert myself ...
//DBG_MESSAGE("leitung_t::replace()","My net %p by %p at (%i,%i)",new_net,current,base_pos.x,base_pos.y);
		set_net_at(welt->lookup(base_pos),new_net);
		//get_net_at(welt->lookup(base_pos),&current);
	}

	for(int i=0; i<4; i++) {
		koord	pos=base_pos+koord::nsow[i];
		const planquadrat_t * plan = welt->lookup(pos);

		if(get_net_at(plan,&current)  &&  current!=new_net) {
			if(current!=old_net) {
				replace(pos,current,new_net);
				if(current!=NULL) {
// memory leak
//					delete current;
				}
			}
			else {
				replace(pos,old_net,new_net);
			}
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
	const koord pos=gib_pos().gib_2d();
	powernet_t *new_net;

	// first get my own ...
	get_net_at(welt->lookup(pos),&new_net);

//DBG_MESSAGE("leitung_t::verbinde()","Searching net at (%i,%i)",pos.x,pos.y);
	for( int i=0;  i<4  &&  new_net==NULL;  i++ ) {
		const planquadrat_t * plan = welt->lookup(pos+koord::nsow[i]);
		get_net_at(plan,&new_net);
	}
//DBG_MESSAGE("leitung_t::verbinde()","Found net %p",new_net);

	// we are alone?
	if(new_net==NULL) {
		// then we start a new net
		new_net = new powernet_t();
		welt->sync_add(new_net);
//DBG_MESSAGE("leitung_t::verbinde()","Creating new net %p",new_net);
	}
	if(new_net!=get_net()) {
		replace(pos, NULL, new_net);
		set_net(new_net);
	}
}


/**
 * Disconencts this piece of powerline from its neighbours.
 * -> This may case a network split and new networks must be introduced.
 */
void leitung_t::trenne()
{
}



const char *
leitung_t::ist_entfernbar(const spieler_t *sp)
{
	if(sp==gib_besitzer()) {
		return NULL;
	}
	else {
		return "Der Besitzer erlaubt das Entfernen nicht";
	}
}



void leitung_t::entferne(const spieler_t *)
{
//DBG_MESSAGE("leitung_t::entferne()","remove pylon at (%i,%i)",gib_pos().x,gib_pos().y);
	grund_t *gr = welt->lookup(gib_pos());
	if(gr) {
		powernet_t *my_net = get_net();
		gr->obj_remove(this, gib_besitzer());

		leitung_t * conn[4];
		gimme_neighbours(welt, gib_pos().gib_2d(), conn);
//		const int count = gimme_neighbours(welt, gib_pos().gib_2d(), conn);

		for(int i=0; i<4; i++) {
			if(conn[i]!=NULL) {
				koord pos = gr->gib_pos().gib_2d()+koord::nsow[i];
				// possible memory leak!
				powernet_t *new_net = new powernet_t();
				welt->sync_add(new_net);
				replace(pos, my_net, new_net);
				conn[i]->calc_neighbourhood();
			}
		}
		// clean up stuff
//		delete this->net;
	}
	this->set_net(NULL);
	setze_pos(koord3d::invalid);
}



void leitung_t::display(int xpos, int ypos, bool dirty) const
{
	ding_t::display(xpos, ypos, dirty);
	// display_fillbox_wh(xpos+20, ypos, 24, 6, ((int) net) & 0x7FFF, dirty);
}


ribi_t::ribi leitung_t::gib_ribi()
{
	const koord pos = gib_pos().gib_2d();
	const ribi_t::ribi ribi =
			(ist_leitung(welt, pos + koord(0,-1)) ? ribi_t::nord : ribi_t::keine) |
			(ist_leitung(welt, pos + koord(0, 1)) ? ribi_t::sued : ribi_t::keine) |
			(ist_leitung(welt, pos + koord( 1,0)) ? ribi_t::ost  : ribi_t::keine) |
			(ist_leitung(welt, pos + koord(-1,0)) ? ribi_t::west : ribi_t::keine);
	return ribi;
}



/* extended by prissi */
void leitung_t::calc_bild()
{
	const koord pos = gib_pos().gib_2d();
	const ribi_t::ribi ribi=gib_ribi();
	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *gr = welt->lookup(pos)->gib_kartenboden();
	hang_t::typ hang = gr->gib_grund_hang();
	if(hang != hang_t::flach) {
		setze_bild(0, wegbauer_t::leitung_besch->gib_hang_bild_nr(hang));
	}
	else {
		if(gr->gib_weg(weg_t::strasse)!=NULL   ||  gr->gib_weg(weg_t::schiene)!=NULL  ||  !gr->ist_natur()) {
			// crossing with road or rail
			if(ribi_t::ist_gerade_ns(ribi)) {
				setze_bild(0, wegbauer_t::leitung_besch->gib_diagonal_bild_nr(ribi_t::nord|ribi_t::ost));
			}
			else {
				setze_bild(0, wegbauer_t::leitung_besch->gib_diagonal_bild_nr(ribi_t::sued|ribi_t::ost));
			}
		}
		else {
			if(ribi_t::ist_gerade(ribi)  &&  !ribi_t::ist_einfach(ribi)  &&  (pos.x+pos.y)&1) {
				// every second skip mast
				if(ribi_t::ist_gerade_ns(ribi)) {
					setze_bild(0, wegbauer_t::leitung_besch->gib_diagonal_bild_nr(ribi_t::nord|ribi_t::west));
				}
				else {
					setze_bild(0, wegbauer_t::leitung_besch->gib_diagonal_bild_nr(ribi_t::sued|ribi_t::west));
				}
			}
			else {
				setze_bild(0, wegbauer_t::leitung_besch->gib_bild_nr(ribi));
			}
		}
	}
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
	buf.append(get_net()->get_capacity());
	buf.append("\nNet: ");
	buf.append((int)get_net());
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
  verbinde();
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
    set_net(NULL);
  } else {
    if(file->is_saving()) {
      value = (unsigned long) get_net();
      file->rdwr_long(value, "\n");
    } else {
      file->rdwr_long(value, "\n");
//      net = powernet_t::load_net((powernet_t *) value);
	set_net(NULL);
    }
  }
}



/* from here on pump (source) stuff */


pumpe_t::pumpe_t(karte_t *welt, loadsave_t *file) : leitung_t(welt , file)
{
	fab = NULL;
	power_there = false;
	calc_bild();
	calc_neighbourhood();
	welt->sync_add(this);
}


pumpe_t::pumpe_t(karte_t *welt, koord3d pos, spieler_t *sp) : leitung_t(welt , pos, sp)
{
	fab = NULL;
	power_there = false;
	calc_bild();
	calc_neighbourhood();
	welt->sync_add(this);
}



pumpe_t::~pumpe_t()
{
	if(fab) {
		fab->set_prodfaktor( MAX(16,fab->get_prodfaktor()/2) );
	}
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


/**
 * recalculates only the image
 */
bool pumpe_t::step(long /*delta_t*/)
{
    setze_bild(0, skinverwaltung_t::pumpe->gib_bild_nr(power_there?1:0) );
    return true;
}


void
pumpe_t::sync_prepare()
{
	if(fab==NULL) {
		fab = leitung_t::suche_fab_4(gib_pos().gib_2d());
		if(fab) {
			fab->set_prodfaktor( fab->get_prodfaktor()*2 );
		}
		step_frequency = 1;
	}
}


bool
pumpe_t::sync_step(long delta_t)
{
	if(!get_net()) {
		return false;
	}
	if(  fab==0 || (fab->gib_eingang()->get_count()>0  &&  fab->gib_eingang()->get(0).menge<=0)  ) {
		power_there = false;
	}
	else {
		// no input needed or has something to consume
		power_there = true;
		get_net()->add_power(delta_t*fab->get_prodbase()*32);
	}
	return true;
}


/* From here on drain stuff */


senke_t::senke_t(karte_t *welt, loadsave_t *file) : leitung_t(welt , file)
{
	fab = NULL;
	einkommen = 1;
	max_einkommen = 1;
	calc_bild();
	calc_neighbourhood();
	welt->sync_add(this);
}


senke_t::senke_t(karte_t *welt, koord3d pos, spieler_t *sp) : leitung_t(welt , pos, sp)
{
	fab = NULL;
	einkommen = 1;
	max_einkommen = 1;
	calc_bild();
	calc_neighbourhood();
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
	if(fab!=NULL) {
		fab->set_prodfaktor( 16 );
	}
	welt->sync_remove(this);
}


/**
 * Methode für asynchrone Funktionen eines Objekts.
 * @author Hj. Malthaner
 */
bool senke_t::step(long /*delta_t*/)
{
    int faktor = (16*einkommen+16)/max_einkommen;
    gib_besitzer()->buche(einkommen >> 11, gib_pos().gib_2d(), COST_INCOME);
    einkommen = 0;
    max_einkommen = 1;
    fab->set_prodfaktor( 16+faktor );
    setze_bild(0, skinverwaltung_t::senke->gib_bild_nr(faktor==16?1:0) );
    return true;
}


void
senke_t::sync_prepare()
{
	if(fab==NULL) {
		fab = leitung_t::suche_fab_4(gib_pos().gib_2d());
		step_frequency = 1;
	}
}


bool
senke_t::sync_step(long time)
{
	if(!get_net()) {
		return false;
	}
//DBG_MESSAGE("senke_t::sync_step()", "called");
	int want_power = time*(PROD/3);
	int get_power = get_net()->withdraw_power(want_power);
	max_einkommen += want_power;
	einkommen += get_power;
	return true;
}


void
senke_t::info(cbuffer_t & buf) const
{
	/* info in a drain */
	buf.append(translator::translate("Power"));
	buf.append(": ");
	buf.append(get_net()->get_capacity());
	buf.append("\n");
	buf.append(translator::translate("Available"));
	buf.append(": ");
	buf.append((200*einkommen+1)/(2*max_einkommen));
	buf.append("\nNet: ");
	buf.append((int)get_net());
}
