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


#include "loadsave.h"
#include "route.h"
#include "umgebung.h"

#include "../tpl/vector_tpl.h"
#include "../tpl/array_tpl.h"

// use this for priorityques:
// NOT RECOMMENDED: search is not faster for all map sizes, and paths are longer ...
//#define PRIOR


#ifdef PRIOR

#include "../tpl/prioqueue_tpl.h"
#define Node KNode

// dies wird fuer die routenplanung benoetigt
class KNode {

public:
    KNode * parent;
    grund_t *gr;
    uint32  f, g;
    uint8 dir;

    inline bool operator < (const KNode &k) const { return (f<=k.f); }
};
#else
#define Node ANode

#endif



route_t::route_t() : route(0)
{
}



void
route_t::kopiere(const route_t *r)
{
	assert(r != NULL);
	const unsigned int hops = r->gib_max_n();
	route.resize(hops+1);
	route.clear();
	for( unsigned int i=0;  i<=hops;  i++ ) {
		route.append(r->route.at(i));
	}
}

void
route_t::append(const route_t *r)
{
	assert(r != NULL);
	const unsigned int hops = r->gib_max_n();
	route.resize(hops+1+route.get_count());

DBG_MESSAGE("route_t::append()","last=%i,%i,%i   first=%i,%i,%i",
	route.at(gib_max_n()).x, route.at(gib_max_n()).y, route.at(gib_max_n()).z,
	r->position_bei(0).x, r->position_bei(0).y, r->position_bei(0).z );

	// must be identical
//	assert(gib_max_n()<=0  ||  r->position_bei(0)==route.at(gib_max_n()));

	while(gib_max_n()>=0  &&  route.get(gib_max_n())==r->position_bei(0)) {
		// skip identical end tiles
		route.remove_at(gib_max_n());
	}
	// then append
	for( unsigned int i=0;  i<=hops;  i++ ) {
		route.append(r->position_bei(i),16);
	}
}

void
route_t::insert(koord3d k)
{
	if(route.get_size()>=route.get_count()-1) {
		route.resize(route.get_size()+16);
	}
//DBG_MESSAGE("route_t::insert()","insert %d,%d",k.x, k.y);
	route.insert_at(0,k);
}

void
route_t::append(koord3d k)
{
	while(route.get_count()>1  &&  route.get(route.get_count()-1)==route.get(route.get_count()-2)) {
		route.remove_at(route.get_count()-1);
	}
	if(route.get_count()==0  ||  k!=route.get(route.get_count()-1)) {
		route.append(k,16);
	}
	route.append(k,16);	// the last is always double
}


void
route_t::remove_koord_from(int i) {
	while(i<gib_max_n()) {
		route.remove_at(gib_max_n());
	}
	route.append( route.at(gib_max_n()) );	// the last is always double
}



/**
 * Appends a straight line from the last koord3d in route to the desired target.
 * Will return false if failed
 * @author prissi
 */
