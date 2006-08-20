/*
 * fabrikbauer.cc
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "../simdebug.h"
#include "fabrikbauer.h"
#include "../simworld.h"
#include "../simfab.h"
#include "../simdisplay.h"
#include "../simgraph.h"
#include "../simtools.h"
#include "../simcity.h"
#include "../simskin.h"
#include "../simhalt.h"
#include "../simplay.h"

#include "../dings/leitung.h"

#include "../boden/grund.h"
#include "../boden/wege/dock.h"

#include "../dataobj/einstellungen.h"
#include "../dataobj/translator.h"

#include "../sucher/bauplatz_sucher.h"


// Hajo: average industry distance
#define DISTANCE 50


/**
 * bauplatz_mit_strasse_sucher_t:
 *
 * Sucht einen freien Bauplatz mithilfe der Funktion suche_platz().
 *
 * @author V. Meyer
 */
class bauplatz_mit_strasse_sucher_t: public bauplatz_sucher_t  {
public:
    bauplatz_mit_strasse_sucher_t(karte_t *welt) : bauplatz_sucher_t (welt) {}

    bool strasse_bei(int x, int y) const {
  grund_t *bd = welt->lookup(koord(x, y))->gib_kartenboden();

  return bd && bd->gib_weg(weg_t::strasse);
    }
    virtual bool ist_platz_ok(koord pos, int b, int h) const {
  if(bauplatz_sucher_t::ist_platz_ok(pos, b, h)) {
      int i;
      for(i = pos.y; i < pos.y + h; i++) {
    if(strasse_bei(pos.x - 1, i) ||  strasse_bei(pos.x + b, i)) {
        return true;
    }
      }
      for(i = pos.x; i < pos.x + b; i++) {
    if(strasse_bei(i, pos.y - 1) ||  strasse_bei(i, pos.y + h)) {
        return true;
    }
      }
  }
  return false;
    }
};


class wasserplatz_sucher_t : public platzsucher_t {
public:
    wasserplatz_sucher_t(karte_t *welt) : platzsucher_t(welt) {}

    virtual bool ist_feld_ok(koord pos, koord  d) const {
  const grund_t *gr = welt->lookup(pos + d)->gib_kartenboden();

  return gr->ist_wasser();
    }
};



stringhashtable_tpl<const fabrik_besch_t *> fabrikbauer_t::table;

slist_tpl <fabrikbauer_t::stadt_fabrik_t> fabrikbauer_t::gebaut;


void fabrikbauer_t::bau_info_t::random(slist_tpl <const fabrik_besch_t *> &fab)
{
    slist_iterator_tpl<const fabrik_besch_t *> iter(fab);
    int gewichtung = 0;
    int next;

    while(iter.next()) {
  gewichtung += iter.get_current()->gib_gewichtung();
    }
    if(gewichtung > 0) {
  next = simrand(gewichtung);
  iter.begin();
  while(iter.next()) {
      if(next < iter.get_current()->gib_gewichtung()) {
    info = iter.get_current();
    besch = hausbauer_t::finde_fabrik(info->gib_name());

    if(besch == 0) {
      dbg->fatal("fabrikbauer_t::bau_info_t::random",
           "No description found for '%s'",
           info->gib_name());
    }

    break;
      }
      next -= iter.get_current()->gib_gewichtung();
  }
  rotate = simrand(4);
  dim = besch->gib_groesse(rotate);
    }
    else {
  besch = NULL;
  info = NULL;
  dim = koord(1,1);
  rotate = 0;
    }
}

void fabrikbauer_t::bau_info_t::random_land(slist_tpl <const fabrik_besch_t *> &fab)
{
    switch(simrand( 5 ) ) {
    case 0:
    case 1:
  random(fab);
  break;
    case 2:
    case 3:
    case 4:
  info = NULL;
  besch = hausbauer_t::waehle_sehenswuerdigkeit();
  if(!besch) {
      random(fab);
  }
  break;
    }
    rotate = simrand(4);
    if(besch) {
  dim = besch->gib_groesse(rotate);
    }
}


const fabrik_besch_t *fabrikbauer_t::gib_fabesch(const char *fabtype)
{
    return table.get(fabtype);
}

