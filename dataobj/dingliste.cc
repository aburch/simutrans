/*
 * author V. Meyer
 * Memory handling rewritten by neroden
 */

#include <stdio.h>
#include <algorithm>

#include "../simdebug.h"
#include "../simgraph.h"
#include "../simworld.h"

#include "../bauer/hausbauer.h"
#include "../dings/dummy.h"
#include "../dings/wolke.h"
#include "../dings/zeiger.h"
#include "../dings/crossing.h"
#include "../dings/baum.h"
#include "../dings/bruecke.h"
#include "../dings/field.h"
#include "../dings/pillar.h"
#include "../dings/tunnel.h"
#include "../dings/gebaeude.h"
#include "../dings/signal.h"
#include "../dings/label.h"
#include "../dings/gebaeude.h"
#include "../dings/leitung2.h"
#include "../dings/wayobj.h"
#include "../dings/roadsign.h"
#include "../dings/groundobj.h"

#include "../simtypes.h"
#include "../simdepot.h"
#include "../simmem.h"

#include "../player/simplay.h"

#include "../vehicle/simvehikel.h"
#include "../vehicle/simverkehr.h"
#include "../vehicle/simpeople.h"
#include "../vehicle/movingobj.h"

#include "../besch/haus_besch.h"
#include "../besch/groundobj_besch.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/freelist.h"
#include "../dataobj/umgebung.h"

#include "dingliste.h"

/*
 * This will clear memory containing stale pointers.
 * Turn it off (#define CLEAR_MEMORY 0) for maximum speed.
 * It is automatically off if assertions are off, because it's
 * pointless to try to debug this file if assertions are off.
 */

#ifndef CLEAR_MEMORY
	#ifdef NDEBUG
		#define CLEAR_MEMORY 0
	#else
		#define CLEAR_MEMORY 1
	#endif
#endif

/* All things including ways are stored in this structure.
 * The entries are packed, i.e. the first free entry is at the top.
 * To save memory, a single element (like in the case for houses or a single tree)
 * is stored directly (obj.one) and capacity==0. (Yes, zero.  Capacity never equals 1.)
 * Otherwise obj.some points to an array.
 * The objects are sorted according to their drawing order.
 * ways are always first.
 */

#define baum_pri (50)
#define pillar_pri (7)

// priority of moving things: should be smaller than the priority of powerlines
#define moving_obj_pri (100)

// priority for entering into dingliste
// unused entries have 255
static uint8 type_to_pri[256]=
{
	255, //
	baum_pri, // baum
	254, // zeiger
	200, 200, 200,	// wolke
	3, 3, // buildings
	6, // signal
	2, 2, // bridge/tunnel
	255,
	1, 1, 1, // depots
	5, // smoke generator (not used any more)
	150, 4, 4, // powerlines
	6, // roadsign
	pillar_pri, // pillar
	1, 1, 1, 1, // depots (must be before tunnel!)
	8, // way objects (electrification)
	0, // ways (always at the top!)
	9, // label, indicates ownership: insert before trees
	3, // field (factory extension)
	1, // crossings, treated like bridges or tunnels
	1, // groundobjs, overlays over bare ground like lakes etc.
	1,  // narrowgaugedepot
	255, 255, 255, 255, 255, 255, 255, 255,	// 32-63 left empty (old numbers)
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	moving_obj_pri,	// pedestrians
	moving_obj_pri,	// city cars
	moving_obj_pri,	// road vehilce
	moving_obj_pri,	// rail vehicle
	moving_obj_pri,	// monorail
	moving_obj_pri,	// maglev
	moving_obj_pri,	// narrowgauge
	255, 255, 255, 255, 255, 255, 255, 255, 255,
	moving_obj_pri,	// ship
	moving_obj_pri+1,	// aircraft (no trailer, could be handled by normal method)
	moving_obj_pri+1,	// movingobject (no trailer, could be handled by normal method)
	255, 255, 255, 255, 255, 255, 255, 255,	// 83-95 left empty (for other moving stuff) 95, is reserved for old choosesignals
	255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,	// 96-128 left empty (old numbers)
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,	// 128-255 left empty
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255
};


inline static void dl_free(ding_t** p, uint8 size)
{
	assert(size > 1);
	if (size <= 16) {
		freelist_t::putback_node(sizeof(*p) * size, p);
	}
	else {
		guarded_free(p);
	}
}


inline static ding_t** dl_alloc(uint8 size)
{
	assert(size > 1);
	ding_t** p;
	if (size <= 16) {
		p = static_cast<ding_t**>(freelist_t::gimme_node(size * sizeof(*p)));
	}
	else {
		p = MALLOCN(ding_t*, size);
	}
	return p;
}


dingliste_t::dingliste_t()
{
	capacity = 0;
	top = 0;
#if CLEAR_MEMORY
	obj.one = NULL;
#endif
}


dingliste_t::~dingliste_t()
{
	assert(capacity != 1);
	if(capacity > 0) {
		dl_free(obj.some, capacity);
	}
#if CLEAR_MEMORY
	obj.some = NULL;
	capacity = 0;
	top = 0;
#endif
}


