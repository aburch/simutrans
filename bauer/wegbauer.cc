/*
 * simbau.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 *
 * Strassen- und Schienenbau
 *
 * Hj. Malthaner
 */

#include "../simdebug.h"
#include "wegbauer.h"

#include "../boden/wege/strasse.h"
#include "../boden/wege/schiene.h"
#include "../boden/grund.h"
#include "../simworld.h"
#include "../simwerkz.h"
#include "../simcosts.h"
#include "../simplay.h"
#include "../simdepot.h"
#include "../blockmanager.h"
#include "../dings/gebaeude.h"

#include "../simintr.h"

#include "hausbauer.h"
#include "tunnelbauer.h"
#include "brueckenbauer.h"

#include "../besch/weg_besch.h"
#include "../besch/kreuzung_besch.h"
#include "../besch/spezial_obj_tpl.h"


#include "../tpl/stringhashtable_tpl.h"

#include "../dings/leitung2.h"


// Hajo: these are needed to build the menu entries
#include "../gui/werkzeug_parameter_waehler.h"
#include "../besch/skin_besch.h"
#include "../dataobj/translator.h"



slist_tpl<const kreuzung_besch_t *> wegbauer_t::kreuzungen;
const weg_besch_t *wegbauer_t::leitung_besch = NULL;

static spezial_obj_tpl<weg_besch_t> spezial_objekte[] = {
    { &wegbauer_t::leitung_besch, "Powerline" },
    { NULL, NULL }
};


static stringhashtable_tpl <const weg_besch_t *> alle_wegtypen;


bool wegbauer_t::alle_wege_geladen()
{
    // Strom muß nicht sein!
    return ::alles_geladen(spezial_objekte + 1);
}


bool wegbauer_t::alle_kreuzungen_geladen()
{
    if(!gib_kreuzung(weg_t::schiene, weg_t::strasse) ||
  !gib_kreuzung(weg_t::strasse, weg_t::schiene))
   {
  ERROR("wegbauer_t::alles_geladen()", "at least one crossing not found");
  return false;
   }
   return true;
}


bool wegbauer_t::register_besch(const weg_besch_t *besch)
{
  dbg->debug("wegbauer_t::register_besch()", besch->gib_name());

  alle_wegtypen.put(besch->gib_name(), besch);

  return ::register_besch(spezial_objekte, besch);
}


bool wegbauer_t::register_besch(const kreuzung_besch_t *besch)
{
    kreuzungen.append(besch);
    return true;
}





/**
 * Tries to look up description for way, described by way type,
 * system type and construction type
 * @author Hj. Malthaner
 */
const weg_besch_t * wegbauer_t::gib_besch(const char * way_name)
{
  return alle_wegtypen.get(way_name);
}


const kreuzung_besch_t *wegbauer_t::gib_kreuzung(weg_t::typ ns, weg_t::typ ow)
{
    slist_iterator_tpl<const kreuzung_besch_t *> iter(kreuzungen);

    while(iter.next()) {
  if(iter.get_current()->gib_wegtyp_ns() == ns &&
      iter.get_current()->gib_wegtyp_ow() == ow) {
      return iter.get_current();
  }
    }
    return NULL;
}


/**
 * Fill menu with icons of given waytype
 * @author Hj. Malthaner
 */
void wegbauer_t::fill_menu(werkzeug_parameter_waehler_t *wzw,
         weg_t::typ wtyp,
         int (* werkzeug)(spieler_t *, karte_t *, koord, value_t),
         int sound_ok,
         int sound_ko)
{
  stringhashtable_iterator_tpl<const weg_besch_t *> iter(alle_wegtypen);

  while(iter.next()) {
    const weg_besch_t * besch = iter.get_current_value();

    if(besch->gib_wtyp() == wtyp &&
       besch->gib_cursor()->gib_bild_nr(1) != -1) {

      char buf[128];

      sprintf(buf, "%s, %2.2f$ (%2.2f$), %dkm/h",
        translator::translate(besch->gib_name()),
        besch->gib_preis()/100.0,
        besch->gib_wartung()/100.0,
        besch->gib_topspeed());


      wzw->add_param_tool(werkzeug,
        (const void *)besch,
        karte_t::Z_PLAN,
        sound_ok,
        sound_ko,
        besch->gib_cursor()->gib_bild_nr(1),
        besch->gib_cursor()->gib_bild_nr(0),
        buf );
    }
  }
}


/**
 * Finds a way with a given speed limit for a given waytype
 * @author prissi
 */
const weg_besch_t *  wegbauer_t::weg_search(weg_t::typ wtyp,int speed_limit)
{
  stringhashtable_iterator_tpl<const weg_besch_t *> iter(alle_wegtypen);

  const weg_besch_t * besch = NULL;
dbg->message("wegbauer_t::weg_search","Search cheapest for limit %i",speed_limit);
  while(iter.next()) {

    if(iter.get_current_value()->gib_wtyp() == wtyp &&
       iter.get_current_value()->gib_cursor()->gib_bild_nr(1) != -1) {

        if(  besch==NULL  ||  (besch->gib_topspeed()<speed_limit  &&  besch->gib_topspeed()<iter.get_current_value()->gib_topspeed()) ||  (iter.get_current_value()->gib_topspeed()>=speed_limit  &&  iter.get_current_value()->gib_wartung()<besch->gib_wartung())  ) {
          besch = iter.get_current_value();
dbg->message("wegbauer_t::weg_search","Found weg %s, limit %i",besch->gib_name(),besch->gib_topspeed());
      }
    }
  }
  return besch;
}