void fabrikbauer_t::register_besch(fabrik_besch_t *besch)
{
    table.put(besch->gib_name(), besch);
}


const fabrik_besch_t *
fabrikbauer_t::finde_hersteller(const ware_besch_t *ware)
{
    stringhashtable_iterator_tpl<const fabrik_besch_t *> iter(table);

    while(iter.next()) {
  const fabrik_besch_t *tmp = iter.get_current_value();

  if(tmp->gib_platzierung() != fabrik_besch_t::Wasser) {
      for(int i = 0; i < tmp->gib_produkte(); i++) {
    const fabrik_produkt_besch_t *produkt = tmp->gib_produkt(i);

    if(produkt->gib_ware() == ware) {
        return tmp;
    }
      }
  }
    }
    dbg->error("fabrikbauer_t::finde_hersteller()",
         "no producer for good '%s' was found\n", translator::translate(ware->gib_name()));
    return NULL;
}


koord3d
fabrikbauer_t::finde_zufallsbauplatz(karte_t * welt, const koord3d pos, const int radius, const koord groesse)
{
    array_tpl<koord3d> list(2048);
    int index = 0;
    koord k;

    // assert(radius <= 32);

    for(k.y=pos.y-radius; k.y<=pos.y+radius; k.y++) {
  for(k.x=pos.x-radius; k.x<=pos.x+radius; k.x++) {
      if(fabrik_t::ist_bauplatz(welt, k, groesse)) {
    list.at(index ++) = welt->lookup(k)->gib_kartenboden()->gib_pos();

    // nicht gleich daneben nochmal suchen
    k.x += 4;
      }
  }
    }

    // printf("Zufallsbauplatzindex %d\n", index);

    if(index == 0) {
  return koord3d(-1, -1, -1);
    } else {
  return list.at(simrand(index));
    }
}