inline void dingliste_t::grow_capacity_above_one()
{
	// Should only be called in the "one" case:
	assert (capacity == 0);
	assert (top == 1);
	// When growing above 1, go directly to 4
	// Why 4?  1 is fairly common (a building or tree)
	// But for more than 1 we have ways,
	// which usually have vehicles; or we have forests.
	uint8 new_cap = 4;

	// Must convert from "one mode" to "some mode"
	ding_t* tmp=obj.one;

	obj.some = dl_alloc(new_cap);
	assert(obj.some); // otherwise, memory allocation failed, which is bad news
#if CLEAR_MEMORY
	MEMZERON(obj.some, new_cap); // clear memory purely for tidiness
#endif
	obj.some[0] = tmp;
	capacity = new_cap;
}

inline bool dingliste_t::grow_capacity()
{
	// Call this *before* increasing "top"
	// Should only be called in the "some" case:
	assert (capacity > 1);
	if(capacity>=254) {
		// capacity exceeded ... (and no need for THAT many objects here ... )
		return false;
	}
	unsigned new_cap = (unsigned) capacity * 2;
	if(new_cap>=255) {
		new_cap = 254;
	}

	// get memory
	ding_t **tmp = dl_alloc(new_cap);
	assert(tmp); // Otherwise, memory allocation failed
	// move data to new memory
	memcpy( tmp, obj.some, sizeof(ding_t*) * top );
	// free old memory
	dl_free(obj.some, capacity);
	// Link up new memory
	obj.some = tmp;

	capacity = new_cap;
	return true;
}

/**
 * Reduce capacity based on new value of "top" (which should already be set).
 * Must be called only in the "some" case.
 */
void dingliste_t::shrink_capacity()
{
	assert(top <= capacity);
	assert(capacity > 1);
	assert(obj.some);

	// Strategy:
	// (A) Always shrink if there are 0 objects left.  This is probably a completely
	//     cleared tile and will likely stay cleared or get a house or a tree.  We can
	//     free up the entire memory allocation, perhaps permanently.
	// (B) If there is 1 object left, this might be a road or track and it will probably
	//     have vehicles show up later.  Since we'll probably have to expand again, only
	//     shrink if capacity > 16 (so we're getting memory from somewhere other than the freelist).
	// (C) If there are 2-4 objects left, shrink to 4, but only if capacity > 16.  We are fairly
	//     likely to stay below 5 objects, but it's only worth reallocating if we had memory from
	//     somewhere other than the freelist.
	// (D) If there are 5 or more objects left, this is a busy tile. Don't shrink at all, we'll
	//     probably need the room later.
	// The cases where we don't shrink simply fall through to the bottom of the function.
	if ( top==0 ) {
		// Convert from "some" to "one"
		// We have nothing to save
		dl_free( obj.some, capacity );
		capacity = 0; // by convention, this means "one".
	}
	else if ( top==1 && capacity > 16 ) {
		// Convert from "some" to "one"
		// We have an object to save
		ding_t* tmp=obj.some[0];
		dl_free( obj.some, capacity );
		obj.one = tmp;
		capacity = 0;  // By convention, this means "one"
	}
	else if ( top <= 4 && capacity > 16 ) {
		// Shrink to 4.  No point in going smaller, we'll probably have to expand again.
		const uint8 new_capacity = 4;
		// get memory
		ding_t **tmp = dl_alloc(new_capacity);
#if CLEAR_MEMORY
		MEMZERON(tmp, new_capacity); // clear memory purely for tidiness
#endif
		// copy old data to new memory
		memcpy( tmp, obj.some, sizeof(ding_t *)*top );
		// free old memory
		dl_free(obj.some, capacity);
		// Hook up new pointer
		obj.some = tmp;
		// Reset capacity
		capacity = new_capacity;
	}
}



inline void dingliste_t::intern_insert_at(ding_t* ding, uint8 pri)
{
	// we have more than one object here, thus we can use obj.some exclusively!

	// Pri "inserts before" an entry so must be <= top, whic is after the last entry
	// Note that this works with top == pri (the for loop resolves to nothing)
	assert(pri <= top);
	// We are going to increase top when we're done; it is critical that we have
	// enough capacity for one plus the number currently used.
	assert(capacity > top);
	for(  uint8 i=top;  i>pri;  i--  ) {
		obj.some[i] = obj.some[i-1];
	}
	obj.some[pri] = ding;
	top++;
	assert(capacity >= top);
	consistency_check();
}