bool
wegbauer_t::kann_mit_strasse_erreichen(const grund_t *bd, const koord zv)
{
    switch(bd->gib_grund_hang()) {
    case hang_t::flach:
    case hang_t::erhoben:
        return true;
    case hang_t::nord:
    case hang_t::sued:
  return zv.x == 0;
    case hang_t::ost:
    case hang_t::west:
  return zv.y == 0;
    default:
        return false;
    }
}

bool wegbauer_t::kann_ribis_setzen(const grund_t *bd, const koord zv)
{
  /*
    if( bd->hat_gebaeude(hausbauer_t::bushalt_besch) ||
  bd->hat_gebaeude(hausbauer_t::bahnhof_besch))
  */
    if( bd->gib_halt().is_bound() )
    {
  weg_t::typ wtyp = bautyp == strasse ? weg_t::strasse : weg_t::schiene;

  return ribi_t::ist_gerade(bd->gib_weg_ribi_unmasked(wtyp) | ribi_typ(zv));
    } else {
  return true;
    }
}


bool
wegbauer_t::ist_bruecke_tunnel_ok(grund_t *bd_von, koord zv, grund_t *bd_nach)
{
    hang_t::typ hang;
    hang_t::typ hang_zv = hang_typ(zv); // zv=sued -> hang:nord

    if(bd_von->gib_typ() == grund_t::brueckenboden) {
  hang = bd_von->gib_grund_hang();
        if(hang != hang_t::flach) {
      // nur Brückenstart auf Nordhang nach sueden erlauben
      if(hang_zv != hang) {
    return false;
      }
  }
  else {
      // nur Brückenrampe mit Südhang nach sueden erlauben
      hang = bd_von->gib_weg_hang();
      if(hang_zv != hang_t::gegenueber(hang)) {
    return false;
      }
  }
    }
    else if(bd_von->gib_typ() == grund_t::tunnelboden) {
  // nur Tunnelausgänge mit Südhang nach sueden erlauben
  hang = bd_von->gib_grund_hang();
  if(hang_zv != hang_t::gegenueber(hang)) {
      return false;
  }
    }
    if(bd_nach->gib_typ() == grund_t::brueckenboden) {
  hang = bd_nach->gib_grund_hang();
        if(hang != hang_t::flach) {
      // nur Brückenstart auf Nordhang von norden erlauben
      if(hang_zv != hang_t::gegenueber(hang)) {
    return false;
      }
  }
  else {
      // nur Brückenrampe mit Nordhang von norden erlauben
      hang = bd_nach->gib_weg_hang();
      if(hang_zv != hang) {
    return false;
      }
  }
    }
    else if(bd_nach->gib_typ() == grund_t::tunnelboden) {
  // nur Tunnelausgänge mit Nordhang von norden erlauben
  hang = bd_nach->gib_grund_hang();
  if(hang_zv != hang) {
      return false;
  }
    }
    return true;
}

koord3d
wegbauer_t::bruecke_finde_ende(karte_t *welt, koord3d pos, koord zv)
{
    do {
  pos = pos + zv;
  if(!welt->ist_in_kartengrenzen(pos.gib_2d())) {
      return koord3d::invalid;
  }
    } while(!welt->lookup(pos + koord3d(0, 0, 16)) );   // keine Brücke im Weg

    // Hajo: we went one step too far (until it's plain again)
    return pos-zv;
}


koord3d
wegbauer_t::tunnel_finde_ende(karte_t *welt, koord3d pos, const koord zv)
{
    do {
  pos = pos + zv;
  if(!welt->ist_in_kartengrenzen(pos.gib_2d()))
      return koord3d::invalid;
    } while(!welt->lookup(pos));

    return pos;
}


/* allow for railroad crossing
 * @author prissi
 */
bool
wegbauer_t::check_crossing(const koord zv, const grund_t *bd,bool ignore_player,weg_t::typ wtyp) const
{
  if(bd->gib_weg(wtyp)  &&  (ignore_player  || bd->gib_besitzer()==NULL  ||  bd->gib_besitzer()==sp)  &&  bd->gib_halt()==NULL) {
	ribi_t::ribi w_ribi = bd->gib_weg_ribi_unmasked(wtyp);
    // it is our way we want to cross: can we built a crossing here?
    // both ways must be straight and no ends
    return
      ribi_t::ist_gerade(w_ribi)
      &&  !ribi_t::ist_einfach(w_ribi)
      &&  ribi_t::ist_gerade(ribi_typ(zv))
      &&  (w_ribi&ribi_typ(zv))==0;
  }
  // nothing to cross here
  return false;
}


/* crossing of powerlines, or no powerline
 * @author prissi
 */
bool
wegbauer_t::check_for_leitung(const koord zv, const grund_t *bd) const
{
	leitung_t *lt=dynamic_cast<leitung_t *> (bd->suche_obj(ding_t::leitung));
	if(lt!=NULL) {//  &&  (bd->gib_besitzer()==0  ||  bd->gib_besitzer()==sp)) {
		ribi_t::ribi lt_ribi = lt->gib_ribi();
	    // it is our way we want to cross: can we built a crossing here?
	    // both ways must be straight and no ends
	    return
	      ribi_t::ist_gerade(lt_ribi)
	      &&  !ribi_t::ist_einfach(lt_ribi)
	      &&  ribi_t::ist_gerade(ribi_typ(zv))
	      &&  (lt_ribi&ribi_typ(zv))==0;
	}
	// check for transformer
	if(bd->suche_obj(ding_t::pumpe)!=NULL  ||  bd->suche_obj(ding_t::senke)!=NULL) {
		return false;
	}
	// ok, there is not high power transmission stuff going on here
	return true;
}




/* heaviely extended to allow for crossing between rail and road
 * @author prissi
 */