void
fabrikbauer_t::verteile_industrie(karte_t * welt, spieler_t *sp, int dichte)
{
    bau_info_t wasser_bau;
    bau_info_t land_bau;

    print("Distributing industries ...\n");

    slist_tpl <const fabrik_besch_t *> wasser_fab, land_fab;

    // fabriken aufteilen
    // - auf Wasser gebaute
    // - auf Land gebaute
    // - an Stadt gebaute
    stringhashtable_iterator_tpl<const fabrik_besch_t *> iter(table);

    while(iter.next()) {
  const fabrik_besch_t *info = iter.get_current_value();

  if(info->gib_platzierung() == fabrik_besch_t::Wasser) {
      wasser_fab.insert(info);
  } else {
      // nur endverbraucher eintragen
      if(info->gib_produkt(0) == NULL) {
    land_fab.insert(info);
      }
  }
    }
    const int groesse = welt->gib_groesse();
    const int staedte = welt->gib_einstellungen()->gib_anzahl_staedte();
    const int display_offset = groesse/2 + staedte*12;
    const int display_part = groesse/2;
    const int display_total = groesse+staedte*12;

    gebaut.clear();

    wasser_bau.random(wasser_fab);
    land_bau.random_land(land_fab);

    //
    // Zur Verteilung der Industrien legen unsere Karte mit quadratischen Kacheln aus .
    // Auf jeder Kachel wird versucht, etwas zu bauen.
    // Die Anzahl der Kachel wird quadratisch aus dem Parameter Industriedichte
    // abgeleteitet und natürlich der Größe der Karte.
    // (2 Reihen am Rand lasen wir dabei aus - da baut sichs nicht so gut)
    //
    const int reihen = MAX(1, groesse * DISTANCE / (dichte*3));
    int i, j, n;

    for(i = 0; i < reihen; i++) {
  for(j = 0; j < reihen; j++) {
      if(is_display_init()) {
    display_progress(display_offset + display_part * (i * reihen + j) / reihen / reihen,
        display_total);

    display_flush(0, 0, 0, "", "");
      }
      /*
       * Geben wir uns weitere Chancen, wenn kein Bau zustande kam?
       * n = 1: Verteilung dünner und ungleichmäßiger als n = 2
       * ...
       * n = 4: recht gleichmäßige Verteilung
       * n = 9: kaum noch "Löcher" zu sehen
       */
      for(n = 2; n > 0; n--) {
    // Wir wählen unseren Bauplatz irgendwo im Viereck
    // um unseren Bauplatz herum.
    koord k(
        2 + (groesse - 4) * i / reihen + simrand((groesse - 5) / reihen + 1),
        2 + (groesse - 4) * j / reihen + simrand((groesse - 5) / reihen + 1));


    if(welt->ist_wasser(k, wasser_bau.dim)) {
        // Wasserbau:
        if(wasser_bau.info &&
      k.x >= 2 && k.x + wasser_bau.dim.x < groesse-2 &&
           k.y >= 2 && k.y + wasser_bau.dim.y < groesse-2)
        {
      koord p;
      bool ok = true;

      for(p.x=k.x-2; ok && p.x<k.x+wasser_bau.dim.x+2; p.x++) {
          for(p.y=k.y-2; ok && p.y<k.y+wasser_bau.dim.y+2; p.y++) {
        ok = welt->lookup(p)->gib_kartenboden()->ist_wasser();
          }
      }
      if(ok && !fabrik_t::ist_da_eine(welt, k.x-5, k.y-5, k.x+wasser_bau.dim.x+4, k.y+wasser_bau.dim.y+4))
      {
          baue_fabrik(welt, NULL, wasser_bau.info, wasser_bau.rotate, welt->lookup(k)->gib_kartenboden()->gib_pos(), sp);
          wasser_bau.random(wasser_fab);
          n = 0;
      }
        }
    }
    else if(!land_bau.info || land_bau.info->gib_platzierung() == fabrik_besch_t::Land) {
        // Landbau:
        if(fabrik_t::ist_bauplatz(welt, k, land_bau.dim) && land_bau.besch) {
      koord3d pos = welt->lookup(k)->gib_kartenboden()->gib_pos();

      if(land_bau.info) {
          baue_hierarchie(welt, NULL, land_bau.info, land_bau.rotate, &pos, welt->gib_spieler( 1 ));
      } else {
          hausbauer_t::baue(welt, welt->gib_spieler(1), pos, land_bau.rotate, land_bau.besch);
      }
      n = 0;
      land_bau.random_land(land_fab);
        }

    }
    else {
        // Staddtbau:
        stadt_fabrik_t sf;

        sf.stadt = welt->suche_naechste_stadt(k);
        sf.info = land_bau.info;
        // Ein Stadt suchen, die noch am wengisten dieser Fabriken hat.
        while(sf.stadt && gebaut.contains(sf)) {
      sf.stadt = welt->suche_naechste_stadt(k, sf.stadt);
        }
        if(!sf.stadt) {
      // Alle habe gleich viel, da fange wir von vorne an
      // zu rechnen:
      for(unsigned int i = 0; i < welt->gib_staedte()->get_size(); i++) {
          sf.stadt = welt->gib_staedte()->get(i);
          gebaut.remove(sf);
      }
      sf.stadt = welt->suche_naechste_stadt(k);
        }
        //
        // Drei Varianten:
        // A:
        // Ein Bauplatz, möglichst nah am Rathaus mit einer Strasse daneben.
        // Das könnte ein Zeitproblem geben, wenn eine Stadt keine solchen Bauplatz
        // hat und die Suche bis zur nächsten Stadt weiterläuft
        // Ansonsten erscheint mir das am realistischtsten..
        k = bauplatz_mit_strasse_sucher_t(welt).suche_platz(sf.stadt->gib_pos(), land_bau.dim.x, land_bau.dim.y);
        // B:
        // Gefällt mir auch. Die Endfabriken stehen eventuell etwas außerhalb der Stadt
        // aber nicht weit weg.
        //k = finde_zufallsbauplatz(welt, welt->lookup(sf.stadt->gib_pos())->gib_boden()->gib_pos(), 3, land_bau.dim).gib_2d();
        // C:
        // Ein Bauplatz, möglichst nah am Rathaus.
        // Wenn mehrere Endfabriken bei einer Stadt landen, sind die oft hinter
        // einer Reihe Häuser "versteckt", von Strassen abgeschnitten.
        //k = bauplatz_sucher_t(welt).suche_platz(sf.stadt->gib_pos(), land_bau.dim.x, land_bau.dim.y);

        if(k != koord::invalid) {
      koord3d pos = welt->lookup(k)->gib_kartenboden()->gib_pos();
      baue_hierarchie(welt, NULL, land_bau.info, land_bau.rotate, &pos, welt->gib_spieler( 1 ));
      gebaut.insert(sf);
      n = 0;
      land_bau.random_land(land_fab);
        }
    }
      }
  }
    }
    {
  slist_iterator_tpl<fabrik_t *> iter (welt->gib_fab_list());

  while(iter.next()) {
      fabrik_t *fab = iter.get_current();

      if(strcmp(fab->gib_name(), "Oelbohrinsel") == 0) {

    int dist = 9999999;
    fabrik_t *best = NULL;
    slist_iterator_tpl<fabrik_t*> iter2 (welt->gib_fab_list());
    while(iter2.next()) {
        fabrik_t *fab2 = iter2.get_current();

        if(strcmp(fab2->gib_name(), "Raffinerie") == 0) {
      const koord dv = fab2->gib_pos().gib_2d() - fab->gib_pos().gib_2d();
      const int d = ABS(dv.x) + ABS(dv.y);

      if(d < dist) {
          dist = d;
          best = fab2;
      }
        }
    }
    if(best) {
        fab->add_lieferziel(best->gib_pos().gib_2d());
    }
      } else if(strcmp(fab->gib_name(), "Kohlekraftwerk") == 0  ||  strcmp(fab->gib_name(), "Oil Power Plant") == 0) {
    koord3d pos = fab->gib_pos()+koord3d(2,0,0);

    if(skinverwaltung_t::pumpe && welt->lookup(pos)) {
        // too near to edge of map or wrong level?
        pumpe_t *pumpe = new pumpe_t(welt, pos, sp);
        pumpe->setze_fabrik(fab);

        welt->lookup(pos)->obj_loesche_alle(NULL);
        welt->lookup(pos)->obj_add(pumpe);
    }
      }
  }
    }
    // Nun können noch zusätzliche Lieferziele ermittelt werden.

    const slist_tpl <fabrik_t *> & list = welt->gib_fab_list();

    slist_iterator_tpl <fabrik_t *> quellen (list);

    while(quellen.next()) {

  const vector_tpl <ware_t> * aus = quellen.get_current()->gib_ausgang();

  for(uint32 i=0; i<aus->get_count(); i++) {
      ware_t ware = aus->get(i);

      slist_iterator_tpl <fabrik_t *> ziele (list);

      while(ziele.next()) {
    const vector_tpl <ware_t> * ein = ziele.get_current()->gib_eingang();

    bool ok = false;

    for(uint32 i=0; i<ein->get_count(); i++) {
        if(ein->get(i).gib_typ() == ware.gib_typ()) {
      ok = true;
      break;
        }
    }

    if(ok) {
        const koord pos1 = quellen.get_current()->gib_pos().gib_2d();
        const koord pos2 = ziele.get_current()->gib_pos().gib_2d();
        const int dist = abs(pos1.x - pos2.x) + abs(pos1.y - pos2.y);

        if(dist < 60 || dist < simrand(300)) {
      quellen.get_current()->add_lieferziel(ziele.get_current()->gib_pos().gib_2d());
        }
    }
      }
  }
    }
}