// this will automatically give the right order for citycars and the like ...
inline bool dingliste_t::intern_add_moving(ding_t* ding)
{
	// we are more than one object, thus we exclusively use obj.some here!
	// it would be nice, if also the objects are inserted according to their priorities as
	// vehicles types (number returned by get_typ()). However, this would increase
	// the calculation even further. :(

	// find out about the first car etc. moving thing.
	// We can start to insert between (start) and (end)
	uint8 start=0;
	while(start<top  &&  !obj.some[start]->is_moving()  ) {
		start ++;
	}
	uint8 end = top;
	while(  end>start  && (!obj.some[end-1] || !obj.some[end-1]->is_moving() ) ) {
		end--;
	}
	if(start==end) {
		intern_insert_at(ding, start);
		return true;
	}

	// if we have two ways, the way at index 0 is ALWAYS the road!
	// however ships and planes may be where not way is below ...
	if(start!=0  &&  obj.some[0]->get_typ()==ding_t::way  &&  ((weg_t *)obj.some[0])->get_waytype()==road_wt) {

		const uint8 fahrtrichtung = ((vehikel_basis_t*)ding)->get_fahrtrichtung();

		// this is very complicated:
		// we may have many objects in two lanes (actually five with tram and pedestrians)
		if(ding->get_welt()->get_settings().is_drive_left()) {

			// driving on left side
			if(fahrtrichtung<4) {	// nord, nordwest

				if((fahrtrichtung&(~ribi_t::suedost))==0) {
					// if we are going east we must be drawn as the first in east direction
					intern_insert_at(ding, start);
					return true;
				}
				else {
					// we must be drawn before south or west (thus insert after)
					for(uint8 i=start;  i<end;  i++  ) {
						if((((const vehikel_t*)obj.some[i])->get_fahrtrichtung()&ribi_t::suedwest)!=0) {
							intern_insert_at(ding, i);
							return true;
						}
					}
					// nothing going southwest
					intern_insert_at(ding, end);
					return true;
				}

			}
			else {
				// going south, west or the rest
				if((fahrtrichtung&(~ribi_t::suedost))==0) {
					// if we are going south or southeast we must be drawn as the first in east direction (after nord and nordeast)
					for(uint8 i=start;  i<end;  i++  ) {
						if (ding_t const* const dt = obj.some[i]) {
							if (vehikel_basis_t const* const v = ding_cast<vehikel_basis_t>(dt)) {
								if ((v->get_fahrtrichtung() & ribi_t::suedwest) != 0) {
									intern_insert_at(ding, i);
									return true;
								}
							}
						}
					}
				}
				// nothing going southeast
				intern_insert_at(ding, end);
				return true;
			}
		}
		else {
			// driving on right side
			if(fahrtrichtung<4) {	// nord, ost, nordost

				if((fahrtrichtung&(~ribi_t::suedost))==0) {

					// if we are going east we must be drawn as the first in east direction (after nord and nordeast)
					for(uint8 i=start;  i<end;  i++  ) {
						if( (((const vehikel_t*)obj.some[i])->get_fahrtrichtung()&ribi_t::nordost)!=0) {
							intern_insert_at(ding, i);
							return true;
						}
					}
					// nothing going to the east
				}
				// we must be drawn before south or west (thus append after)
				intern_insert_at(ding, end);
				return true;

			}
			else {
				// going south, west or the rest

				if((fahrtrichtung&(~ribi_t::suedost))==0) {
					// going south or southeast, insert as first in this dirs
					intern_insert_at(ding, start);
					return true;
				}
				else {
					for(uint8 i=start;  i<end;  i++  ) {
						// west or northwest: append after all westwards
						if((((const vehikel_t*)obj.some[i])->get_fahrtrichtung()&ribi_t::suedwest)==0) {
							intern_insert_at(ding, i);
							return true;
						}
					}
					// nothing going to nordeast
					intern_insert_at(ding, end);
					return true;
				}
			}

		}	// right side/left side

	}
	else {
		// ok, we have to sort vehicles for correct overlapping,
		// but all vehicles are of the same typ, since this is track/channel etc. ONLY!

		// => much simpler to handle
		if((((vehikel_t*)ding)->get_fahrtrichtung()&(~ribi_t::suedost))==0) {
			// if we are going east or south, we must be drawn before (i.e. put first)
			intern_insert_at(ding, start);
			return true;
		}
		else {
			// for north east we must be draw last
			intern_insert_at(ding, end);
			return true;
		}
	}
	return false;
}

/**
 * @returns true if tree1 must be sorted before tree2 (tree1 stands behind tree2)
 */
bool compare_trees(const ding_t *tree1, const ding_t *tree2)
{
	// the tree with larger yoff is in front
	sint8 diff = tree2->get_yoff() - tree1->get_yoff();
	if (diff==0) {
		// .. or the one that is the left most (ie xoff is small)
		diff = tree1->get_xoff() - tree2->get_xoff();
	}
	return diff>0;
}


void dingliste_t::sort_trees(uint8 index, uint8 count)
{
	assert (capacity > 1);
	if(top>=index+count) {
		std::sort(&obj.some[index], &obj.some[index+count], compare_trees);
	}
}


/*
 * Binary search.
 *
 * Result:
 *   a) index of matching item with lowest index
 * or
 *   b) index, where to insert before unmatched item == next larger item
 * @author: BerndGabriel, neroden
 */
unsigned bsearch_dings_by_prio(unsigned pri, ding_t const * const * dings, unsigned count)
{
  unsigned low = 0;
  while (low < count)
  {
    unsigned i = (low + count) >> 1;
    if (dings[i] && pri > type_to_pri[dings[i]->get_typ()])
      low = i + 1;
    else
      count = i;
  }
  return count;
}