bool
wegbauer_t::ist_grund_fuer_strasse(koord pos, const koord zv, koord start, koord ziel) const
{
    bool ok = false;

    if(welt->ist_in_kartengrenzen(pos.x, pos.y)) {
  const grund_t *bd = welt->lookup(pos)->gib_kartenboden();


  if( welt->ist_markiert(bd->gib_pos()))  // als ungeeignet markiert
      return false;

  ok = bd->ist_natur() &&  !bd->ist_wasser();

  switch(bautyp) {
  case strasse:
      ok =
    (ok || bd->gib_weg(weg_t::strasse)  || check_crossing(zv,bd,sp,weg_t::schiene)) &&
    (bd->gib_weg(weg_t::strasse)  ||  bd->gib_besitzer() == NULL || bd->gib_besitzer() == sp) &&
    !bd->hat_gebaeude(hausbauer_t::frachthof_besch) &&
    check_for_leitung(zv,bd)  &&
    !bd->gib_depot();
      break;
  case schiene:
      ok = (ok || bd->gib_weg(weg_t::schiene)  || check_crossing(zv,bd,false,weg_t::strasse)) &&
    (bd->gib_besitzer() == NULL || bd->gib_besitzer() == sp) &&
    check_for_leitung(zv,bd)  &&
    !bd->gib_depot();
      break;
  case leitung:
// built too many      ok = ok ||  (bd->gib_grund_hang()==0  &&  (bd->gib_besitzer() == NULL || bd->gib_besitzer() == sp) && ribi_t::ist_gerade(ribi_typ(zv)));
      ok = ( bd->ist_natur() ||  bd->ist_wasser())  ||  (
//					     (bd->gib_besitzer() == NULL || bd->gib_besitzer() == sp) &&
     						(
      						(bd->gib_weg(weg_t::strasse)!=NULL  &&  check_crossing(zv,bd,true,weg_t::strasse))  ||
      				 		(bd->gib_weg(weg_t::schiene)!=NULL  &&  check_crossing(zv,bd,true,weg_t::schiene))
      				 	)
      				 );
      break;
      // like strasse but allow for railroad crossing
  case strasse_bot:
      ok = (ok || bd->gib_weg(weg_t::strasse)  || check_crossing(zv,bd,false,weg_t::schiene)) &&
    (bd->gib_besitzer() == NULL || bd->gib_besitzer() == sp) &&
    check_for_leitung(zv,bd)  &&
    !bd->hat_gebaeude(hausbauer_t::frachthof_besch);
      break;
  case schiene_bot:
      ok = ok || bd->gib_weg(weg_t::strasse) != NULL || bd->gib_weg(weg_t::schiene) != NULL;
      break;
     // like schiene, but allow for railroad crossings
     // avoid crossings with any (including our) railroad tracks
  case schiene_bot_bau:
      ok = ok || (pos==start || pos==ziel);
      ok = ok &&  (bd->gib_weg(weg_t::strasse)==NULL  ||  check_crossing(zv,bd,false,weg_t::strasse));
      ok = ok && bd->gib_weg(weg_t::schiene)==NULL  &&  (bd->gib_besitzer()==sp  ||  bd->gib_besitzer()==NULL)  &&  check_for_leitung(zv,bd);
      break;
  default:
      break;
  }
    }
    return ok;
}


wegbauer_t::wegbauer_t(karte_t *wl, spieler_t *spl)
{
    n      = 0;
    max_n  = -1;
    baubaer= false;
    sp     = spl;
    welt   = wl;
    bautyp = strasse;   // kann mit route_fuer() gesetzt werden
    maximum = 500;

    keep_existing_ways = false;
    keep_existing_faster_ways = false;

    const int groesse = welt->gib_groesse();

    info = new array2d_tpl<info_t> (groesse, groesse);

    INT_CHECK("simbau 142");

    route = new array_tpl<koord> (max_route_laenge);
}


wegbauer_t::~wegbauer_t()
{
    delete route;
    delete info;
}



/**
 * If a way is built on top of another way, should the type
 * of the former way be kept or replced (true == keep)
 * @author Hj. Malthaner
 */
void wegbauer_t::set_keep_existing_ways(bool yesno)
{
  keep_existing_ways = yesno;
  keep_existing_faster_ways = false;
}


void wegbauer_t::set_keep_existing_faster_ways(bool yesno)
{
  keep_existing_ways = false;
  keep_existing_faster_ways = yesno;
}



void
wegbauer_t::route_fuer(enum bautyp wt, const weg_besch_t *b)
{
  bautyp = wt;
  besch = b;

  dbg->message("wegbauer_t::route_fuer()",
         "setting way type to %d, besch=%s",
         bautyp, besch ? besch->gib_name() : "NULL");
}


void
wegbauer_t::set_maximum(int mx)
{
    maximum = mx;
}


koord
wegbauer_t::forces_lookup(koord k)
{
    slist_iterator_tpl<dir_force_t *> iter (dir_forces);

//    printf("forces: lookup bei %d %d \n", k.x, k.y);


    while(iter.next()) {
  dir_force_t *df = iter.get_current();

//  printf("forces: vergleich mit %d %d \n", df->k.x, df->k.y);

  if(df->k == k) {
      koord fd = df->force_dir;
//      printf("forces: found dir force %d %d bei %d %d\n", fd.x, fd.y, k.x, k.y);

      return fd;
  }
    }
    return koord(0,0); // alle richtungen erlaubt
}

