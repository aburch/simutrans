/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <algorithm>

#include "../simdebug.h"
#include "../display/simgraph.h"
#include "../world/simworld.h"

#include "../builder/hausbauer.h"
#include "../obj/dummy.h"
#include "../obj/wolke.h"
#include "../obj/zeiger.h"
#include "../obj/crossing.h"
#include "../obj/baum.h"
#include "../obj/bruecke.h"
#include "../obj/field.h"
#include "../obj/pillar.h"
#include "../obj/tunnel.h"
#include "../obj/gebaeude.h"
#include "../obj/signal.h"
#include "../obj/label.h"
#include "../obj/leitung2.h"
#include "../obj/wayobj.h"
#include "../obj/roadsign.h"
#include "../obj/groundobj.h"

#include "../simtypes.h"
#include "../obj/depot.h"
#include "../simmem.h"

#include "../player/simplay.h"

#include "../vehicle/air_vehicle.h"
#include "../vehicle/simroadtraffic.h"
#include "../vehicle/pedestrian.h"
#include "../vehicle/movingobj.h"

#include "../descriptor/building_desc.h"
#include "../descriptor/groundobj_desc.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/freelist.h"
#include "../dataobj/environment.h"

#include "objlist.h"


// priority for inserting into objlist

#define PRI_WAY         (0) // ways (always at the top!)
#define PRI_DEPOT       (1) // depots (must be before tunnel!)
#define PRI_CROSSING    (1) // crossings, treated like bridges or tunnels
#define PRI_GROUNDOBJ   (1)
#define PRI_BRIDGE      (2)
#define PRI_TUNNEL      (2)
#define PRI_BUILDING    (3)
#define PRI_FIELD       (3)
#define PRI_TRANSFORMER (4)
#define PRI_RAUCHER     (5)
#define PRI_SIGNAL      (6)
#define PRI_ROADSIGN    PRI_SIGNAL
#define PRI_PILLAR      (7)
#define PRI_WAYOBJ      (8)
#define PRI_LABEL       (9)
#define PRI_TREE        (50)
#define PRI_MOVABLE     (100) // priority of moving things: should be smaller than the priority of powerlines
#define PRI_AIRCRAFT    (PRI_MOVABLE + 1)
#define PRI_MOVINGOBJ   (PRI_MOVABLE + 1)
#define PRI_POWERLINE   (150)
#define PRI_CLOUD       (200)
#define PRI_ZEIGER      (254)
#define PRI_INVALID     (255)


// Maps obj_t::type -> objlist insertion order
// unused entries have PRI_INVALID
static uint8 type_to_pri[256] =
{
	PRI_INVALID,     // obj

	PRI_TREE,        // baum
	PRI_ZEIGER,      // cursor/pointers
	PRI_CLOUD,       // wolke
	PRI_CLOUD,       // sync_wolke
	PRI_CLOUD,       // async_wolke

	PRI_INVALID,     // gebaeude_alt, unused
	PRI_BUILDING,    // gebaeude

	PRI_SIGNAL,      // signal
	PRI_BRIDGE,      // bridge
	PRI_TUNNEL,      // tunnel
	PRI_INVALID,     // old_gebaeudefundament, unused

	PRI_DEPOT,       // bahndepot
	PRI_DEPOT,       // strassendepot
	PRI_DEPOT,       // schiffdepot

	PRI_RAUCHER,     // smoke generator (not used any more)

	PRI_POWERLINE,   // poweline
	PRI_TRANSFORMER, // pumpe
	PRI_TRANSFORMER, // senke

	PRI_ROADSIGN,    // roadsign
	PRI_PILLAR,      // pillar

	PRI_DEPOT,       // airdepot
	PRI_DEPOT,       // monoraildepot
	PRI_DEPOT,       // tramdepot
	PRI_DEPOT,       // maglevdepot

	PRI_WAYOBJ,      // way objects (electrification)
	PRI_WAY,         // way
	PRI_LABEL,       // label, indicates ownership: insert before trees
	PRI_FIELD,       // field (factory extension)
	PRI_CROSSING,    // crossing
	PRI_GROUNDOBJ,   // groundobjs, overlays over bare ground like lakes etc.

	PRI_DEPOT,       // narrowgaugedepot

	// 32-63 left empty (old numbers)
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,

	PRI_MOVABLE,   // pedestrians
	PRI_MOVABLE,   // city cars
	PRI_MOVABLE,   // road vehicle
	PRI_MOVABLE,   // rail vehicle
	PRI_MOVABLE,   // monorail
	PRI_MOVABLE,   // maglev
	PRI_MOVABLE,   // narrowgauge

	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,

	PRI_MOVABLE,   // ship
	PRI_AIRCRAFT,  // aircraft (no trailer, could be handled by normal method)
	PRI_MOVINGOBJ, // movingobject (no trailer, could be handled by normal method)

	// 83-95 left empty (for other moving stuff); 95 is reserved for old choosesignals
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,

	// 96-128 left empty (old numbers)
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,

	// 128-255 left empty
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID,
	PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID, PRI_INVALID
};