bool dingliste_t::add(ding_t* ding)
{
	// Sanity check: if we're trying to add a null pointer, bail out & crash.
	// If we don't die here, we'll die later when trying to access it.
	assert (ding != NULL);

	if (  capacity==0 && top==0  ) {
		// We have zero items;
		// save the first one directly.
		obj.one = ding;
		top = 1;
		return true;
	}
	else if (  capacity==0 && top==1  ) {
		grow_capacity_above_one();
	}
	// Below here we are guaranteed to be in the "some" case
	else if ( top == capacity ) {
		bool success = grow_capacity();
		if (!success) {
			// Too many objects
			return false;
		}
	} else {
		// Try to catch errors where top exceeds capacity,
		// which is only legal in the "one" case
		assert (top <= capacity);
	}

	// Remembering the convention for capacity
	assert (capacity > 1);

	if(top==0) {
		intern_insert_at( ding, 0);
		return true;
	}
	// now top>0

	// now insert it a the correct place
	const uint8 pri=type_to_pri[ding->get_typ()];

	// vehicles need a special order
	if(pri==moving_obj_pri) {
		return intern_add_moving(ding);
	}

	// roads must be first!
	if(pri==0) {
		// check for other ways to keep order! (maximum is two ways per tile at the moment)
		weg_t const* const w   = ding_cast<weg_t>(obj.some[0]);
		uint8        const pos = w  &&  w->get_waytype() < static_cast<weg_t*>(ding)->get_waytype() ? 1 : 0;
		intern_insert_at(ding, pos);
		return true;
	}


	uint8 i = bsearch_dings_by_prio(pri, obj.some, top);
	// now i contains the position, where we either insert of just add ...
	switch (pri) {
		case baum_pri: {
			/* trees are a little tricky, since they cast a shadow
			 * therefore the y-order must be correct!
			 */
			for(  ;  i<top;  i++) {
				baum_t const* const tree = ding_cast<baum_t>(obj.some[i]);
				if (!tree  ||  compare_trees(ding, tree)) {
					break;
				}
			}
			break;
	    }
		case pillar_pri: {
			// pillars have to be sorted wrt their y-offset, too.
			for(  ;  i<top;  i++) {
				pillar_t const* const pillar = ding_cast<pillar_t>(obj.some[i]);
				if (!pillar  ||  ding->get_yoff()  > pillar->get_yoff() ) {
					break;
				}
			}
			break;
		}
	}

	intern_insert_at(ding, i);
	return true;
}



// take the thing out from the list
// use this only for temperary removing
// since it does not shrink list or checks for ownership
ding_t* dingliste_t::remove_last()
{
	ding_t* d=NULL;
	if (top == 0) {
		// nothing
	}
	else if (capacity == 0) {
		assert (top == 1);
		d = obj.one;
		top = 0;
#if CLEAR_MEMORY
		obj.one = NULL;
#endif
	}
	else {
		top --;
		d = obj.some[top];
#if CLEAR_MEMORY
		obj.some[top] = NULL;
#endif
	}
	consistency_check();
	return d;
}


bool dingliste_t::remove(const ding_t* ding)
{
	bool result = false;
	if (top == 0) {
		// nothing
	}
	else if ( capacity == 0 ) {
		assert ( top == 1 );
		if (obj.one == ding) {
			top = 0;
#if CLEAR_MEMORY
			obj.one = NULL;
#endif
			result = true;
		}
	}
	else {
		// "some" case
		// we keep the array dense!
		for(  uint8 i=0;  i<top;  i++  ) {
			if(  obj.some[i] == ding  ) {
				// found it!
				top--;
				while(  i < top  ) {
					obj.some[i] = obj.some[i+1];
					i++;
				}
#if CLEAR_MEMORY
				obj.some[top] = NULL;
#endif
				result = true;
			}
		}
	}
	consistency_check();
	return result;
}


/**
 * removes object from map
 * deletes object if it is not a zeiger_t
 */
inline void local_delete_object(ding_t *ding, spieler_t *sp)
{
	vehikel_basis_t* const v = ding_cast<vehikel_basis_t>(ding);
	if (v  &&  ding->get_typ() != ding_t::fussgaenger  &&  ding->get_typ() != ding_t::verkehr  &&  ding->get_typ() != ding_t::movingobj) {
		v->verlasse_feld();
	}
	else {
		ding->entferne(sp);
		ding->set_flag(ding_t::not_on_map);
		// all objects except zeiger (pointer) are destroyed here
		// zeiger's will be deleted if their associated werkzeug_t (tool) terminates
		if (ding->get_typ() != ding_t::zeiger) {
			delete ding;
		}
	}
}

/**
 * Delete everything after offset
 * @returns true if we deleted anything
 */
bool dingliste_t::loesche_alle(spieler_t *sp, uint8 offset)
{
	bool result = false;
	if(top<=offset) {
		// Note that this guarantees that top >= 1
	}
	else if (capacity == 0 ) {
		// The "one" case.
		assert (top == 1);

		top = 0;
		local_delete_object(obj.one, sp);
#if CLEAR_MEMORY
		obj.one = NULL;
#endif
		result = true;
	}
	else {
		// The "some" case.
		assert( capacity > 1 );
		while ( top > offset ) {
			top--;
			local_delete_object(obj.some[top], sp);
#if CLEAR_MEMORY
			obj.some[top] = NULL;
#endif
		}
		shrink_capacity();
		result = true;
	}
	consistency_check();
	return result;
}



/* returns the text of an error message, if obj could not be removed */
const char *dingliste_t::kann_alle_entfernen(const spieler_t *sp, uint8 offset) const
{
	if(top<=offset) {
		return NULL;
	}
	// Note that top is guaranteed to be 1 or more here

	if(capacity==0) {
		return obj.one->ist_entfernbar(sp);
	}
	else {
		const char * msg = NULL;

		for(uint8 i=offset; i<top; i++) {
			msg = obj.some[i]->ist_entfernbar(sp);
			if(msg != NULL) {
				return msg;
			}
		}
	}
	return NULL;
}



/* recalculates all images
 */