void
wegbauer_t::forces_update(koord k, koord force_dir)
{
    dir_force_t *df = NULL;

    slist_iterator_tpl<dir_force_t *> iter (dir_forces);

    while(iter.next()) {
  df = iter.get_current();
  if(df->k == k) {
      break;
  }
    }

    // check if found
    if(df != NULL && df->k != k) {
  // not found
  df = NULL;
    }

    if(df == NULL) {
  // nicht vorhanden
  if(force_dir != koord(0, 0)) {

//      printf("forces: new dir force %d %d bei %d %d\n", force_dir->x, force_dir->y, k.x, k.y);

      dir_force_t *ndf = new dir_force_t;
      ndf->k = k;
      ndf->force_dir = force_dir;
      dir_forces.insert( ndf );
  }
    } else {
  // update
  if(force_dir != koord(0, 0)) {
      df->force_dir = force_dir;
//      printf("forces: new dir force %d %d bei %d %d\n", force_dir->x, force_dir->y, k.x, k.y);
  } else {
//      printf("forces: deleting dir force bei %d %d\n", k.x, k.y);
      // remove
      dir_forces.remove(df);
      delete(df);
  }
    }
}



int
wegbauer_t::calc_cost(koord pos)
{
    grund_t *bd = welt->lookup(pos)->gib_kartenboden();
    int     cost = 10;

    if(bautyp == strasse) {
  if(bd->gib_weg(weg_t::strasse)) {
      cost -= 4;
  }
    }

    if(bautyp == schiene_bot || bautyp == schiene_bot_bau) {
  // versuche fuer allgemeine routen eigene wege zu finden

  if(  bd->ist_natur()  ||  (bd->gib_weg(weg_t::strasse)!=NULL  &&  (bd->gib_besitzer()==NULL  ||  bd->gib_besitzer()==sp)) ) {
    // natur or a our street crossing with a rail (direction will be enforce by check_crossing)
    // so we do not have to add any cost
  }
  else {
    // obstacle, which is not our street (which we may cross)
      cost += 50;
//dbg->message("wegbauer_t::calc_cost","obstatcle at (%i,%i)",pos.x,pos.y);
  }

    }

    if(bautyp == strasse_bot) {
  // versuche fuer allgemeine routen eigene wege zu finden

  if(!bd->ist_natur()) {
      if(bd->gib_weg(weg_t::strasse)) {
    cost -= 5;
      } else {
    cost += 50;
      }
  }
    }

    if(bd->gib_weg_hang() != hang_t::flach) {
  cost += 10;
    }
    return cost;
}


koord
wegbauer_t::remove()
{
    slist_iterator_tpl<info_t *> iter(queue);

    info_t * best = NULL;
    int maxi = -unseen;

    while(iter.next()) {
  info_t *inf = iter.get_current();

  if(inf->val > maxi) {
      maxi = inf->val;
      best = inf;
  }
    }

//    printf("SB: best ist %p bei (%d,%d) mit %d\n", best, best->k.x, best->k.y, best->val);

    koord k = best->k;

    queue.remove( best );

    delete best;

    return k;
}

bool
wegbauer_t::update(koord k, int val)
{
//    printf("SB: update (%d,%d) %d ", k.x, k.y, val);

    if(val < -600) {
  return false;
    }

    slist_iterator_tpl<info_t *> iter(queue);

    while(iter.next()) {
  info_t *inf = iter.get_current();

  if(inf->k == k) {
      if(inf->val <= val) {
//    printf("-> changed from %d to %d\n", inf->val, val);
    inf->val = val;
    return true;
      } else {
//    printf("-> done\n");
    return false;
      }
  }

    }

    info_t *inf = new info_t();
    inf->val = val;
    inf->k = k;

//    printf("-> insert\n");
    queue.insert( inf );

    return true;

}

bool
wegbauer_t::check_step(const koord k, const koord t,
                            const koord start, const koord /*ziel*/,
                            int cost,
          koord force_dir)
{
//    printf("check_step %d,%d cost %d (%d)\n", t.x, t.y, cost, info->at(t).val);

    bool found = false;

    const int val = info->at(k).val;
    const int priority = val == unseen ? -cost : info->at(k).val-cost;

    if(info->at(t).val == unseen || info->at(t).val < priority) {

  if(update(t, priority)) {
      info->at(t).val = priority;
      info->at(t).k = k;
      if(t == start) {
    found = true;
      }

      forces_update(t, force_dir);
  }
    }
    return found;
}

koord
wegbauer_t::calc_tunnel_ziel(const koord pos1, const koord pos2)
{
    if(pos1.x < 0 || pos1.y < 0) {
  return koord::invalid;
    }

    const koord d = pos2 - pos1;

    if(d.x > 1 || d.x < -1 || d.y > 1 || d.y < -1) {
  return koord::invalid;
    }

    grund_t *bd = welt->lookup(pos2)->gib_kartenboden();

    const hang_t::typ hang = bd->gib_weg_hang();

    if(!hang_t::ist_einfach(hang)) {
  // kann keinen Tunnel bauen;
  return koord::invalid;
    }
    koord zv(hang);

    // printf("calc_tunnel_ziel (%d, %d), (%d, %d) -> (%d, %d)\n", pos1.x, pos1.y, pos2.x, pos2.y, zv.x, zv.y);

    if(pos2+zv == pos1) {
  // kann keinen Tunnel rueckwaerts bauen;
  return koord(-2,-2);
    }

    koord3d ziel = tunnel_finde_ende(welt, welt->access(pos2)->gib_kartenboden()->gib_pos(), zv);

    hang_t::typ hang2 = welt->get_slope(ziel.gib_2d());

    if(!hang_t::ist_gegenueber(hang, hang2)) {
  return koord::invalid;
    }

    // printf("calc_tunnel_ziel: ziel (%d, %d)", ziel.gib_2d().x, ziel.gib_2d().y);

    return ziel.gib_2d();
}