static void dl_free(obj_t** p, uint8 size)
{
	assert(size > 1);
	if (size <= 16) {
		freelist_t::putback_node(sizeof(*p) * size, p);
	}
	else {
		free(p);
	}
}


static obj_t** dl_alloc(uint8 size)
{
	assert(size > 1);
	obj_t** p;
	if (size <= 16) {
		p = static_cast<obj_t**>(freelist_t::gimme_node(sizeof(*p) * size ));
	}
	else {
		p = MALLOCN(obj_t*, size);
	}
	return p;
}


objlist_t::objlist_t()
{
	obj.one = NULL;
	capacity = 0;
	top = 0;
}


objlist_t::~objlist_t()
{
	if(  capacity == 1  ) {
		obj.one->set_flag(obj_t::not_on_map);

		if(!obj.one->has_managed_lifecycle()) {
			delete obj.one;
		}
	}
	else {
		for(  uint8 i=0;  i<top;  i++  ) {
			obj_t* const object = obj.some[i];
			object->set_flag(obj_t::not_on_map);

			if(!object->has_managed_lifecycle()) {
				delete object;
			}
		}
	}

	if(capacity>1) {
		dl_free(obj.some, capacity);
	}
	obj.some = NULL;
	capacity = top = 0;
}


void objlist_t::set_capacity(uint16 req_cap)
{
	// DBG_MESSAGE("objlist_t::set_capacity()", "old cap=%d, new cap=%d", capacity, new_cap);

	const uint8 new_cap = req_cap >= 255 ? 254 : (uint8)req_cap;

	// a single object is stored differentially
	if(new_cap==0) {
		if(capacity>1) {
			dl_free( obj.some, capacity );
		}
		obj.one = NULL;
		capacity = 0;
		top = 0;
	}
	else if(new_cap==1) {
		if(capacity>1) {
			obj_t *tmp=NULL;
			// we have an obj to save into the list
			tmp = obj.some[0];
			dl_free( obj.some, capacity );
			obj.one = tmp;
			assert(top<2);
		}
		else if(top==0) {
			obj.one = NULL;
		}
		capacity = top;
	}
	else if(capacity<=1) {
		// this means we extend from 0 or 1 elements to more than 1
		obj_t *tmp=obj.one;
		obj.some = dl_alloc(new_cap);
		MEMZERON(obj.some, new_cap);
		obj.some[0] = tmp;
		capacity = new_cap;
		assert(top<=1);
	}
	else {
		// we need to copy old list to large list
		assert(  top<=new_cap  );

		// get memory
		obj_t **tmp = dl_alloc(new_cap);

		// free old memory
		if(obj.some) {
			memcpy( tmp, obj.some, sizeof(obj_t *)*top );
			dl_free(obj.some, capacity);
		}
		obj.some = tmp;
		capacity = new_cap;
	}
}


bool objlist_t::grow_capacity()
{
	if(capacity==0) {
		return true;
	}
	else if(capacity==1) {
		set_capacity( 4 );
		return true;
	}
	else if(capacity>=254) {
		// capacity exceeded ... (and no need for THAT many objects here ... )
		return false;
	}
	else {
		// size exceeded, extent
		uint16 new_cap = (uint16)capacity+4;
		set_capacity( new_cap );
		return true;
	}
}


void objlist_t::shrink_capacity(uint8 o_top)
{
	// strategy: avoid freeing mem if not needed. Only if we hold lots of memory then free it.
	// this is almost only called when deleting ways in practice
	if(  capacity > 16  &&  o_top <= 4  ) {
		set_capacity(o_top);
	}
}


inline void objlist_t::intern_insert_at(obj_t* new_obj, uint8 pri)
{
	// we have more than one object here, thus we can use obj.some exclusively!
	for(  uint8 i=top;  i>pri;  i--  ) {
		obj.some[i] = obj.some[i-1];
	}
	obj.some[pri] = new_obj;
	top++;
}


// only used internal for loading. DO NOT USE OTHERWISE!
bool objlist_t::append(obj_t *new_obj)
{
	if(capacity==0) {
		// the first one save direct
		obj.one = new_obj;
		top = 1;
		capacity = 1;
		return true;
	}

	if(top>=capacity  &&  !grow_capacity()) {
		// memory exceeded
		return false;
	}

	intern_insert_at(new_obj, top);
	return true;
}