void dingliste_t::calc_bild()
{
	if (top == 0) {
		// nothing
	}
	else if(capacity==0) {
		// top == 1 because of above
		obj.one->calc_bild();
	}
	else {
		for(uint8 i=0; i<top; i++) {
			obj.some[i]->calc_bild();
		}
	}
}



/* check for obj */
bool dingliste_t::ist_da(const ding_t* ding) const
{
	if (top == 0) {
		return false;
	}
	else if (capacity==0) {
		// top == 1 because of above
		return obj.one==ding;
	}
	else {
		// capacity > 1
		for(uint8 i=0; i<top; i++) {
			if(obj.some[i]==ding) {
				return true;
			}
		}
	}
	return false;
}



ding_t *dingliste_t::suche(ding_t::typ typ,uint8 start) const
{
	if (start >= top) {
		return NULL;
	}
	else if (capacity == 0) {
		// top == 1 because of above
		return obj.one->get_typ()==typ ? obj.one : NULL;
	}
	else {
		// We are in the "some" case
		// Note that the loop will not execute if start >= top
		for(unsigned i=start; i<top; i++) {
			ding_t* tmp = obj.some[i];
			if(tmp->get_typ()==typ) {
				return tmp;
			}
		}
		return NULL;
	}
}



ding_t *dingliste_t::get_leitung() const
{
	if (top == 0) {
		return NULL;
	}
	else if (capacity == 0) {
		return obj.one->get_typ()>=ding_t::leitung  &&  obj.one->get_typ()<=ding_t::senke ? obj.one : NULL;
	}
	else {
		// else we have to search the list
		for(uint8 i=0; i<top; i++) {
			ding_t * tmp = obj.some[i];
			uint8 typ = tmp->get_typ();
			if(typ>=ding_t::leitung  &&  typ<=ding_t::senke) {
				return tmp;
			}
		}
	}
	return NULL;
}



ding_t *dingliste_t::get_convoi_vehicle() const
{
	if (top == 0) {
		return NULL;
	}
	else if (capacity == 0) {
		// top == 1 because of above
		const uint8 t = obj.one->get_typ();
		// Game logic: only aircraft and ships can exist without ways
		return t==ding_t::aircraft  ||  t==ding_t::schiff ? obj.one : NULL;
	}
	else {
		// else we have to search the list
		for(uint8 i=0; i<top; i++) {
			ding_t * tmp = obj.some[i];
			uint8 typ = tmp->get_typ();
			if(  typ>=ding_t::automobil  &&  typ<=ding_t::aircraft  ) {
				return tmp;
			}
		}
	}
	return NULL;
}



