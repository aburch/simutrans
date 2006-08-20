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

#include "../simdebug.h"
#include "../simworld.h"
#include "../simintr.h"
#include "../simtime.h"
#include "../simmem.h"
#include "../simhalt.h"
#include "../boden/wege/weg.h"
#include "../boden/grund.h"
#include "../ifc/fahrer.h"
// history, since no longer support for drivables
//#include "../ifc/route_block_tester_t.h"
static const void * block_tester=NULL;

#include "../tpl/prioqueue_tpl.h"

#include "loadsave.h"
#include "route.h"
#include "umgebung.h"

#include "../tpl/vector_tpl.h"
#include "../tpl/array_tpl.h"


// dies wird fuer die routenplanung benoetigt

class KNode {
private:

public:
    KNode * link;
    uint16  dist;
    uint32  total;
    koord3d pos;

    inline bool operator < (const KNode &k) const {
  return dist+total <= k.dist+k.total;
    };
};

route_t::route_t() : route(0)
{
}

void
route_t::kopiere(const route_t *r)
{
	assert(r != NULL);
	const unsigned int hops = r->gib_max_n()+1;
	route.resize(hops);
	for( unsigned int i=0;  i<=hops;  i++ ) {
		route.append(r->route.at(i));
	}
}

void
route_t::insert(koord3d k)
{
	if(route.get_size()==route.get_count()) {
		route.resize(route.get_size()+1u);
	}
//DBG_MESSAGE("route_t::insert()","insert %d,%d",k.x, k.y);
	route.insert_at(0,k);
}




static inline bool am_i_there(karte_t *welt,
            koord3d pos,
            koord3d ziel,
            bool block_test)
{
  bool ok = false;

  if(block_test) {
    halthandle_t halt1 = haltestelle_t::gib_halt(welt, pos);
    halthandle_t halt2 = haltestelle_t::gib_halt(welt, ziel);

    ok = halt1.is_bound() ? (halt1 == halt2) : (pos == ziel);
  }
  else {
    ok = (pos == ziel);
  }

  return ok;
}