// this will automatically give the right order for citycars and the like ...
bool objlist_t::intern_add_moving(obj_t* new_obj)
{
	// we are more than one object, thus we exclusively use obj.some here!
	// it would be nice, if also the objects are inserted according to their priorities as
	// vehicles types (number returned by get_typ()). However, this would increase
	// the calculation even further. :(

	// insert at this lane
	uint8 lane = ((vehicle_base_t*)new_obj)->get_disp_lane();

	// find out about the first car etc. moving thing.
	// We can start to insert between (start) and (end)
	uint8 start=0;
	while(start<top  &&  (!obj.some[start]->is_moving()  ||  ((vehicle_base_t*)obj.some[start])->get_disp_lane() < lane) ) {
		start ++;
	}
	uint8 end = top;
	while(  end>start  &&  (!obj.some[end-1]->is_moving()  ||  ((vehicle_base_t*)obj.some[end-1])->get_disp_lane() > lane) ) {
		end--;
	}
	if(start==end) {
		intern_insert_at(new_obj, start);
		return true;
	}

	const uint8 direction = ((vehicle_base_t*)new_obj)->get_direction();

	switch(lane) {
		// pedestrians or road vehicles, back: either w/sw/s or n/ne/e
		case 0:
		case 1: {
			// on right side to w,sw; on left to n: insert last
			// on right side to s; on left to ne,e: insert first
			if (direction == ribi_t::south  ||  (direction & ribi_t::east)) {
				intern_insert_at(new_obj, start);
			}
			else {
				intern_insert_at(new_obj, end);
			}
			return true;
		}
		// middle land
		case 2:
		case 3:
		// pedestrians, road vehicles, front lane
		case 4: {
			// going e/s: insert first, else last
			if ( (direction & ribi_t::northwest)==0 ) {
				intern_insert_at(new_obj, start);
			}
			else {
				intern_insert_at(new_obj, end);
			}
			return true;
		}
	}
	return false;
}


/**
 * @returns true if tree1 must be sorted before tree2 (tree1 stands behind tree2)
 */
bool compare_trees(const obj_t *tree1, const obj_t *tree2)
{
	// the tree with larger yoff is in front
	sint8 diff = tree2->get_yoff() - tree1->get_yoff();
	if (diff==0) {
		// .. or the one that is the left most (ie xoff is small)
		diff = tree1->get_xoff() - tree2->get_xoff();
	}
	return diff>0;
}


void objlist_t::sort_trees(uint8 index, uint8 count)
{
	if(top>=index+count) {
		std::sort(&obj.some[index], &obj.some[index+count], compare_trees);
	}
}


bool objlist_t::add(obj_t* new_obj)
{
	if(capacity==0) {
		// the first one save direct
		obj.one = new_obj;
		top = 1;
		capacity = 1;
		return true;
	}

	if(top>=capacity  &&  !grow_capacity()) {
		// memory exceeded
		return false;
	}
	if(top==0) {
		intern_insert_at( new_obj, 0 );
		return true;
	}
	// now top>0

	// now insert it a the correct place
	const uint8 pri=type_to_pri[new_obj->get_typ()];

	// vehicles need a special order
	if(pri==PRI_MOVABLE) {
		return intern_add_moving(new_obj);
	}

	// roads must be first!
	if(pri==0) {
		// check for other ways to keep order! (maximum is two ways per tile at the moment)
		weg_t const* const w   = obj_cast<weg_t>(obj.some[0]);
		uint8        const pos = w  &&  w->get_waytype() < static_cast<weg_t*>(new_obj)->get_waytype() ? 1 : 0;
		intern_insert_at(new_obj, pos);
		return true;
	}

	uint8 i;
	for(  i=0;  i<top  &&  pri>type_to_pri[obj.some[i]->get_typ()];  i++  )
		;
	// now i contains the position, where we either insert of just add ...
	if(i==top) {
		obj.some[top] = new_obj;
		top++;
	}
	else {
		if(pri==PRI_TREE) {
			/* trees are a little tricky, since they cast a shadow
			 * therefore the y-order must be correct!
			 */
			for(  ;  i<top;  i++) {
				baum_t const* const tree = obj_cast<baum_t>(obj.some[i]);
				if (!tree  ||  compare_trees(new_obj, tree)) {
					break;
				}
			}
		}
		else if (pri == PRI_PILLAR) {
			// pillars have to be sorted wrt their y-offset, too.
			for(  ;  i<top;  i++) {
				pillar_t const* const pillar = obj_cast<pillar_t>(obj.some[i]);
				if (!pillar  ||  new_obj->get_yoff()  > pillar->get_yoff() ) {
					break;
				}
			}
		}
		else if(  pri == PRI_WAYOBJ  &&  obj.some[i]->get_typ()==obj_t::wayobj  ) {
			wayobj_t const* const wo = obj_cast<wayobj_t>(obj.some[i]);
			if(  wo  &&  wo->get_waytype() < obj_cast<wayobj_t>(new_obj)->get_waytype() ) {
				// insert after a lower waytype
				i += 1;
			}
		}
		intern_insert_at(new_obj, i);
	}
	// then correct the upper border
	return true;
}


// take the thing out from the list
// use this only for temporary removing
// since it does not shrink list or checks for ownership
obj_t *objlist_t::remove_last()
{
	obj_t *last_obj=NULL;
	if(capacity==0) {
		// nothing
	}
	else if(capacity==1) {
		last_obj = obj.one;
		obj.one = NULL;
		capacity = top = 0;
	}
	else {
		if(top>0) {
			top --;
			last_obj = obj.some[top];
			obj.some[top] = NULL;
		}
	}
	return last_obj;
}


