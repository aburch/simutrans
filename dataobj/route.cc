/*
 * route.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include <stdio.h>
#include <string.h>

#include "../simworld.h"
#include "../simintr.h"
#include "../simtime.h"
#include "../simmem.h"
#include "../simhalt.h"
#include "../boden/wege/weg.h"
#include "../boden/grund.h"
#include "../ifc/fahrer.h"
#include "../ifc/route_block_tester_t.h"

#include "../tpl/prioqueue_tpl.h"

#include "loadsave.h"

#include "route.h"


// dies wird fuer die routenplanung benoetigt

class KNode {
private:

public:
    KNode * link;
    uint32  dist;
    uint32  total;
    koord3d pos;

    inline bool operator < (const KNode &k) const {
	return dist+total <= k.dist+k.total;
    };
};


void route_t::entferne_markierungen() {
    welt->unmarkiere_alle();
}


route_t::route_t()
{
    for(int i=0; i<512; i++) {
	route[i] = koord3d(0, 0, 0);
    }

    max_n = -1;
}

void
route_t::kopiere(const route_t *r)
{
    assert(r != NULL);

    // es gibt am ende ein sonderfeld!

    //    for(int i=0; i<=r->max_n+1; i++) {
    //	route[i] = r->route[i];
    //    }

    memcpy(route, r->route, sizeof(koord3d)*(r->max_n+1));


    max_n = r->max_n;
}

void
route_t::insert(koord3d k)
{
    assert(max_n < 511);

    // es gibt am ende ein sonderfeld!


    memmove(&route[1], &route[0], sizeof(koord3d)*(max_n+1));

    route[0] = k;

    max_n ++;
}


static inline bool am_i_there(karte_t *welt,
			      koord3d pos,
			      koord3d ziel,
			      bool block_test)
{
  bool ok = false;

  if(block_test) {
    halthandle_t halt1 =
      haltestelle_t::gib_halt(welt, pos);

    halthandle_t halt2 =
      haltestelle_t::gib_halt(welt, ziel);

    ok = halt1.is_bound() ? (halt1 == halt2) : (pos == ziel);

  } else {
    ok = (pos == ziel);
  }

  return ok;
}


bool
route_t::intern_calc_route(const koord3d ziel, const koord3d start,
                           fahrer_t *fahr)
{
    const int MAX_STEP = 10000;
    static KNode nodes[MAX_STEP+4+1];
    int step = 0;

    // ziel und start sind hier gegenueber dem Aufruf vertauscht,
    // da diese routine eine Route von ziel nach start berechnet
    // die Berechnung erfolgt durch eine Breitensuche fuer Graphen

    // Warteschlange fuer Breitensuche
    // Kann statisch sein, weil niemals zwei Fahrzuege zugleich eine route
    // suchen ... statisch bedeutet wiedernutzung bereits erzeugter nodes
    // und ist in diesem Fall effizeinter
    static prioqueue_tpl <KNode *> queue;

    // da die queue statisch ist müssen wir die reste der alten suche
    // erst aufräumen

    queue.clear();

    // Hajo: hack - check for ship

    const grund_t * gr = welt->lookup(start);
    const bool is_ship = gr && gr->ist_wasser();

    // Hajo: init first node

    KNode *tmp = &nodes[step++];

    tmp->link = NULL;
    tmp->dist = is_ship ? ABS(start.x-ziel.x) + ABS(start.y-ziel.y) : 0;
    tmp->total = 0;
    tmp->pos = start;


    queue.insert( tmp );        // init queue mit erstem feld

    welt->markiere(tmp->pos);  // betretene Felder markieren

//    printf("Start ist %d %d\n", start.x, start.y);
//    printf("pos ist %d %d ziel ist %d %d\n", tmp->pos.x, tmp->pos.y, ziel.x, ziel.y);

    // Breitensuche

    // const long t0 = get_current_time_millis();


    // Hajo: moved out of loop to optimize
    const weg_t::typ wegtyp = fahr->gib_wegtyp();
    koord3d k_next;
    // V.Meyer: weg_position_t changed to grund_t::get_neighbour()
    grund_t *to;

    do {
	// Hajo: this is too expensive to be called each step
	if((step & 127) == 0) {
	    INT_CHECK("route 161");
	}

	tmp = queue.pop();


	// debugging
	// intr_refresh_display(false);


	//print("Teste route at %d,%d,%d\n", tmp->pos.x,tmp->pos.y,tmp->pos.z);
	if(tmp->pos != ziel) {                      // am ziel ?
	    gr = welt->lookup(tmp->pos);

	    if(gr && fahr->ist_befahrbar(gr)) {
	      const ribi_t::ribi ribi = gr->gib_weg_ribi(wegtyp);

	      //print("Teste route at %d,%d,%d -> ribi %d\n", tmp->pos.x,tmp->pos.y,tmp->pos.z, ribi);

	      for(int r=0; r<4; r++) {

	        if((ribi & ribi_t::nsow[r])
		   && gr->get_neighbour(to, wegtyp, koord::nsow[r])
		   && !welt->ist_markiert(k_next = to->gib_pos())
		   && (block_tester == 0
		       || tmp->total < 30
		       || !block_tester->is_blocked(tmp->pos, k_next))

		   ) {

		    KNode *k = &nodes[step++];

		    k->link = tmp;
		    k->dist = is_ship ? ABS(k_next.x-ziel.x) +
		                        ABS(k_next.y-ziel.y) : 0;
		    k->total = tmp->total+1;
		    k->pos = k_next;

		    queue.insert( k );

		    welt->markiere(k->pos);  // betretene Felder markieren
		}
	      }
	    }
	}
    } while(!queue.is_empty()
	    && !am_i_there(welt, tmp->pos, ziel, block_tester != 0)
	    && step < MAX_STEP);

    // const long t1 = get_current_time_millis();
    // printf("Route calc took %ld ms for %d steps\n", t1-t0, step);

    INT_CHECK("route 194");

    // routenbau init.
    max_n = -1;
    bool ok = false;

    // Abbruch pruefen
    if(!am_i_there(welt, tmp->pos, ziel, block_tester != 0)
       || step >= MAX_STEP) {
	if(step >= MAX_STEP) {
	    dbg->warning("route_t::intern_calc_route()",
	                 "Too many steps in route (too long/complex)");
	}
	// keine Route gefunden: zu lange strecke ?!
    } else {
        // route zusammenbauen
        int n = 0;

        while(tmp != NULL && n < 511) {
	    route[n++] = tmp->pos;

	    //printf("Route %d: at %d,%d,%d\n", n, tmp->pos.x, tmp->pos.y, tmp->pos.z);

	    tmp = tmp->link;
    	}

	// wir lassen eine reserve für "insert()" aufrufe - eigentlich
	// 511 - maximale_convoi_laenge
    	if(n >= 480) {
            dbg->warning("route_t::intern_calc_route()", "Route too long");
	    n = 0;
    	} else {
    	    // einige berechnungen brauchen noch zwei felder extra
    	    route[n+1] = route[n] = route[n-1];

    	    // start und maximum init.
    	    max_n = n-1;
    	    n = 0;
	    // wir koennen fahren, wenn die Route min. ein Feld (index 0) enthaelt
	    ok = max_n >= 0;
	}
    }

    return ok;
}


bool
route_t::calc_route(karte_t *w,
                    const koord3d ziel, const koord3d start,
                    fahrer_t *fahr)
{
    welt = w;
    block_tester = 0;

    INT_CHECK("route 336");

    const bool ok = intern_calc_route(ziel, start, fahr);

    INT_CHECK("route 343");

    entferne_markierungen();

    INT_CHECK("route 347");

    if( !ok ) {
	route[0] = start;		// muss Vehikel spaeter entfernen
	route[1] = start;              	// koennen
	max_n = -1;

	dbg->message("route_t::calc_route()",
		     "No route from %d,%d to %d,%d found",
		     start.x, start.y, ziel.x, ziel.y);
    } else {
	//	printf("Route mit %d Schritten gefunden.\n",max_n);

	halthandle_t halt = welt->lookup(start)->gib_halt();

	// in bahnhof einfahren
	if(halt.is_bound() && max_n < 510) {
	    if(max_n > 0 &&
	       welt->lookup(start)->gib_weg(weg_t::schiene) != NULL) {

		const koord zv = route[max_n].gib_2d() - route[max_n-1].gib_2d();
		const int ribi = fahr->gib_ribi(welt->lookup(start));
                koord3d k = start;
                koord3d k2 = start + zv;

		while(max_n < 510 &&
		      welt->lookup(k2) &&
		      welt->lookup(k2)->gib_halt() == halt &&
		      welt->lookup(k2)->gib_weg(weg_t::schiene) != NULL &&
		      (fahr->gib_ribi(welt->lookup(k2)) & ribi))
		{
		    k = k2;
		    k2 += zv;
                    max_n++;
		    route[max_n] = k;
		}
	    }
	}
        if(max_n < 511)
	   route[max_n+1] = route[max_n];
    }
    return ok;
}


bool
route_t::calc_unblocked_route(karte_t *w,
			      const koord3d start,
			      const koord3d ziel,
			      fahrer_t *fahr,
			      route_block_tester_t *tester)
{
    welt = w;
    block_tester = tester;

    const bool ok = intern_calc_route(ziel, start, fahr);
    entferne_markierungen();

    block_tester = 0;

    if( !ok ) {
	route[0] = start;		// muss Vehikel spaeter entfernen
	route[1] = start;              	// koennen
	max_n = -1;

	dbg->message("route_t::calc_unblocked_route()",
		     "No route from %d,%d to %d,%d found",
		     start.x, start.y, ziel.x, ziel.y);
    } else {
	//	printf("Route mit %d Schritten gefunden.\n",max_n);

        if(max_n < 511)
	   route[max_n+1] = route[max_n];
    }
    return ok;
}





void
route_t::rdwr(loadsave_t *file)
{
    int i;

    file->rdwr_int(max_n, "\n");

    for(i=0; i<=max_n; i++) {
	route[i].rdwr( file );
    }
    if(file->is_loading()) {    // War da nicht noch was mit 1 extra? V. Meyer
        route[i] = route[i - 1];
    }
}

bool route_t::find_path(karte_t * welt, const koord3d start, fahrer_t * fahr, ding_t::typ typ)
{
	block_tester = 0;
  const int MAX_STEP = 10000;
  KNode nodes[MAX_STEP+4+1];
  int step = 0;

  prioqueue_tpl <KNode *> queue;
  dbg->message("path_t::find_path()", "start:\t%d,%d", start.x, start.y);

  queue.clear();

  grund_t * gr = welt->lookup(start);

  // Hajo: init first node

  KNode *tmp = &nodes[step++];

  tmp->pos =   start;
  tmp->dist =  0;
  tmp->total = 0;
  tmp->link =  0;


  // Hajo: init queue with first node
  queue.insert( tmp );
	welt->markiere(tmp->pos);  // betretene Felder markieren

  const weg_t::typ wegtyp = fahr->gib_wegtyp();
  koord3d k_next;
  grund_t *to;

  do {
    tmp = queue.pop();

    // Hajo: is destination reached?
    if(is_ding_there(welt, tmp->pos, typ) == false &&
       tmp->total < 250 &&
       step < MAX_STEP) {
	    gr = welt->lookup(tmp->pos);
	    if(gr && fahr->ist_befahrbar(gr)) {
	      const ribi_t::ribi ribi = gr->gib_weg_ribi(wegtyp);
	      for(int r=0; r<4; r++) {
	      	if((ribi & ribi_t::nsow[r])
							&& gr->get_neighbour(to, wegtyp, koord::nsow[r])
							&& !welt->ist_markiert(k_next = to->gib_pos())
							) {

							KNode *k = &nodes[step++];

							k->pos   = k_next;
							k->dist  = 0;
							k->total = tmp->total+1;
							k->link  = tmp;

							queue.insert( k );

							// Hajo: mark entered squares
							welt->markiere(k->pos);
						}
	      }
	    }
    }
  } while(queue.is_empty() == false
	  && is_ding_there(welt, tmp->pos, typ) == false
	  && step < MAX_STEP);

	max_n = -1;
  bool ok = false;

	if(!is_ding_there(welt, tmp->pos, typ) || step >= MAX_STEP) {
		if(step >= MAX_STEP) {
			dbg->warning("route_t::find_path()", "Too many steps in route (too long/complex)");
		}
	} else {
		// route zusammenbauen
		int n = 0;

		while(tmp != NULL && n < 511) {
			route[n++] = tmp->pos;
			tmp = tmp->link;
		}
		if(n >= 480) {
			dbg->warning("route_t::intern_calc_route()", "Route too long");
			n = 0;
		} else {
			route[n+1] = route[n] = route[n-1];
			max_n = n-1;
			n = 0;
			ok = max_n >= 0;
		}
	}
  if( !ok ) {
		route[0] = start;		// muss Vehikel spaeter entfernen
		route[1] = start;              	// koennen
		max_n = -1;
		dbg->message("route_t::find_path()", "No route from %d,%d to a depot found",
		     start.x, start.y);
  } else {
    if(max_n < 511)
			route[max_n+1] = route[max_n];
  }
	// entferne_markierungen();
  return ok;
}

bool
route_t::is_ding_there(karte_t * welt, const koord3d pos, ding_t::typ typ)
{
	bool ok = false;
	grund_t * gr = welt->lookup(koord(pos.x,pos.y))->gib_kartenboden();
	if (gr) {
		ding_t * ding = gr->suche_obj(typ);
		if (ding) {
			ok = true;
		}
	}
	return ok;
}