void dingliste_t::rdwr(karte_t *welt, loadsave_t *file, koord3d current_pos)
{
	if(file->is_loading()) {

		sint32 max_object_index;
		if(  file->get_version()<=110000  ) {
			file->rdwr_long(max_object_index);
			if(max_object_index>254) {
				dbg->error("dingliste_t::laden()","Too many objects (%i) at (%i,%i), some vehicle may not appear immediately.",max_object_index,current_pos.x,current_pos.y);
			}
		}
		else {
			uint8 obj_count;
			file->rdwr_byte(obj_count);
			max_object_index = -1+(sint32)obj_count;
		}


		for(sint32 i=0; i<=max_object_index; i++) {
			ding_t::typ typ = (ding_t::typ)file->rd_obj_id();
			// DBG_DEBUG("dingliste_t::laden()", "Thing type %d", typ);

			if(typ == -1) {
				continue;
			}

			ding_t *d = NULL;

			switch(typ) {
				case ding_t::bruecke:	    d = new bruecke_t (welt, file);	        break;
				case ding_t::tunnel:	    d = new tunnel_t (welt, file);	        break;
				case ding_t::pumpe:		    d = new pumpe_t (welt, file);	        break;
				case ding_t::leitung:	    d = new leitung_t (welt, file);	        break;
				case ding_t::senke:		    d = new senke_t (welt, file);	        break;
				case ding_t::zeiger:	    d = new zeiger_t (welt, file);	        break;
				case ding_t::signal:	    d = new signal_t (welt, file);   break;
				case ding_t::label:			d = new label_t(welt,file); break;
				case ding_t::crossing:		d = new crossing_t(welt,file); break;

				case ding_t::wayobj:
				{
					wayobj_t* const wo = new wayobj_t(welt, file);
					if (wo->get_besch() == NULL) {
						// ignore missing wayobjs
						wo->set_flag(ding_t::not_on_map);
						delete wo;
						d = NULL;
					}
					else {
						d = wo;
					}
					break;
				}

				// some old offsets will be converted to new ones
				case ding_t::old_fussgaenger:
					typ = ding_t::fussgaenger;
				case ding_t::fussgaenger:
				{
					fussgaenger_t* const pedestrian = new fussgaenger_t(welt, file);
					if (pedestrian->get_besch() == NULL) {
						// no pedestrians ... delete this
						pedestrian->set_flag(ding_t::not_on_map);
						delete pedestrian;
						d = NULL;
					}
					else {
						d = pedestrian;
					}
					break;
				}

				case ding_t::old_verkehr:
					typ = ding_t::verkehr;
				case ding_t::verkehr:
				{
					stadtauto_t* const car = new stadtauto_t(welt, file);
					if (car->get_besch() == NULL) {
						// no citycars ... delete this
						car->set_flag(ding_t::not_on_map);
						delete car;
					}
					else {
						d = car;
					}
					break;
				}

				case ding_t::old_monoraildepot:
					typ = ding_t::monoraildepot;
				case ding_t::monoraildepot:
					d = new monoraildepot_t (welt, file);
					break;
				case ding_t::old_tramdepot:
					typ = ding_t::tramdepot;
				case ding_t::tramdepot:
					d = new tramdepot_t (welt, file);
					break;
				case ding_t::strassendepot:
					d = new strassendepot_t (welt, file);
					break;
				case ding_t::schiffdepot:
					d = new schiffdepot_t (welt, file);
					break;
				case ding_t::old_airdepot:
					typ = ding_t::airdepot;
				case ding_t::airdepot:
					d = new airdepot_t (welt, file);
					break;
				case ding_t::maglevdepot:
					d = new maglevdepot_t (welt, file);
					break;
				case ding_t::narrowgaugedepot:
					d = new narrowgaugedepot_t (welt, file);
					break;

				case ding_t::bahndepot:
				{
					// for compatibilty reasons we may have to convert them to tram and monorail depots
					bahndepot_t*                   bd;
					gebaeude_t                     gb(welt, file);
					haus_tile_besch_t const* const tile = gb.get_tile();
					if(  tile  ) {
						switch (tile->get_besch()->get_extra()) {
							case monorail_wt: bd = new monoraildepot_t(welt, gb.get_pos(), gb.get_besitzer(), tile); break;
							case tram_wt:     bd = new tramdepot_t(    welt, gb.get_pos(), gb.get_besitzer(), tile); break;
							default:          bd = new bahndepot_t(    welt, gb.get_pos(), gb.get_besitzer(), tile); break;
						}
					}
					else {
						bd = new bahndepot_t( welt, gb.get_pos(), gb.get_besitzer(), NULL );
					}
					bd->rdwr_vehicles(file);
					d   = bd;
					typ = d->get_typ();
					// do not remove from this position, since there will be nothing
					gb.set_flag(ding_t::not_on_map);
				}
				break;

				// check for pillars
				case ding_t::old_pillar:
					typ = ding_t::pillar;
				case ding_t::pillar:
				{
					pillar_t *p = new pillar_t(welt, file);
					if(p->get_besch()!=NULL  &&  p->get_besch()->get_pillar()!=0) {
						d = p;
					}
					else {
						// has no pillar ...
						// do not remove from this position, since there will be nothing
						p->set_flag(ding_t::not_on_map);
						delete p;
					}
				}
				break;

				case ding_t::baum:
				{
					baum_t *b = new baum_t(welt, file);
					if(  !b->get_besch()  ) {
						// is there a replacement possible
						if(  const baum_besch_t *besch = baum_t::random_tree_for_climate( welt->get_climate(current_pos.z) )  ) {
							b->set_besch( besch );
						}
						else {
							// do not remove from map on this position, since there will be nothing
							b->set_flag(ding_t::not_on_map);
							delete b;
							b = NULL;
						}
					}
					else {
						d = b;
					}
				}
				break;

				case ding_t::groundobj:
				{
					groundobj_t* const groundobj = new groundobj_t(welt, file);
					if(groundobj->get_besch() == NULL) {
						// do not remove from this position, since there will be nothing
						groundobj->set_flag(ding_t::not_on_map);
						// not use entferne, since it would try to lookup besch
						delete groundobj;
					}
					else {
						d = groundobj;
					}
					break;
				}

				case ding_t::movingobj:
				{
					movingobj_t* const movingobj = new movingobj_t(welt, file);
					if (movingobj->get_besch() == NULL) {
						// no citycars ... delete this
						movingobj->set_flag(ding_t::not_on_map);
						delete movingobj;
					}
					else {
						d = movingobj;
					}
					break;
				}

				case ding_t::gebaeude:
				{
					gebaeude_t *gb = new gebaeude_t (welt, file);
					if(gb->get_tile()==NULL) {
						// do not remove from this position, since there will be nothing
						gb->set_flag(ding_t::not_on_map);
						delete gb;
						gb = NULL;
					}
					else {
						d = gb;
					}
				}
				break;

				case ding_t::old_roadsign:
					typ = ding_t::roadsign;
				case ding_t::roadsign:
				{
					roadsign_t *rs = new roadsign_t (welt, file);
					if(rs->get_besch()==NULL) {
						// roadsign_t without description => ignore
						rs->set_flag(ding_t::not_on_map);
						delete rs;
					}
					else {
						d = rs;
					}
				}
				break;

				// will be ignored, was only used before 86.09
				case ding_t::old_gebaeudefundament: { dummy_ding_t(welt, file); break; }

				// only factories can smoke; but then, the smoker is reinstated after loading
				case ding_t::raucher: { raucher_t(welt, file); break; }

				// wolke is not saved anymore
				case ding_t::sync_wolke: { wolke_t(welt, file); break; }
				case ding_t::async_wolke: { async_wolke_t(welt, file); break; }

				default:
					dbg->fatal("dingliste_t::laden()", "During loading: Unknown object type '%d'", typ);
			}

			if(d  &&  d->get_typ()!=typ) {
				dbg->warning( "dingliste_t::rdwr()","typ error : %i instead %i on %i,%i, object ignored!", d->get_typ(), typ, d->get_pos().x, d->get_pos().y );
				d = NULL;
			}

			if(d  &&  d->get_pos()==koord3d::invalid) {
				d->set_pos( current_pos );
			}

			if(d  &&  d->get_pos()!=current_pos) {
				dbg->warning("dingliste_t::rdwr()","position error: %i,%i,%i instead %i,%i,%i (object will be ignored)",d->get_pos().x,d->get_pos().y,d->get_pos().z,current_pos.x,current_pos.y,current_pos.z);
				d = NULL;
			}

			if(d) {
				add(d);
			}
		}
	}
	else {
		/* here is the saving part ...
		 * first: construct a list of stuff really neded to save
		 */
		ding_t *save[256];
		sint32 max_object_index = 0;
		for(  uint16 i=0;  i<top;  i++  ) {
			ding_t *d = bei((uint8)i);
			if(d->get_typ()==ding_t::way
				// do not save smoke
				||  d->get_typ()==ding_t::raucher
				||  d->get_typ()==ding_t::sync_wolke
				||  d->get_typ()==ding_t::async_wolke
				// fields will be built by factory
				||  d->get_typ()==ding_t::field
				// do not save factory buildings => factory will reconstruct them
				||  (d->get_typ()==ding_t::gebaeude  &&  ((gebaeude_t *)d)->get_fabrik())
				// things with convoi will not be saved
				||  (d->get_typ()>=66  &&  d->get_typ()<82)
				||  (umgebung_t::server  &&  d->get_typ()==ding_t::baum  &&  file->get_version()>=110001)
			) {
				// these objects are simply not saved
			}
			else {
				save[max_object_index++] = d;
			}
		}
		// now we know the number of stuff to save
		max_object_index --;
		if(  file->get_version()<=110000  ) {
			file->rdwr_long( max_object_index );
		}
		else {
			uint8 obj_count = max_object_index+1;
			file->rdwr_byte( obj_count );
		}
		for(sint32 i=0; i<=max_object_index; i++) {
			ding_t *d = save[i];
			if(d->get_pos()==current_pos) {
				file->wr_obj_id(d->get_typ());
				d->rdwr(file);
			}
			else if (d->get_pos().get_2d() == current_pos.get_2d()) {
				// ok, just error in z direction => we will correct it
				dbg->warning( "dingliste_t::rdwr()","position error: z pos corrected on %i,%i from %i to %i", d->get_pos().x, d->get_pos().y, d->get_pos().z, current_pos.z);
				file->wr_obj_id(d->get_typ());
				d->set_pos(current_pos);
				d->rdwr(file);
			}
			else {
				dbg->error("dingliste_t::rdwr()","unresolvable position error: %i,%i instead %i,%i (object type %i will be not saved!)", d->get_pos().x, d->get_pos().y, current_pos.x, current_pos.y, d->get_typ());
				file->wr_obj_id(-1);
			}
		}
	}
}