bool objlist_t::remove(const obj_t* remove_obj)
{
	if(  capacity == 0  ) {
		return false;
	}
	else if(  capacity == 1  ) {
		if(  obj.one == remove_obj  ) {
			obj.one = NULL;
			capacity = 0;
			top = 0;
			return true;
		}
		return false;
	}

	// we keep the array dense!
	for(  uint8 i=0;  i<top;  i++  ) {
		if(  obj.some[i] == remove_obj  ) {
			// found it!
			top--;
			while(  i < top  ) {
				obj.some[i] = obj.some[i+1];
				i++;
			}
			obj.some[top] = NULL;
			return true;
		}
	}

	return false;
}


/**
 * removes object from map
 * deletes object if it is not a zeiger_t
 */
static void local_delete_object(obj_t *remove_obj, player_t *player)
{
	vehicle_base_t* const v = obj_cast<vehicle_base_t>(remove_obj);
	if (v  &&  remove_obj->get_typ() != obj_t::pedestrian  &&  remove_obj->get_typ() != obj_t::road_user  &&  remove_obj->get_typ() != obj_t::movingobj) {
		v->leave_tile();
	}
	else {
		remove_obj->cleanup(player);
		remove_obj->set_flag(obj_t::not_on_map);
		// all objects except zeiger (pointer) are destroyed here
		// zeiger's will be deleted if their associated tool terminates
		if (!remove_obj->has_managed_lifecycle()) {
			delete remove_obj;
		}
	}
}


bool objlist_t::loesche_alle(player_t *player, uint8 offset)
{
	if(top<=offset) {
		return false;
	}

	// something to delete?
	bool ok=false;

	if(capacity>1) {
		while(  top>offset  ) {
			top --;
			local_delete_object(obj.some[top], player);
			obj.some[top] = NULL;
			ok = true;
		}
	}
	else {
		if(capacity==1) {
			local_delete_object(obj.one, player);
			ok = true;
			obj.one = NULL;
			capacity = top = 0;
		}
	}
	shrink_capacity(top);

	return ok;
}


/* returns the text of an error message, if obj could not be removed */
const char *objlist_t::kann_alle_entfernen(const player_t *player, uint8 offset) const
{
	if(top<=offset) {
		return NULL;
	}

	if(capacity==1) {
		return obj.one->is_deletable(player);
	}
	else {
		const char * msg = NULL;

		for(uint8 i=offset; i<top; i++) {
			msg = obj.some[i]->is_deletable(player);
			if(msg != NULL) {
				return msg;
			}
		}
	}
	return NULL;
}


/* recalculates all images
 */
void objlist_t::calc_image()
{
	if(capacity==0) {
		// nothing
	}
	else if(capacity==1) {
		obj.one->calc_image();
	}
	else {
		for(uint8 i=0; i<top; i++) {
			obj.some[i]->calc_image();
		}
	}
}


void objlist_t::set_all_dirty()
{
	if(  capacity == 0  ) {
		// nothing
	}
	else if(  capacity == 1  ) {
		obj.one->set_flag( obj_t::dirty );
	}
	else {
		for(  uint8 i = 0;  i < top;  i++  ) {
			obj.some[i]->set_flag( obj_t::dirty );
		}
	}
}


/* check for obj */
bool objlist_t::ist_da(const obj_t* test_obj) const
{
	if(capacity<=1) {
		return obj.one==test_obj;
	}
	else {
		for(uint8 i=0; i<top; i++) {
			if(obj.some[i]==test_obj) {
				return true;
			}
		}
	}
	return false;
}


obj_t *objlist_t::suche(obj_t::typ typ,uint8 start) const
{
	if(  start >= top  ) {
		// start==0 and top==0 is already covered by this too
		return NULL;
	}

	if(  capacity <= 1  ) {
		// it will crash on capacity==1 and top==0, but this should never happen!
		// this is only reached for top==1 and start==0
		return obj.one->get_typ()!=typ ? NULL : obj.one;
	}
	else {
		// else we have to search the list
		for(uint8 i=start; i<top; i++) {
			obj_t * tmp = obj.some[i];
			if(tmp->get_typ()==typ) {
				return tmp;
			}
		}
	}
	return NULL;
}


obj_t *objlist_t::get_leitung() const
{
	if(  top == 0  ) {
		return NULL;
	}

	if(  capacity <= 1  ) {
		// it will crash on capacity==1 and top==0, but this should never happen!
		if(  obj.one->get_typ() >= obj_t::leitung  &&  obj.one->get_typ() <= obj_t::senke  ) {
			return obj.one;
		}
	}
	else {
		// else we have to search the list
		for(  uint8 i=0;  i<top;  i++  ) {
			uint8 typ = obj.some[i]->get_typ();
			if(  typ >= obj_t::leitung  &&  typ <= obj_t::senke  ) {
				return obj.some[i];
			}
		}
	}
	return NULL;
}