fabrik_t *
fabrikbauer_t::baue_fabrik(karte_t * welt, koord3d *parent, const fabrik_besch_t *info, int rotate, koord3d pos, spieler_t *sp)
{
    halthandle_t halt;

    if(info->gib_platzierung() == fabrik_besch_t::Wasser) {
  const haus_besch_t *besch = hausbauer_t::finde_fabrik(info->gib_name());
  koord dim = besch->gib_groesse(rotate);

        koord k;
  halt = welt->gib_spieler(0)->halt_add(pos.gib_2d());

  halt->set_pax_enabled( true );
  halt->set_ware_enabled( true );
  halt->set_post_enabled( true );

  for(k.x=pos.x-1; k.x<=pos.x+dim.x; k.x++) {
      for(k.y=pos.y-1; k.y<=pos.y+dim.y; k.y++) {
    if(! halt->ist_da(k)) {
        const planquadrat_t *plan = welt->lookup(k);

        if(plan != NULL) {
      grund_t *gr = plan->gib_kartenboden();

      if(gr->ist_wasser() && gr->gib_weg(weg_t::wasser) == 0) {
          gr->neuen_weg_bauen(new dock_t(welt), ribi_t::alle, welt->gib_spieler(0));
          halt->add_grund( gr );
      }
        }
    }
      }
  }
    }

    fabrik_t * fab = new fabrik_t(welt, pos, sp, info);
    int i;

    vector_tpl<ware_t> *eingang = new vector_tpl<ware_t> (info->gib_lieferanten());
    vector_tpl<ware_t> *ausgang = new vector_tpl<ware_t> (info->gib_produkte());

    for(i=0; i < info->gib_lieferanten(); i++) {
  const fabrik_lieferant_besch_t *lieferant = info->gib_lieferant(i);
  ware_t ware(lieferant->gib_ware());

  ware.max = lieferant->gib_kapazitaet() << fabrik_t::precision_bits;

  eingang->append(ware);
    }
    for(i=0; i < info->gib_produkte(); i++) {
  const fabrik_produkt_besch_t *produkt = info->gib_produkt(i);
  ware_t ware(produkt->gib_ware());

  ware.max = produkt->gib_kapazitaet() << fabrik_t::precision_bits;

  ausgang->append(ware);
    }
    fab->set_eingang( eingang );
    fab->set_ausgang( ausgang );
    fab->set_prodbase( 1 );
    fab->set_prodfaktor( info->gib_produktivitaet() +
                         simrand(info->gib_bereich()) );
    if(parent) {
  fab->add_lieferziel(parent->gib_2d());
    }

    fab->baue(rotate, true);

    welt->add_fab(fab);

    if(info->gib_platzierung() == fabrik_besch_t::Wasser) {
        welt->lookup(halt->gib_basis_pos())->gib_kartenboden()->setze_text( translator::translate(fab->gib_name()) );
    }

    if(halt.is_bound()) {
      halt->verbinde_fabriken();
    }

    return fab;
}