koord
wegbauer_t::calc_bruecke_ziel(const koord pos1, const koord pos2)
{
    if(pos1.x < 0 || pos1.y < 0) {
  return koord::invalid;;
    }

    const koord d = pos2 - pos1;

    if(d.x > 1 || d.x < -1 || d.y > 1 || d.y < -1) {
  return koord::invalid;;
    }

    grund_t *bd = welt->lookup(pos2)->gib_kartenboden();

  if(  bd==NULL  ||   !(bd->gib_besitzer()==NULL  ||  bd->gib_besitzer()==sp)  ) {
      // can only end on unowned/our land
      return koord::invalid;
  }
  if(  !bd->ist_natur()  ) {
    // can only end on nature
    return koord::invalid;
  }

    const hang_t::typ hang = bd->gib_weg_hang();

    if(!hang_t::ist_einfach(hang)) {
  // kann keine Bruecke bauen;
  return koord::invalid;
    }
    koord zv (hang);//hang_t::gegenueber(hang));

    zv = -zv;

    // printf("Brueckenprufung start=%d,%d  end=%d,%d  zv=%d,%d\n", pos1.x, pos1.y, pos2.x, pos2.y, zv.x, zv.y);

    if(pos2+zv == pos1) {
  // kann keine Bruecke rueckwaerts bauen;
  return koord(-2,-2);
    }
    koord3d ziel = bruecke_finde_ende(welt, welt->access(pos2)->gib_kartenboden()->gib_pos(), zv);

    if(!welt->ist_in_kartengrenzen(ziel.x, ziel.y)) {
        // printf("Brueckenprufung ziel ungültig\n");

  return koord::invalid;
    }

    hang_t::typ hang2 = welt->get_slope(ziel.gib_2d());

    if(!hang_t::ist_gegenueber(hang, hang2)) {
        // printf("Brueckenprufung zielhang an %d, %d ungeeignet\n", ziel.gib_2d().x, ziel.gib_2d().y);
  return koord::invalid;
    }
    return ziel.gib_2d();
}


bool
wegbauer_t::check_tunnelbau(const koord k, const koord t,
                                 const koord start, const koord ziel)
{
    // berechne kosten für den Schritt

    const koord d = t - k;
    int dist = (ABS(d.x) + ABS(d.y)+1);

//    printf("Tunneldistanz %d\n", dist);

    if(dist > 1) {
  int cost;

  if(dist < 5) {
      cost = 18 * dist;
  } else if(dist < 7) {
      cost = 20 * dist;
  } else if(dist < 11) {
      cost = 24 * dist;
  } else {
      cost = 36 * dist;
  }

  //    printf("dist %d cost %d\n", dist, cost);

  if(check_step(k, t, start, ziel, cost, koord(sgn(d.x), sgn(d.y)))) {
      return true;
  }

    }
    return false;
}

bool
wegbauer_t::check_brueckenbau(const koord k, const koord t,
                                   const koord start, const koord ziel)
{
    // berechne kosten für den Schritt

    const koord d = t - k;
    int dist = (ABS(d.x) + ABS(d.y)+1);

    if(dist > 1) {
  int cost;

  if(dist < 5) {
      cost = 9 * dist;
  } else if(dist < 7) {
      cost = 10 * dist;
  } else if(dist < 11) {
      cost = 12 * dist;
  } else {
      cost = 18 * dist;
  }
  //    printf("dist %d cost %d\n", dist, cost);

  if(check_step(k, t, start, ziel, cost, koord(sgn(d.x), sgn(d.y)))) {
      return true;
  }

    }
    return false;
}

void
wegbauer_t::calc_route_init()
{
    // alle info's aufräumen
    slist_iterator_tpl<info_t *> queue_iter(queue);

    while(queue_iter.next()) {
  delete queue_iter.get_current();
    }

    queue.clear();


    // alle dir_forces aufräumen
    slist_iterator_tpl<dir_force_t *> forces_iter(dir_forces);

    while(forces_iter.next()) {
  delete forces_iter.get_current();
    }

    dir_forces.clear();

    INT_CHECK("simbau 583");

    // neu init.
    const int groesse = welt->gib_groesse();

    for(int j=0; j<groesse; j++) {
  for(int i=0; i<groesse; i++) {
      info->at(i, j).val = unseen;
  }
    }
}