bool
route_t::append_straight_route(karte_t *welt, koord3d dest )
{
	if(route.get_count()<=1  ||  !welt->ist_in_kartengrenzen(dest.gib_2d())) {
		return false;
	}

	while(route.get_count()>1  &&  route.get(route.get_count()-2)==route.get(route.get_count()-1)) {
		route.remove_at(route.get_count()-1);
	}

	// then try to calculate direct route
	koord pos=route.get(gib_max_n()).gib_2d();
	const koord ziel=dest.gib_2d();
	route.resize( route.get_count()+abs_distance(pos,ziel)+2 );
	while(pos!=ziel) {
		// shortest way
		koord diff;
		if(abs(pos.x-ziel.x)>=abs(pos.y-ziel.y)) {
			diff = (pos.x>ziel.x) ? koord(-1,0) : koord(1,0);
		}
		else {
			diff = (pos.y>ziel.y) ? koord(0,-1) : koord(0,1);
		}
		pos += diff;
		if(!welt->ist_in_kartengrenzen(pos)) {
			break;
		}
		route.append( welt->lookup(pos)->gib_kartenboden()->gib_pos(),16 );
	}
	route.append( route.get(route.get_count()-1),16 );
	DBG_MESSAGE("route_t::append_straight_route()","to (%i,%i) found.",ziel.x,ziel.y);

	return pos==ziel;
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




// node arrays
route_t::nodestruct* route_t::nodes=NULL;
uint32 route_t::MAX_STEP=0;
#ifdef DEBUG
bool route_t::node_in_use=false;
#endif

/* find the route to an unknow location
 * @author prissi
 */
bool
route_t::find_route(karte_t *welt,
                    const koord3d start,
                    fahrer_t *fahr, const uint32 /*max_khm*/, uint8 start_dir, uint32 max_depth )
{
	bool ok = false;

	// check for existing koordinates
	if(welt->lookup(start)==NULL) {
		return false;
	}

	// some thing for the search
	const weg_t::typ wegtyp = fahr->gib_wegtyp();
	const grund_t *gr;
	grund_t *to;

	// memory in static list ...
	if(nodes==NULL) {
		MAX_STEP = umgebung_t::max_route_steps;
		nodes = (ANode *)malloc( sizeof(Node)*MAX_STEP );
	}

	INT_CHECK("route 347");

	// arrays for A*
	static vector_tpl <ANode *> open = vector_tpl <ANode *>(0);
	vector_tpl <ANode *> close =vector_tpl <ANode *>(0);

	// nothing in lists
	open.clear();
	close.clear();	// close list may be short than mark/unmark (hopefully)

	// we clear it here probably twice: does not hurt ...
	route.clear();

	// first tile is not valid?!?
	if(!fahr->ist_befahrbar(welt->lookup(start))) {
		return false;
	}

	GET_NODE();

	uint32 step = 0;
	ANode *tmp = &(nodes[step++]);
	tmp->parent = NULL;
	tmp->gr = welt->lookup(start);

	// start in open
	open.append(tmp,256);

//DBG_MESSAGE("route_t::find_route()","calc route from %d,%d,%d",start.x, start.y, start.z);
	do {
		// Hajo: this is too expensive to be called each step
		if((step & 127) == 0) {
			INT_CHECK("route 161");
		}

		tmp = open.at( 0 );
		open.remove_at( 0 );

		close.append(tmp,16);
		gr = tmp->gr;

//DBG_DEBUG("add to close","(%i,%i,%i) f=%i",gr->gib_pos().x,gr->gib_pos().y,gr->gib_pos().z,tmp->f);
		// already there
		if(fahr->ist_ziel(gr,tmp->parent==NULL?NULL:tmp->parent->gr)) {
			// we added a target to the closed list: we are finished
			break;
		}

		// testing all four possible directions
		// (since this goes the other way round compared to normal search, we must eventually invert the mask)
		const weg_t *w=gr->gib_weg(wegtyp);
		ribi_t::ribi ribi = w->gib_ribi_unmasked();
		if(w  &&  w->gib_ribi_maske()!=ribi_t::keine) {
			ribi &= (~ribi_t::rueckwaerts(w->gib_ribi_maske()));
		}

		for(int r=0; r<4; r++) {

			// a way goes here, and it is not marked (i.e. in the closed list)
			if(  (ribi & ribi_t::nsow[r] & start_dir)!=0  // allowed dir (we can restrict the first step by start_dir)
				&& abs_distance(start.gib_2d(),gr->gib_pos().gib_2d()+koord::nsow[r])<max_depth	// not too far away
				&& gr->get_neighbour(to, wegtyp/*weg_t::invalid*/, koord::nsow[r])  // is connected
				&& fahr->ist_befahrbar(to)	// can be driven on
			) {
				unsigned index;

				// already in open list?
				for(  index=0;  index<open.get_count();  index++  ) {
					if(  open.get(index)->gr==to  ) {
						break;
					}
				}
				// in open list => ignore this
				if(index<open.get_count()) {
					continue;
				}


				// already in closed list (i.e. all processed nodes)
				for( index=0;  index<close.get_count();  index++  ) {
					if(  close.get(index)->gr==to  ) {
						break;
					}
				}
				// in close list => ignore this
				if(index<close.get_count()) {
					continue;
				}

				// not in there or taken out => add new
				ANode *k=&(nodes[step++]);

				k->parent = tmp;
				k->gr = to;

//DBG_DEBUG("insert to open","%i,%i,%i",to->gib_pos().x,to->gib_pos().y,to->gib_pos().z);
				// insert here
				open.append(k,16);

			}
		}

		// ok, now no more restrains
		start_dir = ribi_t::alle;

	} while(open.get_count()>0  &&  step<MAX_STEP  &&  open.get_count()<max_depth);

	INT_CHECK("route 194");

//DBG_DEBUG("reached","");
	// target reached?
	if(!fahr->ist_ziel(gr,tmp->parent==NULL?NULL:tmp->parent->gr)  ||  step >= MAX_STEP) {
		dbg->warning("route_t::find_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,MAX_STEP);
	}
	else {
		// reached => construct route
		route.insert_at( 0, tmp->gr->gib_pos() );
		while(tmp != NULL) {
			route.insert_at( 0, tmp->gr->gib_pos() );
//DBG_DEBUG("add","%i,%i",tmp->gr->gib_pos().x,tmp->gr->gib_pos().y);
			tmp = tmp->parent;
		}
		ok = true;
    }

	RELEASE_NODE();
	return ok;
}



ribi_t::ribi *
get_next_dirs(koord gr_pos, koord ziel)
{
	static ribi_t::ribi next_ribi[4];
	if( abs(gr_pos.x-ziel.x)>abs(gr_pos.y-ziel.y) ) {
		next_ribi[0] = (ziel.x>gr_pos.x) ? ribi_t::ost : ribi_t::west;
		next_ribi[1] = (ziel.y>gr_pos.y) ? ribi_t::sued : ribi_t::nord;
	}
	else {
		next_ribi[0] = (ziel.y>gr_pos.y) ? ribi_t::sued : ribi_t::nord;
		next_ribi[1] = (ziel.x>gr_pos.x) ? ribi_t::ost : ribi_t::west;
	}
	next_ribi[2] = ribi_t::rueckwaerts( next_ribi[1] );
	next_ribi[3] = ribi_t::rueckwaerts( next_ribi[0] );
	return next_ribi;
}



bool
route_t::intern_calc_route(karte_t *welt, const koord3d ziel, const koord3d start, fahrer_t *fahr, const uint32 max_speed, const uint32 max_cost)
{
	bool ok = false;

	// check for existing koordinates
	const grund_t *gr=welt->lookup(start);
	if(gr==NULL  ||  welt->lookup(ziel)==NULL) {
		return false;
	}

	// some thing for the search
	const weg_t::typ wegtyp = fahr->gib_wegtyp();
	grund_t *to;

	// memory in static list ...
	if(nodes==NULL) {
		MAX_STEP = umgebung_t::max_route_steps;	// may need very much memory => configurable
		nodes = (ANode *)malloc( sizeof(ANode)*(MAX_STEP+4+2) );
	}

	INT_CHECK("route 347");

#ifdef PRIOR
    // Warteschlange fuer Breitensuche
    // Kann statisch sein, weil niemals zwei Fahrzuege zugleich eine route
    // suchen ... statisch bedeutet wiedernutzung bereits erzeugter nodes
    // und ist in diesem Fall effizienter
    static prioqueue_tpl <KNode *> queue;
#else
	// arrays for A*
	// (was vector_tpl before, but since we really badly need speed here, we use unchecked arrays)
	static ANode **open;
#endif
	static uint32 open_size=0, open_count;

	const bool is_airplane = fahr->gib_wegtyp()==weg_t::luft;

	// we clear it here probably twice: does not hurt ...
	route.clear();

	// first tile is not valid?!?
	if(!fahr->ist_befahrbar(gr)) {
		return false;
	}

	GET_NODE();

	uint32 step = 1;
	Node *tmp =(Node *)&(nodes[0]);

	tmp->parent = NULL;
	tmp->gr = welt->lookup(start);
	tmp->f = calc_distance(start,ziel);
	tmp->g = 0;
	tmp->dir = 0;

	// nothing in lists
	welt->unmarkiere_alle();

#ifdef PRIOR
DBG_DEBUG("sizes","KNode=%i, ANode=%i",sizeof(KNode),sizeof(ANode));
    // da die queue statisch ist müssen wir die reste der alten suche
    // erst aufräumen
    queue.clear();
    queue.insert(tmp);
#else
	if(open_size==0) {
		open_size = 4096;
		open = (ANode **)malloc( sizeof(ANode*)*4096 );
	}

	// start in open
	open[0] = tmp;
	open_count = 1;
#endif

//DBG_MESSAGE("route_t::itern_calc_route()","calc route from %d,%d,%d to %d,%d,%d",ziel.x, ziel.y, ziel.z, start.x, start.y, start.z);
	uint32 beat=1;
	do {
		// Hajo: this is too expensive to be called each step
		if((beat++ & 255) == 0) {
			INT_CHECK("route 161");
		}

#ifdef PRIOR
		Node *test_tmp = queue.pop();
		tmp = test_tmp;
#else
		open_count --;

		if(welt->ist_markiert(open[open_count]->gr)) {
#if 0
			// well we may have had already a better one going here ...
			// gives a 20% gain ...
			Node *check=tmp;
			uint32 this_f=open[open_count]->f+25;
			const grund_t *this_gr=open[open_count]->gr;
			while(check->parent!=NULL  &&  check->f>this_f  &&  check->gr!=this_gr) {
				check = check->parent;
			}
			// this new one is worse?
			if(check->gr==this_gr) {
				continue;
			}
#else
			// this is not 100% optimal, but is about two to three times faster ...
			continue;
#endif
		}

		tmp = open[open_count];
#endif

		gr = tmp->gr;
		welt->markiere(gr);

//DBG_DEBUG("add to close","(%i,%i,%i) f=%i",gr->gib_pos().x,gr->gib_pos().y,gr->gib_pos().z,tmp->f);

		// already there
		if(gr->gib_pos() == ziel) {
			// we added a target to the closed list: we are finished
			break;
		}

		// testing all four possible directions
		const ribi_t::ribi ribi =  fahr->gib_ribi(gr);
		const ribi_t::ribi *next_ribi = get_next_dirs(gr->gib_pos().gib_2d(),ziel.gib_2d());
		for(int r=0; r<4; r++) {

			// a way in our direction?
			if(  (ribi & next_ribi[r])==0  ) {
				continue;
			}

			to = NULL;
			if(is_airplane) {
				const planquadrat_t *pl=welt->lookup(gr->gib_pos().gib_2d()+koord(next_ribi[r]));
				if(pl) {
					to = pl->gib_kartenboden();
				}
			}

			// a way goes here, and it is not marked (i.e. in the closed list)
			if((to  ||  gr->get_neighbour(to, wegtyp, koord(next_ribi[r]) ))  &&  fahr->ist_befahrbar(to)  &&  !welt->ist_markiert(to)) {

				// new values for cost g
				uint32 new_g = tmp->g + fahr->gib_kosten(to,max_speed);

				// check for curves (usually, one would need the lastlast and the last;
				// if not there, then we could just take the last
				uint8 current_dir;
				if(tmp->parent!=NULL) {
					current_dir = ribi_typ( tmp->parent->gr->gib_pos().gib_2d(), to->gib_pos().gib_2d() );
					if(tmp->dir!=current_dir) {
						new_g += 3;
						if(tmp->parent->dir!=tmp->dir) {
							// discourage 90° turns
							new_g += 10;
						}
						else if(ribi_t::ist_exakt_orthogonal(tmp->dir,current_dir)) {
							// discourage v turns heavily
							new_g += 25;
						}
					}

				}
				else {
					 current_dir = ribi_typ( gr->gib_pos().gib_2d(), to->gib_pos().gib_2d() );
				}

				const uint32 new_f = new_g + calc_distance( to->gib_pos(), ziel );

#ifdef PRIOR
				// not in there or taken out => add new
				KNode *k=(KNode *)(nodes+step);
				step ++;

				k->parent = tmp;
				k->gr = to;
				k->g = new_g;
				k->f = new_f;
				k->dir = current_dir;

				queue.insert( k );
//DBG_DEBUG("insert to open","%i,%i,%i",to->gib_pos().x,to->gib_pos().y,to->gib_pos().z);
#else
				uint32 index = open_count;
				// insert with binary search, ignore doublettes
				if(open_count>0  &&  new_f>open[open_count-1]->f) {//  || (new_f==open[open_count-1]->f  &&  new_g<=open[open_count-1]->g))) {
					sint32 high = open_count, low = -1, probe;
					while(high - low>1) {
						probe = ((uint32) (low + high)) >> 1;
						if(open[probe]->f==new_f) {
							low = probe;
							break;
						}
						else if(open[probe]->f>new_f) {
							low = probe;
						}
						else {
							high = probe;
						}
					}
					// we want to insert before, so we may add 1
					index = 0;
					if(low>=0) {
						index = low;
						if(open[index]->f>=new_f) {
							index ++;
						}
					}
//DBG_MESSAGE("bsort","current=%i f=%i  f+1=%i",new_f,open[index]->f,open[index+1]->f);
				}

				// not in there or taken out => add new
				ANode *k=&(nodes[step]);
				step++;

				k->parent = tmp;
				k->gr = to;
				k->g = new_g;
				k->f = new_f;
				k->dir = current_dir;

				// need to enlarge?
				if(open_count==open_size) {
					ANode **tmp=open;
					open_size += 4096;
					open = (ANode **)malloc( sizeof(ANode*)*open_size );
					memcpy( open, tmp, sizeof(ANode*)*(open_size-4096) );
					free( tmp );
				}

				if(index<open_count) {
					// was not best f so far => insert
					memmove( open+index+1ul, open+index, sizeof(ANode*)*(open_count-index) );
				}
				open[index] = k;
				open_count ++;
#endif
//DBG_DEBUG("insert to open","(%i,%i,%i)  f=%i at %i",to->gib_pos().x,to->gib_pos().y,to->gib_pos().z,k->f, index);
			}
		}
#ifdef PRIOR
		open_count = queue.count();
#endif
	} while(open_count>0  &&  !am_i_there(welt, gr->gib_pos(), ziel, false)  &&  step<MAX_STEP  &&  tmp->g<max_cost);

	INT_CHECK("route 194");
#ifdef DEBUG_ROUTES
DBG_DEBUG("route_t::intern_calc_route()","steps=%i  (max %i) in route, open %i, cost %u (max %u)",step,MAX_STEP,open_count,tmp->g,max_cost);
#endif
	// target reached?
	if(!am_i_there(welt, tmp->gr->gib_pos(), ziel, false)  || step >= MAX_STEP  ||  tmp->parent==NULL) {
		dbg->warning("route_t::intern_calc_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,MAX_STEP);
	}
	else {
		// reached => construct route
		while(tmp != NULL) {
			route.append( tmp->gr->gib_pos(), 256 );
//DBG_DEBUG("add","%i,%i",tmp->pos.x,tmp->pos.y);
			tmp = tmp->parent;
		}
		ok = true;
    }

	RELEASE_NODE();

	return ok;
}



/* searches route, uses intern_calc_route() for distance between stations
 * handles only driving in stations by itself
 * corrected 12/2005 for station search
 * @author Hansjörg Malthaner, prissi
 */
bool
route_t::calc_route(karte_t *welt,
                    const koord3d ziel, const koord3d start,
                    fahrer_t *fahr, const uint32 max_khm, const uint32 max_cost)
{
	block_tester = 0;
	route.clear();

	INT_CHECK("route 336");

#ifdef DEBUG_ROUTES
	// profiling for routes ...
	long ms=get_current_time_millis();
	bool ok = intern_calc_route(welt, ziel, start, fahr, max_khm,max_cost);
	if(fahr->gib_wegtyp()==weg_t::wasser) {DBG_DEBUG("route_t::calc_route()","route from %d,%d to %d,%d with %i steps in %u ms found.",start.x, start.y, ziel.x, ziel.y, route.get_count()-2, get_current_time_millis()-ms );}
#else
	bool ok = intern_calc_route(welt, ziel, start, fahr, max_khm,max_cost);
#endif

	INT_CHECK("route 343");

	if( !ok ) {
DBG_MESSAGE("route_t::calc_route()","No route from %d,%d to %d,%d found",start.x, start.y, ziel.x, ziel.y);
		// no route found
		route.resize(1);
		route.append(start);	// just to be save
		return false;

	}
	else {
		halthandle_t halt = welt->lookup(start)->gib_halt();

		// only needed for stations: go to the very end
		if(halt.is_bound()) {

			// does only make sence for trains
			if(fahr->gib_wegtyp()==weg_t::schiene  ||  fahr->gib_wegtyp()==weg_t::monorail  ||  fahr->gib_wegtyp()==weg_t::schiene_strab) {

				int max_n = route.get_count()-1;

				const koord zv = route.at(max_n).gib_2d() - route.at(max_n-1).gib_2d();
//DBG_DEBUG("route_t::calc_route()","zv=%i,%i",zv.x,zv.y);

				const int ribi = ribi_typ(zv);//fahr->gib_ribi(welt->lookup(start));
				grund_t *gr=welt->lookup(start);

				while(gr->get_neighbour(gr,fahr->gib_wegtyp(),zv)  &&  gr->gib_halt() == halt  &&   fahr->ist_befahrbar(gr)   &&  (fahr->gib_ribi(gr)&&ribi)!=0) {
					// stop at end of track! (prissi)
//DBG_DEBUG("route_t::calc_route()","add station at %i,%i",gr->gib_pos().x,gr->gib_pos().y);
					route.append(gr->gib_pos(),12);
				}

			}
		}
	}
	return true;
}




void
route_t::rdwr(loadsave_t *file)
{
	int max_n = route.get_count()-1;
	int i;

	file->rdwr_long(max_n, "\n");
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
