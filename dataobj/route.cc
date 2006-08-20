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
	if(route.get_size()==route.get_count()) {
		route.resize(route.get_size()+1u);
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
	route.append(k,16);	// the last is always double
	route.append(k,16);
}


void
route_t::remove_koord_from(int i) {
	while(i<gib_max_n()) {
		route.remove_at(gib_max_n());
	}
	route.append( route.at(gib_max_n()) );	// the last is always double
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




struct ANode {
    ANode *parent;
    const grund_t *gr;
    uint32  f,g;
    uint8 dir;
};

static struct ANode *nodes = NULL;

static inline uint32 calc_distance( const koord3d p1, const koord3d p2 )
{
	return (abs(p1.x-p2.x)+abs(p1.y-p2.y)+abs(p1.z-p2.z)/16)*4;
}


#if 0
/* find the route to an unknow location
 * Astar does not work here
 * @author prissi
 */
bool
route_t::find_route_astar(karte_t *welt,
                    const koord3d start,
                    fahrer_t *fahr, const uint32 max_khm )
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
	const int MAX_STEP = max(65530,umgebung_t::max_route_steps);	// may need very much memory => configurable
	if(nodes==NULL) {
		nodes = new ANode[MAX_STEP+4+1];
	}
	int step = 0;

//	welt->unmarkiere_alle();	// test in closed list are likely faster ...
	INT_CHECK("route 347");

	// arrays for A*
	static vector_tpl <struct ANode *> open = vector_tpl <struct ANode *>(0);
	vector_tpl <struct ANode *> close =vector_tpl <struct ANode *>(0);

	// nothing in lists
	open.clear();
	close.clear();

	// we clear it here probably twice: does not hurt ...
	route.clear();

	struct ANode *tmp = &(nodes[step++]);
	tmp->parent = NULL;
	tmp->gr = welt->lookup(start);
	tmp->f = 0;
	tmp->g = 0;
	tmp->dir = 0;

	// first tile is not valid?!?
	if(!fahr->ist_befahrbar(tmp->gr)) {
		return false;
	}

	// start in open
	open.append(tmp,256);

//DBG_MESSAGE("route_t::find_route()","calc route from %d,%d,%d",start.x, start.y, start.z);
	do {
		// Hajo: this is too expensive to be called each step
		if((step & 127) == 0) {
			INT_CHECK("route 161");
		}

		tmp = open.at( open.get_count()-1 );
		open.remove_at( open.get_count()-1 );

		close.append(tmp,16);
		gr = tmp->gr;

//DBG_DEBUG("add to close","(%i,%i,%i) f=%i",gr->gib_pos().x,gr->gib_pos().y,gr->gib_pos().z,tmp->f);
		// already there
		if(fahr->ist_ziel(gr)) {
			// we added a target to the closed list: we are finished
			break;
		}

		// testing all four possible directions (since this goes the other way round compared to normal search, we must eventually invert)
		ribi_t::ribi ribi = fahr->gib_ribi(gr);
		if(ribi_t::alle!=ribi) {
			ribi = ribi_t::rueckwaerts(ribi);
		}

		for(int r=0; r<4; r++) {

			// a way goes here, and it is not marked (i.e. in the closed list)
			if(  (ribi & ribi_t::nsow[r])!=0
				&& gr->get_neighbour(to, wegtyp, koord::nsow[r])
				&& fahr->ist_befahrbar(to)
			) {

				// already in closed list (i.e. all processed nodes
				unsigned index;
				for( index=0;  index<close.get_count();  index++  ) {
					if(  close.get(index)->gr==to  ) {
						break;
					}
				}
				// in close list => ignore this
				if(index<close.get_count()) {
					continue;
				}

				// already in open list ?
				for(  index=0;  index<open.get_count();  index++  ) {
					if(  open.get(index)->gr==gr  ) {
						break;
					}
				}

				// new values for cost g
				uint32 new_g = tmp->g;

				// first favor faster ways
				const weg_t *w=to->gib_weg(wegtyp);
				uint32 max_tile_speed = w ? w->gib_max_speed() : 999;
				// add cost for going (with maximum spoeed, cost is 1 ...
				new_g += ( (max_khm<=max_tile_speed) ? 4 :  (max_khm*4+max_tile_speed)/max_tile_speed );

				// malus for occupied tiles
				if(wegtyp==weg_t::strasse) {
					// assume all traffic (and even road signs etc.) is not good ...
					new_g += to->obj_count();
				}

				// check for curves (usually, one would need the lastlast and the last;
				// if not there, then we could just take the last
				uint8 current_dir;
				if(tmp->parent!=NULL) {
					current_dir = ribi_typ( tmp->parent->gr->gib_pos().gib_2d(), to->gib_pos().gib_2d() );
					if(tmp->dir!=current_dir) {
						new_g += 5;
						if(tmp->parent->dir!=tmp->dir) {
							// discourage double turns
							new_g += 10;
						}
					}
				}
				else {
					 current_dir = ribi_typ( gr->gib_pos().gib_2d(), to->gib_pos().gib_2d() );
				}

				// effect of hang ...
				if(to->gib_weg_hang()!=0) {
					// we do not need to check whether up or down, since everything that goes up must go down
					// thus just add whenever there is a slope ... (counts as nearly 2 standard tiles)
					new_g += 7;
				}

				// it is already contained in the list
				if(index<open.get_count()) {
//DBG_DEBUG("check open","at %i",index);
					// => check, if our is better
					if(open.get(index)->g>new_g) {
						// our is better
						open.remove_at(index);
//DBG_DEBUG("remove from open","%i,%i,%i",to->gib_pos().x,to->gib_pos().y,to->gib_pos().z);
					}
					else {
						// ignore this one
						continue;
					}
				}

				// not in there or taken out => add new
				struct ANode *k=&(nodes[step++]);

				k->parent = tmp;
				k->gr = to;
				k->g = new_g;
				k->f = new_g+1;
				k->dir = current_dir;

				// insert sorted
				for(  index=0;  index<open.get_count();  index++  ) {
					if(k->f>open.get(index)->f) {
						// insert here
						break;
					}
				}

//DBG_DEBUG("insert to open","%i,%i,%i",to->gib_pos().x,to->gib_pos().y,to->gib_pos().z);
				// insert here
				open.insert_at(index,k);

			}
		}
	} while(open.get_count()>0  &&  step<MAX_STEP);

	INT_CHECK("route 194");

DBG_DEBUG("reached","");
	// target reached?
	if(!fahr->ist_ziel(gr)  ||  step >= MAX_STEP  ||  tmp->parent==NULL) {
		dbg->warning("route_t::find_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,MAX_STEP);
	}
	else {
		// reached => construct route
		route.insert_at( 0, tmp->gr->gib_pos() );
		while(tmp != NULL) {
			route.insert_at( 0, tmp->gr->gib_pos() );
DBG_DEBUG("add","%i,%i",tmp->gr->gib_pos().x,tmp->gr->gib_pos().y);
			tmp = tmp->parent;
		}
		ok = true;
    }

    return ok;
}
#endif


/* find the route to an unknow location
 * @author prissi
 */
bool
route_t::find_route(karte_t *welt,
                    const koord3d start,
                    fahrer_t *fahr, const uint32 max_khm, uint8 start_dir, uint32 max_depth )
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
	const int MAX_STEP = max(65530,umgebung_t::max_route_steps);	// may need very much memory => configurable
	if(nodes==NULL) {
		nodes = new ANode[MAX_STEP+4+1];
	}
	int step = 0;

//	welt->unmarkiere_alle();	// test in closed list are likely faster ...
	INT_CHECK("route 347");

	// arrays for A*
	static vector_tpl <struct ANode *> open = vector_tpl <struct ANode *>(0);
	vector_tpl <struct ANode *> close =vector_tpl <struct ANode *>(0);

	// nothing in lists
	open.clear();
	close.clear();

	// we clear it here probably twice: does not hurt ...
	route.clear();

	struct ANode *tmp = &(nodes[step++]);
	tmp->parent = NULL;
	tmp->gr = welt->lookup(start);

	// first tile is not valid?!?
	if(!fahr->ist_befahrbar(tmp->gr)) {
		return false;
	}

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
		if(fahr->ist_ziel(gr)) {
			// we added a target to the closed list: we are finished
			break;
		}

		// testing all four possible directions
		// (since this goes the other way round compared to normal search, we must eventually invert the mask)
		ribi_t::ribi ribi = fahr->gib_ribi(gr);
		const weg_t *w=gr->gib_weg(wegtyp);
		if(w  &&  w->gib_ribi_maske()!=ribi_t::keine) {
			ribi = ribi_t::rueckwaerts(ribi);
		}

		for(int r=0; r<4; r++) {

			// a way goes here, and it is not marked (i.e. in the closed list)
			if(  (ribi & ribi_t::nsow[r] & start_dir)!=0  // allowed dir (we can restrict the first step by start_dir)
				&& abs_distance(start.gib_2d(),gr->gib_pos().gib_2d()+koord::nsow[r])<max_depth	// not too far away
				&& gr->get_neighbour(to, wegtyp, koord::nsow[r])  // is connected
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
				struct ANode *k=&(nodes[step++]);

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
	if(!fahr->ist_ziel(gr)  ||  step >= MAX_STEP) {
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

    return ok;
}




bool
route_t::intern_calc_route(const koord3d ziel, const koord3d start, fahrer_t *fahr, const uint32 max_speed)
{
	bool ok = false;

	// check for existing koordinates
	if(welt->lookup(start)==NULL  ||  welt->lookup(ziel)==NULL) {
		return false;
	}

	// some thing for the search
	const weg_t::typ wegtyp = fahr->gib_wegtyp();
	const grund_t *gr;
	grund_t *to;

	// memory in static list ...
	const int MAX_STEP = max(65530,umgebung_t::max_route_steps);	// may need very much memory => configurable
	if(nodes==NULL) {
		nodes = new ANode[MAX_STEP+4+1];
	}
	int step = 0;

//	welt->unmarkiere_alle();	// test in closed list are likely faster ...
	INT_CHECK("route 347");

	// arrays for A*
	static vector_tpl <struct ANode *> open = vector_tpl <struct ANode *>(0);

	const bool is_airplane = fahr->gib_wegtyp()==weg_t::luft;

	// nothing in lists
	open.clear();
	welt->unmarkiere_alle();

	// we clear it here probably twice: does not hurt ...
	route.clear();

	struct ANode *tmp = &(nodes[step++]);
	tmp->parent = NULL;
	tmp->gr = welt->lookup(start);
	tmp->f = calc_distance(start,ziel);
	tmp->g = 0;
	tmp->dir = 0;

	// first tile is not valid?!?
	if(!fahr->ist_befahrbar(tmp->gr)) {
		return false;
	}

	// start in open
	open.append(tmp,256);

//DBG_MESSAGE("route_t::itern_calc_route()","calc route from %d,%d,%d to %d,%d,%d",ziel.x, ziel.y, ziel.z, start.x, start.y, start.z);
	do {
		// Hajo: this is too expensive to be called each step
		if((step & 15) == 0) {
			INT_CHECK("route 161");
		}

		tmp = open.at( open.get_count()-1 );
		open.remove_at( open.get_count()-1 );

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
		for(int r=0; r<4; r++) {

			// a way in our direction?
			if(  (ribi & ribi_t::nsow[r])==0  ) {
				continue;
			}

			to = NULL;
			if(is_airplane) {
				const planquadrat_t *pl=welt->lookup(gr->gib_pos().gib_2d()+koord::nsow[r]);
				if(pl) {
					to = pl->gib_kartenboden();
				}
			}

			// a way goes here, and it is not marked (i.e. in the closed list)
			if((to  ||  gr->get_neighbour(to, wegtyp, koord::nsow[r]))  &&  fahr->ist_befahrbar(to)  &&  !welt->ist_markiert(to)) {

				// new values for cost g
				uint32 new_g = tmp->g + fahr->gib_kosten(to,max_speed);

				// check for curves (usually, one would need the lastlast and the last;
				// if not there, then we could just take the last
				uint8 current_dir;
				if(tmp->parent!=NULL) {
					current_dir = ribi_typ( tmp->parent->gr->gib_pos().gib_2d(), to->gib_pos().gib_2d() );
					if(tmp->dir!=current_dir) {
						new_g += 5;
						if(tmp->parent->dir!=tmp->dir) {
							// discourage double turns
							new_g += 10;
						}
					}
				}
				else {
					 current_dir = ribi_typ( gr->gib_pos().gib_2d(), to->gib_pos().gib_2d() );
				}

				const uint32 new_f = new_g+calc_distance( to->gib_pos(), ziel );

				// already in open list and better?
				sint32 index;
				for(  index=open.get_count()-1;  index>=0  &&   open.get(index)->f<=new_f;  index--  ) {
					if(open.get(index)->gr==gr) {
						break;
					}
				}

				if(index>=0  &&  open.get(index)->gr==gr) {
					// it is already contained in the list
					// and it is lower in f ...
					continue;
				}

				// it may or may not be in the list; but since the arrays are sorted
				// we find out about this during inserting!

				// not in there or taken out => add new
				struct ANode *k=&(nodes[step++]);

				k->parent = tmp;
				k->gr = to;
				k->g = new_g;
				k->f = new_f;
				k->dir = current_dir;

				// insert sorted
				for(  index=0;  index<open.get_count()  &&  new_f<open.get(index)->f;  index++  ) {
					if(open.get(index)->gr==to) {
						open.remove_at(index);
						index --;
					}
				}
				// was best f so far => append
				if(index>=open.get_count()) {
					open.append(k,16);
				}
				else {
					open.insert_at(index,k);
				}

//DBG_DEBUG("insert to open","(%i,%i,%i)  f=%i at %i",to->gib_pos().x,to->gib_pos().y,to->gib_pos().z,k->f, index);
			}
		}
	} while(open.get_count()>0  &&  !am_i_there(welt, gr->gib_pos(), ziel, false)  &&  step<MAX_STEP);

	INT_CHECK("route 194");

//DBG_DEBUG("reached","%i,%i",tmp->pos.x,tmp->pos.y);
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
                    fahrer_t *fahr, const uint32 max_khm)
{
	welt = w;
	block_tester = 0;
	route.clear();

	INT_CHECK("route 336");

	bool ok = intern_calc_route(ziel, start, fahr, max_khm);

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
			if(welt->lookup(start)->gib_weg(weg_t::schiene) != NULL) {

				int max_n = route.get_count()-1;

				const koord zv = route.at(max_n).gib_2d() - route.at(max_n-1).gib_2d();
//DBG_DEBUG("route_t::calc_route()","zv=%i,%i",zv.x,zv.y);

				const int ribi = ribi_typ(zv);//fahr->gib_ribi(welt->lookup(start));
				grund_t *gr=welt->lookup(start);

				while(gr->get_neighbour(gr,weg_t::schiene,zv)  &&  gr->gib_halt() == halt  &&   fahr->ist_befahrbar(gr)   &&  (fahr->gib_ribi(gr)&&ribi)!=0) {
					// stop at end of track! (prissi)
//DBG_DEBUG("route_t::calc_route()","add station at %i,%i",gr->gib_pos().x,gr->gib_pos().y);
					route.append(gr->gib_pos(),12);
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