obj_t *objlist_t::get_convoi_vehicle() const
{
	if(  top == 0  ) {
		return NULL;
	}

	if(  capacity <= 1  ) {
		// it will crash on capacity==1 and top==0, but this should never happen!
		// only ships and aircraft can go on tiles without ways => only test for those
		uint8 t = obj.one->get_typ();
		if(  t == obj_t::air_vehicle  ||  t == obj_t::water_vehicle  ) {
			return obj.one;
		}
	}
	else {
		for(  uint8 i=0;  i < top;  i++  ) {
			uint8 typ = obj.some[i]->get_typ();
			if(  typ >= obj_t::road_vehicle  &&  typ <= obj_t::air_vehicle  ) {
				return obj.some[i];
			}
		}
	}
	return NULL;
}


void objlist_t::rdwr(loadsave_t *file, koord3d current_pos)
{
	if(file->is_loading()) {

		sint32 max_object_index;
		if(  file->is_version_less(110, 1)  ) {
			file->rdwr_long(max_object_index);
			if(max_object_index>254) {
				dbg->error("objlist_t::laden()","Too many objects (%i) at (%i,%i), some vehicle may not appear immediately.",max_object_index,current_pos.x,current_pos.y);
			}
		}
		else {
			uint8 obj_count;
			file->rdwr_byte(obj_count);
			max_object_index = -1+(sint32)obj_count;
		}


		for(sint32 i=0; i<=max_object_index; i++) {
			obj_t::typ typ = (obj_t::typ)file->rd_obj_id();
			// DBG_DEBUG("objlist_t::laden()", "Thing type %d", typ);

			if(typ == -1) {
				continue;
			}

			obj_t *new_obj = NULL;

			switch(typ) {
				case obj_t::bruecke:    new_obj = new bruecke_t(file);  break;
				case obj_t::tunnel:     new_obj = new tunnel_t(file);   break;
				case obj_t::pumpe:      new_obj = new pumpe_t(file);    break;
				case obj_t::leitung:    new_obj = new leitung_t(file);  break;
				case obj_t::senke:      new_obj = new senke_t(file);    break;
				case obj_t::zeiger:     new_obj = new zeiger_t(file);   break;
				case obj_t::signal:     new_obj = new signal_t(file);   break;
				case obj_t::label:      new_obj = new label_t(file);    break;
				case obj_t::crossing:   new_obj = new crossing_t(file); break;

				case obj_t::wayobj:
				{
					wayobj_t* const wo = new wayobj_t(file);
					if (wo->get_desc() == NULL) {
						// ignore missing wayobjs
						wo->set_flag(obj_t::not_on_map);
						delete wo;
						new_obj = NULL;
					}
					else {
						new_obj = wo;
					}
					break;
				}

				// some old offsets will be converted to new ones
				case obj_t::old_fussgaenger:
					typ = obj_t::pedestrian;
					/* FALLTHROUGH */
				case obj_t::pedestrian:
				{
					pedestrian_t* const pedestrian = new pedestrian_t(file);
					if (pedestrian->get_desc() == NULL) {
						// no pedestrians ... delete this
						pedestrian->set_flag(obj_t::not_on_map);
						delete pedestrian;
						new_obj = NULL;
					}
					else {
						new_obj = pedestrian;
					}
					break;
				}

				case obj_t::old_verkehr:
					typ = obj_t::road_user;
					/* FALLTHROUGH */
				case obj_t::road_user:
				{
					private_car_t* const car = new private_car_t(file);
					if (car->get_desc() == NULL) {
						// no citycars ... delete this
						car->set_flag(obj_t::not_on_map);
						delete car;
					}
					else {
						new_obj = car;
					}
					break;
				}

				case obj_t::old_monoraildepot:
					typ = obj_t::monoraildepot;
					/* FALLTHROUGH */
				case obj_t::monoraildepot:
					new_obj = new monoraildepot_t(file);
					break;
				case obj_t::old_tramdepot:
					typ = obj_t::tramdepot;
					/* FALLTHROUGH */
				case obj_t::tramdepot:
					new_obj = new tramdepot_t(file);
					break;
				case obj_t::strassendepot:
					new_obj = new strassendepot_t(file);
					break;
				case obj_t::schiffdepot:
					new_obj = new schiffdepot_t(file);
					break;
				case obj_t::old_airdepot:
					typ = obj_t::airdepot;
					/* FALLTHROUGH */
				case obj_t::airdepot:
					new_obj = new airdepot_t(file);
					break;
				case obj_t::maglevdepot:
					new_obj = new maglevdepot_t(file);
					break;
				case obj_t::narrowgaugedepot:
					new_obj = new narrowgaugedepot_t(file);
					break;

				case obj_t::bahndepot:
				{
					// for compatibility reasons we may have to convert them to tram and monorail depots
					bahndepot_t*                   bd;
					gebaeude_t                     gb(file);
					building_tile_desc_t const* const tile = gb.get_tile();
					if(  tile  ) {
						switch (tile->get_desc()->get_extra()) {
							case monorail_wt: bd = new monoraildepot_t( gb.get_pos(), gb.get_owner(), tile); break;
							case tram_wt:     bd = new tramdepot_t(     gb.get_pos(), gb.get_owner(), tile); break;
							default:          bd = new bahndepot_t(     gb.get_pos(), gb.get_owner(), tile); break;
						}
					}
					else {
						bd = new bahndepot_t( gb.get_pos(), gb.get_owner(), NULL );
					}
					bd->rdwr_vehicles(file);
					new_obj   = bd;
					typ = new_obj->get_typ();
					// do not remove from this position, since there will be nothing
					gb.set_flag(obj_t::not_on_map);
				}
				break;

				// check for pillars
				case obj_t::old_pillar:
					typ = obj_t::pillar;
					/* FALLTHROUGH */
				case obj_t::pillar:
				{
					pillar_t *p = new pillar_t(file);
					if(p->get_desc()!=NULL  &&  p->get_desc()->get_pillar()!=0) {
						new_obj = p;
					}
					else {
						// has no pillar ...
						// do not remove from this position, since there will be nothing
						p->set_flag(obj_t::not_on_map);
						delete p;
					}
				}
				break;

				case obj_t::baum:
				{
					baum_t *b = new baum_t(file);
					if(  !b->get_desc()  ) {
						// is there a replacement possible
						if(  const tree_desc_t *desc = tree_builder_t::random_tree_for_climate( world()->get_climate_at_height(current_pos.z) )  ) {
							b->set_desc( desc );
						}
						else {
							// do not remove from map on this position, since there will be nothing
							b->set_flag(obj_t::not_on_map);
							delete b;
							b = NULL;
						}
					}
					else {
						new_obj = b;
					}
				}
				break;

				case obj_t::groundobj:
				{
					groundobj_t* const groundobj = new groundobj_t(file);
					if(groundobj->get_desc() == NULL) {
						// do not remove from this position, since there will be nothing
						groundobj->set_flag(obj_t::not_on_map);
						delete groundobj;
					}
					else {
						new_obj = groundobj;
					}
					break;
				}

				case obj_t::movingobj:
				{
					movingobj_t* const movingobj = new movingobj_t(file);
					if (movingobj->get_desc() == NULL) {
						// no citycars ... delete this
						movingobj->set_flag(obj_t::not_on_map);
						delete movingobj;
					}
					else {
						new_obj = movingobj;
					}
					break;
				}

				case obj_t::gebaeude:
				{
					gebaeude_t *gb = new gebaeude_t(file);
					if(gb->get_tile()==NULL) {
						// do not remove from this position, since there will be nothing
						gb->set_flag(obj_t::not_on_map);
						delete gb;
						gb = NULL;
					}
					else {
						new_obj = gb;
					}
				}
				break;

				case obj_t::old_roadsign:
					typ = obj_t::roadsign;
					/* FALLTHROUGH */
				case obj_t::roadsign:
				{
					roadsign_t *rs = new roadsign_t(file);
					if(rs->get_desc()==NULL) {
						// roadsign_t without description => ignore
						rs->set_flag(obj_t::not_on_map);
						delete rs;
					}
					else {
						new_obj = rs;
					}
				}
				break;

				// will be ignored, was only used before 86.09
				case obj_t::old_gebaeudefundament: { dummy_obj_t d(file); break; }

				// only factories can smoke; but then, the smoker is reinstated after loading
				case obj_t::raucher: { raucher_t r(file); break; }

				// wolke is not saved any more
				case obj_t::sync_wolke: { wolke_t w(file); break; }
				case obj_t::async_wolke: { async_wolke_t w(file); break; }

				default:
					dbg->fatal("objlist_t::laden()", "During loading: Unknown object type '%d'", typ);
			}

			if(new_obj  &&  new_obj->get_typ()!=typ) {
				dbg->warning( "objlist_t::rdwr()","typ error : %i instead %i on %i,%i, object ignored!", new_obj->get_typ(), typ, new_obj->get_pos().x, new_obj->get_pos().y );
				new_obj = NULL;
			}

			if(new_obj  &&  new_obj->get_pos()==koord3d::invalid) {
				new_obj->set_pos( current_pos );
			}

			if(new_obj  &&  new_obj->get_pos()!=current_pos) {
				dbg->warning("objlist_t::rdwr()","position error: %i,%i,%i instead %i,%i,%i (object will be ignored)",new_obj->get_pos().x,new_obj->get_pos().y,new_obj->get_pos().z,current_pos.x,current_pos.y,current_pos.z);
				new_obj = NULL;
			}

			if(new_obj) {
				append(new_obj);
			}
		}
	}
	else {
		/* here is the saving part ...
		 * first: construct a list of stuff really needed to save
		 */
		obj_t *save[256];
		sint32 max_object_index = 0;
		for(  uint16 i=0;  i<top;  i++  ) {
			obj_t *new_obj = bei((uint8)i);
			if(new_obj->get_typ()==obj_t::way
				// do not save smoke
				||  new_obj->get_typ()==obj_t::raucher
				||  new_obj->get_typ()==obj_t::sync_wolke
				||  new_obj->get_typ()==obj_t::async_wolke
				// fields will be built by factory
				||  new_obj->get_typ()==obj_t::field
				// do not save factory buildings => factory will reconstruct them
				||  (new_obj->get_typ()==obj_t::gebaeude  &&  ((gebaeude_t *)new_obj)->get_fabrik())
				// things with convoi will not be saved
				||  (new_obj->get_typ()>=66  &&  new_obj->get_typ()<82)
				||  (new_obj->get_typ()==obj_t::baum  &&  file->is_version_atleast(110, 1)  && (env_t::server  ||  file->is_version_atleast(122, 2))) // trees are saved from boden_t
			) {
				// these objects are simply not saved
			}
			else {
				save[max_object_index++] = new_obj;
			}
		}
		// now we know the number of stuff to save
		max_object_index --;
		if(  file->is_version_less(110, 1)  ) {
			file->rdwr_long( max_object_index );
		}
		else {
			uint8 obj_count = max_object_index+1;
			file->rdwr_byte( obj_count );
		}
		for(sint32 i=0; i<=max_object_index; i++) {
			obj_t *new_obj = save[i];
			if(new_obj->get_pos()==current_pos) {
				file->wr_obj_id(new_obj->get_typ());
				new_obj->rdwr(file);
			}
			else if (new_obj->get_pos().get_2d() == current_pos.get_2d()) {
				// ok, just error in z direction => we will correct it
				dbg->warning( "objlist_t::rdwr()","position error: z pos corrected on %i,%i from %i to %i", new_obj->get_pos().x, new_obj->get_pos().y, new_obj->get_pos().z, current_pos.z);
				file->wr_obj_id(new_obj->get_typ());
				new_obj->set_pos(current_pos);
				new_obj->rdwr(file);
			}
			else {
				dbg->error("objlist_t::rdwr()","unresolvable position error: %i,%i instead %i,%i (object type %i will be not saved!)", new_obj->get_pos().x, new_obj->get_pos().y, current_pos.x, current_pos.y, new_obj->get_typ());
				file->wr_obj_id(-1);
			}
		}
	}
}