void
wegbauer_t::intern_calc_route(const koord start, const koord ziel)
{
    koord k = ziel;

  // try to built extra straight track for these road types
  bool  straight_track = (bautyp==schiene_bot  ||  bautyp==schiene_bot_bau  ||  bautyp==leitung);
  // at the beginning try biggest distance first;
  // after half time try smallest minimize first
  int   dist=abs(start.x-ziel.x)+abs(start.y-ziel.y);
  int   dist_straight_start=1+(dist*3)/4;
  int   dist_straight_ziel=1+dist/4;


//dbg->message("wegbauer_t::intern_calc_route","From (%i,%i) to (%i,%i) [low %i] [high %i]",ziel.x,ziel.y,start.x,start.y,dist_straight_start,dist_straight_ziel);

    INT_CHECK("simbau 604");

    calc_route_init();

    INT_CHECK("simbau 608");

    // Verhindern, daß wir auf einem Depot enden!
    if(!ist_grund_fuer_strasse(ziel, koord(0,0), start, ziel)) {
  return;
    }
    // Verhindern, das wir eine Strasse der Länge 1 auf einem schiefen Hang bauen:
    if(!hang_t::ist_wegbar(welt->lookup(start)->gib_kartenboden()->gib_grund_hang())) {
  return;
    }
    if(update(k, 0)) {
  info->at(k).k = koord::invalid;;
  info->at(k).val = 0;
    }


    do {
  k = remove();

  koord t;

  if(baubaer == true && k != start && k != ziel) {
      // tunnelbau checken

      t = calc_tunnel_ziel(info->at(k).k, k);

      if(t.x != -1 && t.y != -1 && t != start && t != ziel) {
    // gültiges tunnelziel gefunden

    if(t.x == -2 && t.y == -2) {
        // gültiges tunnelziel gefunden
        // aber tunnel nicht betretbar -> zweite chance
//        info[k.x][k.y].val = unseen;
//        continue;

    } else {
        if(check_tunnelbau(k, t, start, ziel)) {
      // Ziel gefunden
      k = t;
      goto found;
        }
    }
      }

      t = calc_bruecke_ziel(info->at(k).k, k);

      if(t.x != -1 && t.y != -1 && t != start && t != ziel) {
        // gültiges brueckenziel gefunden

        if(t.x == -2 && t.y == -2) {
      // gültiges brueckenziel gefunden
      // aber bruecke nicht betretbar -> zweite chance
//      info[k.x][k.y].val = unseen;
//      continue;

        } else {
      if(check_brueckenbau(k, t, start, ziel)) {
          // Ziel gefunden
          k = t;
          goto found;
      }
        }

      }
  } // if(baubaer == true)

  koord force_dir = forces_lookup(k);
//  koord force_dir = koord(0, 0);

  koord next_try_dir[4];  // will be updated each step: biggest distance try first ...

  int current_dist =  abs(start.x-k.x)+abs(start.y-k.y);

    if(straight_track) {
    // we want to arrive straight at the target for the KI to be able to built a station
    bool  choose_x_first=abs(start.x-k.x)>abs(start.y-k.y);
    // then try to go straight to the start (after half of the distance)
    choose_x_first ^= (current_dist<=dist/2);

//dbg->message("wegbauer_t::intern_calc_route","choose_x_first %i",choose_x_first);
    if(   (start.y==k.y   ||  choose_x_first)  &&  start.x!=k.x    ) {
      // just decide east/west
      next_try_dir[start.x>k.x ? 0 : 3] = koord(1,0);
      next_try_dir[start.x>k.x ? 3 : 0] = koord(-1,0);
      next_try_dir[start.y>k.y ? 1 : 2] = koord(0,1);
      next_try_dir[start.y>k.y ? 2 : 1] = koord(0,-1);
    }
    else {
      // just decide north south first
      next_try_dir[start.y>k.y ? 0 : 3] = koord(0,1);
      next_try_dir[start.y>k.y ? 3 : 0] = koord(0,-1);
      next_try_dir[start.x>k.x ? 1 : 2] = koord(1,0);
      next_try_dir[start.x>k.x ? 2 : 1] = koord(-1,0);
    }
//dbg->message("wegbauer_t::intern_calc_route","At (%i,%i) [dist %i] dir (%i,%i) (%i,%i) (%i,%i) (%i,%i)",k.x,k.y,current_dist,next_try_dir[0].x,next_try_dir[0].y,next_try_dir[1].x,next_try_dir[1].y,next_try_dir[2].x,next_try_dir[2].y,next_try_dir[3].x,next_try_dir[3].y);
  }

  for(int r=0; r<4; r++) {

    koord zv;
    // use straight track algorithm?
      if(straight_track) {
        zv = next_try_dir[r];
      }
      else {
         zv = koord::nsow[r];
     }

      if(force_dir != koord(0,0) && zv != force_dir) {
          continue;
      }

            t = k + zv;

      if(t.x > 0 && t.y > 0 &&
               t.x < welt->gib_groesse()-1 && t.y < welt->gib_groesse()-1) {
    // prüfen, ob wir überhaupt dort hin dürfen

    grund_t *bd_von = welt->lookup(k)->gib_kartenboden();
    grund_t *bd_nach = welt->lookup(t)->gib_kartenboden();

    // spezielle checks, hanglage
    if(!hang_t::ist_wegbar(bd_nach->gib_grund_hang())) {
        continue;
    }


    // spezielle checks, Uferlage
    if(welt->gib_spieler(0) == sp && bd_nach->gib_hoehe() <= welt->gib_grundwasser()) {
        continue;
    }


    if(!kann_mit_strasse_erreichen(bd_von, zv)) {
        continue;
    }
    if(!kann_ribis_setzen(bd_von, zv)) {
        continue;
    }
    if(!kann_ribis_setzen(bd_nach, -zv)) {
        continue;
    }

    if(!ist_bruecke_tunnel_ok(bd_von, zv, bd_nach)) {
        continue;
    }

    // allgemeine checks, untergrund
                if(!ist_grund_fuer_strasse(t, zv, start, ziel)) {
        continue;
    }

    // berechne kosten für den Schritt
    int cost = calc_cost(t);
    // make it cheaper to go straight
    if(  straight_track  &&  (current_dist>=dist_straight_start  ||  current_dist<=dist_straight_ziel)  ) {
      cost += ((r+1)&0x06)*8;
    }

    if(check_step(k, t, start, ziel, cost, koord(0, 0))) {
        // Ziel gefunden
        k = t;
        goto found;
    }
      }
  }

  INT_CHECK("simbau 1042");
    } while(k != start && !queue.is_empty() && queue.count() < 500);

found:;

    if(k != start) {
  n = -1;
  max_n = -1;
    } else {

  n = 0;
  do {
      route->at(n++) = k;

//      printf("Route %d ist (%d,%d) mit val %d\n",
//             n, k.x, k.y,
//       info[k.x][k.y].val);

      k = info->at(k).k;
  } while(n<maximum && n<max_route_laenge && k != koord::invalid);

  max_n = n-1;
  n = 0;
    }
}