bool
route_t::intern_calc_route(const koord3d ziel, const koord3d start,
                           fahrer_t *fahr)
{
	const int MAX_STEP = umgebung_t::max_route_steps;	// may need very much memory => configurable
	static KNode *nodes = NULL;
	if(nodes==NULL) {
		nodes = new KNode[MAX_STEP+4+1];
	}
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
	KNode *tmp = &(nodes[step++]);

	tmp->link = NULL;
	tmp->dist = is_ship ? ABS(start.x-ziel.x) + ABS(start.y-ziel.y) : 0;
	tmp->total = 0;
	tmp->pos = start;

	queue.insert( tmp );        // init queue mit erstem feld
	welt->unmarkiere_alle();
	welt->markiere(tmp->pos);  // betretene Felder markieren

	// Breitensuche
	// always sucessful, if there is a route and enough memory ... this may be a problem with ships!

	// Hajo: moved out of loop to optimize
	const weg_t::typ wegtyp = fahr->gib_wegtyp();
	koord3d k_next;
	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *to;

//DBG_MESSAGE("route_t::itern_calc_route()","calc route from %d,%d to %d,%d",ziel.x, ziel.y, start.x, start.y);
	do {
		// Hajo: this is too expensive to be called each step
		if((step & 127) == 0) {
			INT_CHECK("route 161");
		}

		tmp = queue.pop();
		// already there?
		if(tmp->pos != ziel) {

			gr = welt->lookup(tmp->pos);
			if(gr && fahr->ist_befahrbar(gr)) {

				// testing all four possible directions
				const ribi_t::ribi ribi = gr->gib_weg_ribi(wegtyp);
				for(int r=0; r<4; r++) {

					if(  (ribi & ribi_t::nsow[r])!=0
						&& gr->get_neighbour(to, wegtyp, koord::nsow[r])
						&&   !welt->ist_markiert(k_next = to->gib_pos())
					) {

						KNode *k = &nodes[step++];

						k->link = tmp;
						k->dist = is_ship ? ABS(k_next.x-ziel.x) + ABS(k_next.y-ziel.y) : 0;
						k->total = tmp->total+1;
						k->pos = k_next;
//DBG_DEBUG("search","%i,%i",k->pos.x,k->pos.y);

						queue.insert( k );
						welt->markiere(k->pos);  // betretene Felder markieren
					}
				} //for
			}
		}
	} while(!queue.is_empty()  && !am_i_there(welt, tmp->pos, ziel, false)  &&  step<MAX_STEP);

    INT_CHECK("route 194");

	route.clear();

	bool ok = false;
//DBG_DEBUG("reached","%i,%i",tmp->pos.x,tmp->pos.y);
	// target reached?
	if(!am_i_there(welt, tmp->pos, ziel, false)  || step >= MAX_STEP  ||  tmp->link==NULL) {
		dbg->warning("route_t::intern_calc_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,MAX_STEP);
	}
	else {
		// reached => construct route
		route.resize(tmp->total+10);
		while(tmp != NULL) {
			route.append( tmp->pos );
//DBG_DEBUG("add","%i,%i",tmp->pos.x,tmp->pos.y);
			tmp = tmp->link;
		}
		ok = true;
    }

    return ok;
}



/* searches route, uses intern_calc_route() for distance between stations
 * handles only driving in stations by itself
 * corrected 12/2005 for station search
 * @author Hansjörg Malthaner, prissi
 */
bool
route_t::calc_route(karte_t *w,
                    const koord3d ziel, const koord3d start,
                    fahrer_t *fahr)
{
	welt = w;
	block_tester = 0;

	INT_CHECK("route 336");

	intern_calc_route(ziel, start, fahr);

	INT_CHECK("route 343");

	welt->unmarkiere_alle();

	INT_CHECK("route 347");

	if( route.get_count()==0 ) {
DBG_MESSAGE("route_t::calc_route()","No route from %d,%d to %d,%d found",start.x, start.y, ziel.x, ziel.y);
		// no route found
		route.resize(2);
		route.append(start);	// just to be save
		route.append(start);
		return false;

	}
	else {
		halthandle_t halt = welt->lookup(start)->gib_halt();

		// only needed for stations: go to the very end
		if(halt.is_bound()) {

			if(welt->lookup(start)->gib_weg(weg_t::schiene) != NULL) {

				int max_n = route.get_count()-1;

				const koord zv = route.at(max_n).gib_2d() - route.at(max_n-1).gib_2d();
//DBG_DEBUG("zv","%i,%i",zv.x,zv.y);

				const int ribi = fahr->gib_ribi(welt->lookup(start));
				koord3d k = start;
				koord3d k2 = start + zv;
				const grund_t *gr;

				while((gr=welt->lookup(k2))!=NULL  &&
						gr->gib_halt() == halt &&
						gr->gib_weg(weg_t::schiene) != NULL &&
						(fahr->gib_ribi(gr) & ribi)  &&
						!ribi_t::ist_einfach(fahr->gib_ribi(welt->lookup(k)))  &&
						fahr->ist_befahrbar(gr)  )  // stop at end of track! (prissi)
				{
					k = k2;
					k2 += zv;
					if(route.get_size()==route.get_count()) {
						route.resize(route.get_size()+24);	// should be ok for the longest trains
					}
//DBG_DEBUG("add station","%i,%i",k.x,k.y);
					route.append(k);
				}

			}
		}
//DBG_DEBUG("route_t::calc_route()","calc route from %d,%d to %d,%d with %i",start.x, start.y, ziel.x, ziel.y, route.get_count()-2 );
	}
	return true;
}




void
route_t::rdwr(loadsave_t *file)
{
	int max_n = route.get_count()-1;
	int i;

	file->rdwr_int(max_n, "\n");
	if(file->is_loading()) {
		koord3d k;
		route.clear();
		route.resize(max_n+2);
		for(i=0;  i<=max_n;  i++ ) {
			k.rdwr(file);
			route.append( k );
		}
	}
	else {
		// writing
		for(i=0; i<=max_n; i++) {
			route.at(i).rdwr( file );
		}
	}
}



#if 0
// prissi: apparently not used
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
  route[0] = start;   // muss Vehikel spaeter entfernen
  route[1] = start;               // koennen
  max_n = -1;

  DBG_MESSAGE("route_t::calc_unblocked_route()",
         "No route from %d,%d to %d,%d found",
         start.x, start.y, ziel.x, ziel.y);
    } else {
  //  printf("Route mit %d Schritten gefunden.\n",max_n);

        if(max_n < 511)
     route[max_n+1] = route[max_n];
    }
    return ok;
}

// ??? not used
bool route_t::find_path(karte_t * welt, const koord3d start, fahrer_t * fahr, ding_t::typ typ)
{
  block_tester = 0;
  const int MAX_STEP = 10000;
  KNode nodes[MAX_STEP+4+1];
  int step = 0;

  prioqueue_tpl <KNode *> queue;
  DBG_MESSAGE("path_t::find_path()", "start:\t%d,%d", start.x, start.y);

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
    route[0] = start;   // muss Vehikel spaeter entfernen
    route[1] = start;               // koennen
    max_n = -1;
    DBG_MESSAGE("route_t::find_path()", "No route from %d,%d to a depot found",
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
#endif