/** display all things, faster, but will lead to clipping errors
 */
#ifdef MULTI_THREAD
void objlist_t::display_obj_quick_and_dirty( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const sint8 clip_num ) const
#else
void objlist_t::display_obj_quick_and_dirty( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const bool is_global ) const
#endif
{
	if(capacity==0) {
		return;
	}
	else if(capacity==1) {
		if(start_offset==0) {
			// only draw background on request
			obj.one->display( xpos, ypos  CLIP_NUM_PAR);
		}
		// foreground need to be drawn in any case
#ifdef MULTI_THREAD
		obj.one->display_after(xpos, ypos, clip_num );
#else
		obj.one->display_after( xpos, ypos, is_global );
		if(  is_global  ) {
			obj.one->clear_flag( obj_t::dirty );
		}
#endif
		return;
	}

	for(  uint8 n = start_offset;  n < top;  n++  ) {
		// is there an object ?
		obj.some[n]->display( xpos, ypos  CLIP_NUM_PAR);
	}
	// foreground (needs to be done backwards!
	for(  size_t n = top;  n-- != 0;    ) {
#ifdef MULTI_THREAD
		obj.some[n]->display_after( xpos, ypos, clip_num );
#else
		obj.some[n]->display_after( xpos, ypos, is_global );
		if(  is_global  ) {
			obj.some[n]->clear_flag( obj_t::dirty );
		}
#endif
	}
}