/* calc_route
 *
 */

void
wegbauer_t::calc_route(koord start, const koord ziel)
{
    INT_CHECK("simbau 740");

    if(baubaer) {
  intern_calc_route(start, ziel);
  const int len2 = max_n;

  INT_CHECK("simbau 745");
  intern_calc_route(ziel, start);


  INT_CHECK("simbau 755");

  const int len1 = max_n;


  if(len1 == -1 && len2 == -1) {
      return;
  }

  if(len1 == -1 && len2 > 0) {
      return;
  }

  if(len2 == -1 && len1 > 0) {
      intern_calc_route(start, ziel);
//      return;
  }

  if(len2 <= len1) {
      return;
  } else {
      intern_calc_route(start, ziel);
//      return;
  }

    } else {
  INT_CHECK("simbau 773");
  intern_calc_route(start, ziel);
    }

    INT_CHECK("simbau 778");

    // alle info's aufräumen
    slist_iterator_tpl<info_t *> queue_iter(queue);

    while(queue_iter.next()) {
  delete queue_iter.get_current();
    }

    queue.clear();


    // alle dir_forces aufräumen
    slist_iterator_tpl<dir_force_t*> forces_iter(dir_forces);

    while(forces_iter.next()) {
  delete forces_iter.get_current();
    }

    dir_forces.clear();

    INT_CHECK("simbau 797");
}


ribi_t::ribi
wegbauer_t::calc_ribi(int step)
{
    ribi_t::ribi ribi = ribi_t::keine;

    if(step < max_n) {
  // check distance, only neighbours
  const koord zv = route->at(step+1) - route->at(step);
  if(abs(zv.x) <= 1 && abs(zv.y) <= 1) {
      ribi |= ribi_typ(zv);
  }
    }
    if(step > 0) {
  // check distance, only neighbours
  const koord zv = route->at(step-1) - route->at(step);
  if(abs(zv.x) <= 1 && abs(zv.y) <= 1) {
      ribi |= ribi_typ(zv);
  }
    }
    return ribi;
}

bool
wegbauer_t::pruefe_route()
{
    int i;
    bool ok = true;
    for(i=0; i<= max_n; i++) {
  ok &= hang_t::ist_wegbar(welt->get_slope(route->at(i)));
    }

    if( !ok ) {
//  printf("Bauigel hat fehlerhafte route erzeugt !\n");
  max_n = -1;
    }
    return ok;
}

void
wegbauer_t::baue_tunnel_und_bruecken()
{
    // tunnel pruefen

    for(int i=0; i<max_n; i++) {
  koord d = route->at(i+1) - route->at(i);

  if(d.x > 1 || d.y > 1 || d.x < -1 || d.y < -1) {

      koord zv = koord (sgn(d.x), sgn(d.y));

      if(welt->lookup(route->at(i))->gib_kartenboden()->gib_hoehe() < welt->lookup(route->at(i)+zv)->gib_kartenboden()->gib_hoehe()) {

    if(bautyp == strasse) {
        tunnelbauer_t::baue(sp, welt, route->at(i), weg_t::strasse);
    } else if(bautyp == schiene || bautyp == schiene_bot_bau){
        tunnelbauer_t::baue(sp, welt, route->at(i), weg_t::schiene);
          }

      } else {

    if(bautyp == strasse) {
dbg->message("wegbauer_t::baue_tunnel_und_bruecken","built road bridge %p",besch);
dbg->message("wegbauer_t::baue_tunnel_und_bruecken","built road bridge with top speed %i",besch->gib_topspeed());
        brueckenbauer_t::baue(sp, welt, route->at(i), weg_t::strasse,besch->gib_topspeed());
    } else if(bautyp == schiene || bautyp == schiene_bot_bau){
dbg->message("wegbauer_t::baue_tunnel_und_bruecken","built rail bridge %p",besch);
dbg->message("wegbauer_t::baue_tunnel_und_bruecken","built rail bridge with top speed %i",besch->gib_topspeed());
        brueckenbauer_t::baue(sp, welt, route->at(i), weg_t::schiene,besch->gib_topspeed());
          }
      }
  }
    }
}

void wegbauer_t::optimiere_stelle(int index)
{
    if(index > 0 && index < max_n-1) {
  const ribi_t::ribi ribi = calc_ribi(index);
  koord p = route->at(index);
  koord p2 = route->at(index+1);

  koord d = p2-p;

  if(ABS(d.x) > 1 || ABS(d.y) > 1) {
      // das ist ein brücken/tunnelansatz
      return;
  }


  if(p.x > p2.x || p.y > p2.y) {
      koord tmp = p2;
            p2 = p;
      p = tmp;
  }


  const hang_t::typ hang1 = welt->get_slope( p );
  const hang_t::typ hang2 = welt->get_slope( p2 );

  bool ok;

  if(ribi == ribi_t::nordsued) {
      if(hang1 == hang_t::sued && hang2 == hang_t::nord) {
    ok = welt->can_raise(p.x, p.y+1);
    ok &= welt->can_raise(p.x+1, p.y+1);
    if(ok) {
        welt->raise(p+koord(0,1));
        welt->raise(p+koord(1,1));
    }

      }else if(hang1 == hang_t::nord && hang2 == hang_t::sued) {
    ok = welt->can_lower(p.x, p.y+1);
    ok &= welt->can_lower(p.x+1, p.y+1);
    if(ok) {
        welt->lower(p+koord(0,1));
        welt->lower(p+koord(1,1));
    }

      }

  } else if(ribi == ribi_t::ostwest) {
      if(hang1 == hang_t::ost && hang2 == hang_t::west) {
    ok = welt->can_raise(p.x+1, p.y);
    ok &= welt->can_raise(p.x+1, p.y+1);
    if(ok) {
        welt->raise(p+koord(1,0));
        welt->raise(p+koord(1,1));
    }

      } else if(hang1 == hang_t::west && hang2 == hang_t::ost) {
    ok = welt->can_lower(p.x+1, p.y);
    ok &= welt->can_lower(p.x+1, p.y+1);
    if(ok) {
        welt->lower(p+koord(1,0));
        welt->lower(p+koord(1,1));
    }

      }
  }
    }
}