/**
 * vorbedingung: pos ist für fabrikbau geeignet
 */
int
fabrikbauer_t::baue_hierarchie(karte_t * welt, koord3d *parent, const fabrik_besch_t *info, int rotate, koord3d *pos, spieler_t *sp)
{
    int n = 1;

    baue_fabrik(welt, parent, info, rotate, *pos, sp);

    for(int i=0; i < info->gib_lieferanten(); i++) {
  const fabrik_lieferant_besch_t *lieferant = info->gib_lieferant(i);
  const ware_besch_t *ware = lieferant->gib_ware();
  const fabrik_besch_t *hersteller = finde_hersteller(ware);
  const haus_besch_t *haus = hersteller->gib_haus();

  int lcount = lieferant->gib_anzahl();

  if(lcount == 0) {
    // Hajo: search if there already is one

    const slist_tpl<fabrik_t *> & list = welt->gib_fab_list();
    slist_iterator_tpl <fabrik_t *> iter (list);
    bool found = false;

    while( iter.next() ) {
      fabrik_t * fab = iter.get_current();

      if(fab->vorrat_an(ware) > -1) {

        const koord3d d = fab->gib_pos() - *pos;
        const int distance = abs(d.x) + abs(d.y);
        const bool ok = distance < 60 || distance < simrand(240);

        if(ok) {
    found = true;
    fab->add_lieferziel(pos->gib_2d());
        }
      }
    }


    // Hajo: if none exist, build one
    lcount = found ? 0 : 1;
  }


  for(int j=0; j<lcount; j++) {
      int rotate = simrand(4);

      koord3d k = finde_zufallsbauplatz(welt, *pos, DISTANCE, haus->gib_groesse(rotate));
      if(welt->lookup(k)) {
    n += baue_hierarchie(welt, pos, hersteller, rotate, &k, sp);
      }
  }
    }
    return n;
}