/**
 * Routine to display background images of non-moving things
 * powerlines have to be drawn after vehicles (and thus are in the obj-array inserted after vehicles)
 * @return the index of the first moving thing (or powerline)
 *
 * objlist_t::display_obj_bg() .. called by the methods in grund_t
 * local_display_obj_bg()        .. local function to avoid code duplication, returns false if the first non-valid obj is reached
 */
inline bool local_display_obj_bg(const obj_t *obj, const sint16 xpos, const sint16 ypos  CLIP_NUM_DEF)
{
	const bool display_obj = !obj->is_moving();
	if(  display_obj  ) {
		obj->display( xpos, ypos  CLIP_NUM_PAR);
	}
	return display_obj;
}

uint8 objlist_t::display_obj_bg( const sint16 xpos, const sint16 ypos, const uint8 start_offset  CLIP_NUM_DEF) const
{
	if(  start_offset >= top  ) {
		return start_offset;
	}

	if(  capacity == 1  ) {
		return local_display_obj_bg( obj.one, xpos, ypos  CLIP_NUM_PAR);
	}

	for(  uint8 n = start_offset;  n < top;  n++  ) {
		if(  !local_display_obj_bg( obj.some[n], xpos, ypos  CLIP_NUM_PAR)  ) {
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
 * objlist_t::display_obj_vh() .. called by the methods in grund_t
 * local_display_obj_vh()        .. local function to avoid code duplication, returns false if the first non-valid obj is reached
 */
inline bool local_display_obj_vh(const obj_t *draw_obj, const sint16 xpos, const sint16 ypos, const ribi_t::ribi ribi, const bool ontile  CLIP_NUM_DEF)
{
	vehicle_base_t const* const v = obj_cast<vehicle_base_t>(draw_obj);
	air_vehicle_t      const*       a;
	if(  v  &&  (ontile  ||  !(a = obj_cast<air_vehicle_t>(v))  ||  a->is_on_ground())  ) {
		const ribi_t::ribi veh_ribi = v->get_direction();
		if(  ontile  ||  (veh_ribi & ribi) == ribi  ||  (ribi_t::backward(veh_ribi) & ribi )== ribi  ||  draw_obj->get_typ() == obj_t::air_vehicle  ) {
			// activate clipping only for our direction masked by the ribi argument
			// use non-convex clipping (16) only if we are on the currently drawn tile or its n/w neighbours
			activate_ribi_clip( ((veh_ribi|ribi_t::backward(veh_ribi))&ribi) | (ontile  ||  ribi == ribi_t::north  ||  ribi == ribi_t::west ? 16 : 0)  CLIP_NUM_PAR);
			draw_obj->display( xpos, ypos  CLIP_NUM_PAR);
		}
		return true;
	}
	else {
		// if !ontile starting_offset is not correct, hence continue searching till the end
		return !ontile;
	}
}


uint8 objlist_t::display_obj_vh( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const ribi_t::ribi ribi, const bool ontile  CLIP_NUM_DEF) const
{
	if(  start_offset >= top  ) {
		return start_offset;
	}

	if(  capacity <= 1  ) {
		uint8 i = local_display_obj_vh( obj.one, xpos, ypos, ribi, ontile  CLIP_NUM_PAR);
		activate_ribi_clip( ribi_t::all  CLIP_NUM_PAR);
		return i;
	}

	uint8 nr_v = start_offset;
	for(  uint8 n = start_offset;  n < top;  n++  ) {
		if(  local_display_obj_vh( obj.some[n], xpos, ypos, ribi, ontile  CLIP_NUM_PAR)  ) {
			nr_v = n;
		}
		else {
			break;
		}
	}
	activate_ribi_clip( ribi_t::all  CLIP_NUM_PAR);
	return nr_v+1;
}


/**
 * Routine to draw foreground images of everything on the tile (no clipping) and powerlines
 * @param start_offset .. draws also background images of all objects with index>=start_offset
 * @param is_global will be only true for the main display; all miniworld windows should still reset main window
 */
#ifdef MULTI_THREAD
void objlist_t::display_obj_fg( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const sint8 clip_num ) const
#else
void objlist_t::display_obj_fg( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const bool is_global ) const
#endif
{
	if(  top == 0  ) {
		// nothing => finish
		return;
	}

	// now draw start_offset background and all foreground!
	if(  capacity == 1  ) {
		if(  start_offset == 0  ) {
			obj.one->display( xpos, ypos  CLIP_NUM_PAR);
		}
#ifdef MULTI_THREAD
		obj.one->display_after( xpos, ypos, clip_num );
#else
		obj.one->display_after( xpos, ypos, is_global );
		if(  is_global  ) {
			obj.one->clear_flag(obj_t::dirty);
		}
#endif
		return;
	}

	for(  uint8 n = start_offset;  n < top;  n++  ) {
		obj.some[n]->display( xpos, ypos  CLIP_NUM_PAR);
	}
	// foreground (needs to be done backwards!)
	for(  size_t n = top;  n-- != 0;    ) {
#ifdef MULTI_THREAD
		obj.some[n]->display_after( xpos, ypos, clip_num );
#else
		obj.some[n]->display_after( xpos, ypos, is_global );
		if(  is_global  ) {
			obj.some[n]->clear_flag( obj_t::dirty );
		}
#endif
	}
	return;
}


#ifdef MULTI_THREAD
void objlist_t::display_obj_overlay(const sint16 xpos, const sint16 ypos) const
{
	if(  top == 0  ) {
		return; // nothing => finish
	}

	if(  capacity == 1  ) {
		obj.one->display_overlay( xpos, ypos );
		obj.one->clear_flag( obj_t::dirty );
	}
	else {
		for(  size_t n = top;  n-- != 0;    ) {
			obj.some[n]->display_overlay( xpos, ypos );
			obj.some[n]->clear_flag( obj_t::dirty );
		}
	}
}
#endif


void objlist_t::check_season(const bool calc_only_season_change)
{
	if(  0 == top  ) {
		return;
	}

	if(  capacity <= 1  ) {
		// lets check here for consistency
		if(  top != capacity  ) {
			dbg->fatal( "objlist_t::check_season()", "top not matching!" );
		}
		obj_t *check_obj = obj.one;
		if(  !check_obj->check_season( calc_only_season_change )  ) {
			delete check_obj;
		}
	}
	else {
		// copy object pointers to check them
		vector_tpl<obj_t*> list;

		for(  uint8 i = 0;  i < top;  i++  ) {
			list.append(obj.some[i]);
		}
		// now work on the copied list
		// check_season may change this list (by planting new trees)
		for(  uint8 i = 0, end = list.get_count();  i < end;  i++  ) {
			obj_t *check_obj = list[i];
			if(  !check_obj->check_season( calc_only_season_change )  ) {
				delete check_obj;
			}
		}

	}
}