void
wegbauer_t::baue_strasse()
{
    int cost = 0;

    for(int i=0; i<=max_n; i++) {
  const koord k = route->at(i);

  if(baubaer) {
      optimiere_stelle(i);
  }
  grund_t *gr = welt->lookup(k)->gib_kartenboden();

  if(gr->weg_erweitern(weg_t::strasse, calc_ribi(i))) {
    weg_t * weg = gr->gib_weg(weg_t::strasse);
    // keep faster ways ... (@author prissi)
    if(weg->gib_besch()!=besch  &&  !keep_existing_ways  &&  !(keep_existing_faster_ways  &&  weg->gib_besch()->gib_topspeed()>=besch->gib_topspeed())  ) {
      // Hajo: den typ des weges aendern, kosten berechnen

      if(sp) {
        sp->add_maintenance(besch->gib_wartung() - weg->gib_besch()->gib_wartung());
      }

      weg->setze_besch(besch);
      gr->calc_bild();
      cost = -besch->gib_preis();

    }
  } else {
      strasse_t * str = new strasse_t(welt);
      str->setze_besch(besch);

      gr->neuen_weg_bauen(str, calc_ribi(i), sp);

      cost = -besch->gib_preis();
  }

  if(cost && sp) {
    sp->buche(cost, k, COST_CONSTRUCTION);
  }
    } // for

    baue_tunnel_und_bruecken();
}


void
wegbauer_t::baue_leitung()
{
	int i;
	for(i=0; i<=max_n; i++) {
		const koord k = route->at(i);
		grund_t *gr = welt->lookup(k)->gib_kartenboden();

		leitung_t *lt = dynamic_cast <leitung_t *> (gr->suche_obj(ding_t::leitung));
		printf("powerline %p at (%i,%i)\n",lt,k.x,k.y);
		if(lt == 0) {
			lt = new leitung_t(welt, gr->gib_pos(), sp);
			sp->buche(CST_LEITUNG, gr->gib_pos().gib_2d(), COST_CONSTRUCTION);
			gr->obj_add(lt);
//			if(gr->gib_besitzer()==NULL) {
//				gr->setze_besitzer(sp);
//			}
		}
		lt->calc_neighbourhood();
	}
}


void
wegbauer_t::baue_schiene()
{
    int i;

    if(max_n >= 1) {

        // blockstrecken verwalten

        blockmanager * bm = blockmanager::gib_manager();

        for(i=1; i<max_n; i++) {

            if(baubaer) {
    optimiere_stelle(i);
            }
        }

        // schienen legen

        for(i=0; i<=max_n; i++) {
    int cost = 0;
    grund_t *gr = welt->lookup(route->at(i))->gib_kartenboden();
    ribi_t::ribi ribi = calc_ribi(i);

    if(gr->weg_erweitern(weg_t::schiene, ribi)) {
      weg_t * weg = gr->gib_weg(weg_t::schiene);
      if(weg->gib_besch() != besch && !keep_existing_ways) {
        // Hajo: den typ des weges aendern, kosten berechnen

        if(sp) {
    sp->add_maintenance(besch->gib_wartung() - weg->gib_besch()->gib_wartung());
        }

        weg->setze_besch(besch);
        gr->calc_bild();
        cost = -besch->gib_preis();
      }

      bm->schiene_erweitern(welt, gr);

  }
  else {
      schiene_t * sch = new schiene_t(welt);
      sch->setze_besch(besch);
      gr->neuen_weg_bauen(sch, ribi, sp);

      bm->neue_schiene(welt, gr);
      cost = -besch->gib_preis();
    }

    if(cost) {
      sp->buche(cost, gr->gib_pos().gib_2d(), COST_CONSTRUCTION);
    }

    gr->calc_bild();
        }

  // V.Meyer: weg_position_t changed to grund_t::get_neighbour()
  grund_t *start = welt->lookup(route->at(0))->gib_kartenboden();
  grund_t *end = welt->lookup(route->at(max_n))->gib_kartenboden();
  grund_t *to;

        for (i=0; i <= 4; i++) {
            if (start->get_neighbour(to, weg_t::schiene, koord::nsow[i])) {
                to->calc_bild();
            }
            if (end->get_neighbour(to, weg_t::schiene, koord::nsow[i])) {
                to->calc_bild();
            }
        }
    }

    baue_tunnel_und_bruecken();
}

void
wegbauer_t::baue()
{
  dbg->message("wegbauer_t::baue()",
         "type=%d max_n=%d start=%d,%d end=%d,%d",
         bautyp, max_n,
         route->at(0).x, route->at(0).y,
         route->at(max_n).x, route->at(max_n).y);


    INT_CHECK("simbau 1072");
    switch(bautyp) {
    case strasse:
    case strasse_bot:
  baue_strasse();
  break;
    case schiene:
    case schiene_bot:
    case schiene_bot_bau:
  baue_schiene();
  break;
    case leitung:
  baue_leitung();
  break;
    }
    INT_CHECK("simbau 1087");
}