/* Dumps a short info about the things on this tile
 *  @author prissi
 */
void dingliste_t::dump() const
{
	if (top == 0) {
//		DBG_MESSAGE("dingliste_t::dump()","empty");
		return;
	}
	else if(capacity==0) {
		DBG_MESSAGE("dingliste_t::dump()","one object \'%s\' owned by sp %p", obj.one->get_name(), obj.one->get_besitzer() );
		return;
	}

	DBG_MESSAGE("dingliste_t::dump()","%i objects", top );
	for(uint8 n=0; n<top; n++) {
		DBG_MESSAGE( obj.some[n]->get_name(), "at %i owned by sp %p", n, obj.some[n]->get_besitzer() );
	}
}


/** display all things, faster, but will lead to clipping errors
 *  @author prissi
 */
void dingliste_t::display_dinge_quick_and_dirty( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const bool is_global ) const
{
	if(top==0) {
		return;
	}
	else if(capacity==0) {
		if(start_offset==0) {
			obj.one->display(xpos, ypos);
			obj.one->display_after(xpos, ypos, is_global);
			if(is_global) {
				obj.one->clear_flag(ding_t::dirty);
			}
		}
		return;
	}

	for(uint8 n=start_offset; n<top; n++) {
		// ist dort ein objekt ?
		obj.some[n]->display(xpos, ypos );
	}
	// foreground (needs to be done backwards!
	for (size_t n = top; n-- != 0;) {
		obj.some[n]->display_after(xpos, ypos, is_global);
		if(is_global) {
			obj.some[n]->clear_flag(ding_t::dirty);
		}
	}
}


/**
 * Routine to display background images of non-moving things
 * powerlines have to be drawn after vehicles (and thus are in the obj-array inserted after vehicles)
 * @return the index of the first moving thing (or powerline)
 *
 * dingliste_t::display_dinge_bg() .. called by the methods in grund_t
 * local_display_dinge_bg()        .. local function to avoid code duplication, returns false if the first non-valid obj is reached
 * @author Dwachs
 */
inline bool local_display_dinge_bg(const ding_t *ding, const sint16 xpos, const sint16 ypos)
{
	const bool display_ding = !ding->is_moving();
	if (display_ding) {
		ding->display(xpos, ypos);
	}
	return display_ding;
}


uint8 dingliste_t::display_dinge_bg( const sint16 xpos, const sint16 ypos, const uint8 start_offset) const
{
	if(start_offset>=top) {
		return start_offset;
	}

	// Here we know that top>=1
	if(capacity==0) {
		if(local_display_dinge_bg(obj.one, xpos, ypos)) {
			return 1;
		}
		return 0;
	}

	for(uint8 n=start_offset; n<top; n++) {
		if (!local_display_dinge_bg(obj.some[n], xpos, ypos)) {
			return n;
		}
	}
	return top;
}


/**
 * Routine to draw vehicles
 * .. vehicles are draws if driving in direction ribi (with special treatment of flying aircrafts)
 * .. clips vehicle only along relevant edges (depends on ribi and vehicle direction)
 * @param ontile if true then vehicles are on the tile that defines the clipping
 * @return the index of the first non-moving thing
 *
 * dingliste_t::display_dinge_vh() .. called by the methods in grund_t
 * local_display_dinge_vh()        .. local function to avoid code duplication, returns false if the first non-valid obj is reached
 * @author Dwachs
 */
inline bool local_display_dinge_vh(const ding_t *ding, const sint16 xpos, const sint16 ypos, const ribi_t::ribi ribi, const bool ontile)
{
	vehikel_basis_t const* const v = ding_cast<vehikel_basis_t>(ding);
	aircraft_t      const*       a;
	if (v && (ontile || !(a = ding_cast<aircraft_t>(v)) || a->is_on_ground())) {
		const ribi_t::ribi veh_ribi = v->get_fahrtrichtung();
		if (ontile || (veh_ribi & ribi)==ribi  ||  (ribi_t::rueckwaerts(veh_ribi) & ribi)==ribi  ||  ding->get_typ()==ding_t::aircraft) {
			// activate clipping only for our direction masked by the ribi argument
			// use non-convex clipping (16) only if we are on the currently drawn tile or its n/w neighbours
			activate_ribi_clip( ((veh_ribi|ribi_t::rueckwaerts(veh_ribi))&ribi)  |  (ontile  ||  ribi==ribi_t::nord || ribi==ribi_t::west ? 16 : 0));
			ding->display(xpos, ypos);
		}
		return true;
	}
	else {
		// if !ontile starting_offset is not correct, hence continue searching till the end
		return !ontile;
	}
}


uint8 dingliste_t::display_dinge_vh( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const ribi_t::ribi ribi, const bool ontile ) const
{
	if(start_offset>=top) {
		return start_offset;
	}

	// Here we know that top >=0
	if(capacity==0) {
		if(local_display_dinge_vh(obj.one, xpos, ypos, ribi, ontile)) {
			return 1;
		}
		else {
			return 0;
		}
	}

	uint8 nr_v = start_offset;
	for(uint8 n=start_offset; n<top; n++) {
		if (local_display_dinge_vh(obj.some[n], xpos, ypos, ribi, ontile)) {
			nr_v = n;
		}
		else {
			break;
		}
	}
	activate_ribi_clip();
	return nr_v+1;
}


/**
 * Routine to draw foreground images of everything on the tile (no clipping) and powerlines
 * @param start_offset .. draws also background images of all objects with index>=start_offset
 * @param is_global will be only true for the main display; all miniworld windows should still reset main window
 */
void dingliste_t::display_dinge_fg( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const bool is_global ) const
{
	if(top==0) {
		return;
	}
	else if(capacity==0) {
		if(start_offset==0) {
			obj.one->display(xpos, ypos);
		}
		obj.one->display_after(xpos, ypos, is_global);
		if(is_global) {
			obj.one->clear_flag(ding_t::dirty);
		}
		return;
	}

	for(uint8 n=start_offset; n<top; n++) {
		// ist dort ein objekt ?
		obj.some[n]->display(xpos, ypos);
	}
	// foreground (needs to be done backwards!
	for (size_t n = top; n-- != 0;) {
		obj.some[n]->display_after(xpos, ypos, is_global);
		if(is_global) {
			obj.some[n]->clear_flag(ding_t::dirty);
		}
	}
	return;
}


// start next month (good for toogling a seasons)
// Also deletes any objects which want to die based on the timeline
// -- mostly trees do this.
void dingliste_t::check_season(const long month)
{
	consistency_check();
	slist_tpl<ding_t *>to_remove;
	bool do_shrink=false;
	if (top == 0) {
		return;
	}
	else if(capacity==0) {
		assert (top == 1);
		ding_t *d = obj.one;
		if (!d->check_season(month)) {
			to_remove.insert( d );
		}
		return;
	}
	else {
		for(uint8 i=0; i<top; i++) {
			ding_t *d = obj.some[i];
			if (!d->check_season(month)) {
				to_remove.insert( d );
				do_shrink = true;
			}
		}
	}

	// delete all objects, which do not want to step anymore
	// These are mostly dying trees
	// There should not be many of them, so don't worry about efficiency
	FOR( slist_tpl<ding_t*>, & d, to_remove)
	{
		remove(d);
		// in case something other than trees is deleted,
		// perform the checks and clean up properly
		local_delete_object(d, NULL);
	}
	if (do_shrink) {
		shrink_capacity();
	}
}

/**
 * Checks that the data is consistent and abort otherwise.
 * If there are null pointers in the "good data", it isn't.
 * This shouldn't be used normally; it's slow.
 * It's needed to debug memory errors.
 *
 */
inline void dingliste_t::consistency_check() const {
	assert (capacity != 1);
	if (capacity == 0) {
		assert (top <= 1);
		if (top == 1) {
			assert (obj.one);
		}
	}
	else {
		// "some" case
		assert (obj.some);
		assert (top <= capacity);
		for (int i = 0; i < top; i++) {
			assert (obj.some[i]);
		}
	}
}
