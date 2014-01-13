/*
 * author V. Meyer
 */

#include <stdio.h>
#include <algorithm>

#include "../simdebug.h"
#include "../display/simgraph.h"
#include "../simworld.h"

#include "../bauer/hausbauer.h"
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
#include "../dataobj/environment.h"

#include "objlist.h"

/* All things including ways are stored in this structure.
 * The entries are packed, i.e. the first free entry is at the top.
 * To save memory, a single element (like in the case for houses or a single tree)
 * is stored directly (obj.one) and capacity==1. Otherwise obj.some points to an
 * array.
 * The objects are sorted according to their drawing order.
 * ways are always first.
 */

#define baum_pri (50)
#define pillar_pri (7)

// priority of moving things: should be smaller than the priority of powerlines
#define moving_obj_pri (100)

// priority for entering into objlist
// unused entries have 255
static uint8 type_to_pri[256]=
{
	255, //
	baum_pri, // tree
	254, // cursor/pointers
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
	moving_obj_pri,	// road vehicle
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


static void dl_free(obj_t** p, uint8 size)
{
	assert(size > 1);
	if (size <= 16) {
		freelist_t::putback_node(sizeof(*p) * size, p);
	}
	else {
		guarded_free(p);
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
	if(capacity>1) {
		dl_free(obj.some, capacity);
	}
	obj.some = NULL;
	capacity = top = 0;
}


void objlist_t::set_capacity(uint16 new_cap)
{
	// DBG_MESSAGE("objlist_t::set_capacity()", "old cap=%d, new cap=%d", capacity, new_cap);

	if(new_cap>=255) {
		new_cap = 254;
	}

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
	// a single object is stored differentially
	else if(capacity<=1  &&  new_cap>1) {
		// if we reach here, new_cap>1 and (capacity==0 or capacity>1)
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



// this will automatically give the right order for citycars and the like ...
bool objlist_t::intern_add_moving(obj_t* new_obj)
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
	while(  end>start  &&  !obj.some[end-1]->is_moving()  ) {
		end--;
	}
	if(start==end) {
		intern_insert_at(new_obj, start);
		return true;
	}

	// if we have two ways, the way at index 0 is ALWAYS the road!
	// however ships and planes may be where not way is below ...
	if(start!=0  &&  obj.some[0]->get_typ()==obj_t::way  &&  ((weg_t *)obj.some[0])->get_waytype()==road_wt) {

		const uint8 fahrtrichtung = ((vehikel_basis_t*)new_obj)->get_fahrtrichtung();

		// this is very complicated:
		// we may have many objects in two lanes (actually five with tram and pedestrians)
		if(world()->get_settings().is_drive_left()) {

			// driving on left side
			if(fahrtrichtung<4) {	// north, northwest

				if((fahrtrichtung&(~ribi_t::suedost))==0) {
					// if we are going east we must be drawn as the first in east direction
					intern_insert_at(new_obj, start);
					return true;
				}
				else {
					// we must be drawn before south or west (thus insert after)
					for(uint8 i=start;  i<end;  i++  ) {
						if((((const vehikel_t*)obj.some[i])->get_fahrtrichtung()&ribi_t::suedwest)!=0) {
							intern_insert_at(new_obj, i);
							return true;
						}
					}
					// nothing going southwest
					intern_insert_at(new_obj, end);
					return true;
				}

			}
			else {
				// going south, west or the rest
				if((fahrtrichtung&(~ribi_t::suedost))==0) {
					// if we are going south or southeast we must be drawn as the first in east direction (after north and northeast)
					for(uint8 i=start;  i<end;  i++  ) {
						if (obj_t const* const dt = obj.some[i]) {
							if (vehikel_basis_t const* const v = obj_cast<vehikel_basis_t>(dt)) {
								if ((v->get_fahrtrichtung() & ribi_t::suedwest) != 0) {
									intern_insert_at(new_obj, i);
									return true;
								}
							}
						}
					}
				}
				// nothing going southeast
				intern_insert_at(new_obj, end);
				return true;
			}
		}
		else {
			// driving on right side
			if(fahrtrichtung<4) {	// north, east, northeast

				if((fahrtrichtung&(~ribi_t::suedost))==0) {

					// if we are going east we must be drawn as the first in east direction (after north and northeast)
					for(uint8 i=start;  i<end;  i++  ) {
						if( (((const vehikel_t*)obj.some[i])->get_fahrtrichtung()&ribi_t::nordost)!=0) {
							intern_insert_at(new_obj, i);
							return true;
						}
					}
					// nothing going to the east
				}
				// we must be drawn before south or west (thus append after)
				intern_insert_at(new_obj, end);
				return true;

			}
			else {
				// going south, west or the rest

				if((fahrtrichtung&(~ribi_t::suedost))==0) {
					// going south or southeast, insert as first in this dirs
					intern_insert_at(new_obj, start);
					return true;
				}
				else {
					for(uint8 i=start;  i<end;  i++  ) {
						// west or northwest: append after all westwards
						if((((const vehikel_t*)obj.some[i])->get_fahrtrichtung()&ribi_t::suedwest)==0) {
							intern_insert_at(new_obj, i);
							return true;
						}
					}
					// nothing going to northeast
					intern_insert_at(new_obj, end);
					return true;
				}
			}

		}	// right side/left side

	}
	else {
		// ok, we have to sort vehicles for correct overlapping,
		// but all vehicles are of the same typ, since this is track/channel etc. ONLY!

		// => much simpler to handle
		if((((vehikel_t*)new_obj)->get_fahrtrichtung()&(~ribi_t::suedost))==0) {
			// if we are going east or south, we must be drawn before (i.e. put first)
			intern_insert_at(new_obj, start);
			return true;
		}
		else {
			// for north east we must be draw last
			intern_insert_at(new_obj, end);
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
	if(pri==moving_obj_pri) {
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
		if(pri==baum_pri) {
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
		else if (pri == pillar_pri) {
			// pillars have to be sorted wrt their y-offset, too.
			for(  ;  i<top;  i++) {
				pillar_t const* const pillar = obj_cast<pillar_t>(obj.some[i]);
				if (!pillar  ||  new_obj->get_yoff()  > pillar->get_yoff() ) {
					break;
				}
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
void local_delete_object(obj_t *remove_obj, spieler_t *sp)
{
	vehikel_basis_t* const v = obj_cast<vehikel_basis_t>(remove_obj);
	if (v  &&  remove_obj->get_typ() != obj_t::fussgaenger  &&  remove_obj->get_typ() != obj_t::verkehr  &&  remove_obj->get_typ() != obj_t::movingobj) {
		v->verlasse_feld();
	}
	else {
		remove_obj->entferne(sp);
		remove_obj->set_flag(obj_t::not_on_map);
		// all objects except zeiger (pointer) are destroyed here
		// zeiger's will be deleted if their associated werkzeug_t (tool) terminates
		if (remove_obj->get_typ() != obj_t::zeiger) {
			delete remove_obj;
		}
	}
}


bool objlist_t::loesche_alle(spieler_t *sp, uint8 offset)
{
	if(top<=offset) {
		return false;
	}

	// something to delete?
	bool ok=false;

	if(capacity>1) {
		while(  top>offset  ) {
			top --;
			local_delete_object(obj.some[top], sp);
			obj.some[top] = NULL;
			ok = true;
		}
	}
	else {
		if(capacity==1) {
			local_delete_object(obj.one, sp);
			ok = true;
			obj.one = NULL;
			capacity = top = 0;
		}
	}
	shrink_capacity(top);

	return ok;
}



/* returns the text of an error message, if obj could not be removed */
const char *objlist_t::kann_alle_entfernen(const spieler_t *sp, uint8 offset) const
{
	if(top<=offset) {
		return NULL;
	}

	if(capacity==1) {
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
void objlist_t::calc_bild()
{
	if(capacity==0) {
		// nothing
	}
	else if(capacity==1) {
		obj.one->calc_bild();
	}
	else {
		for(uint8 i=0; i<top; i++) {
			obj.some[i]->calc_bild();
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
		if(  t == obj_t::aircraft  ||  t == obj_t::schiff  ) {
			return obj.one;
		}
	}
	else {
		for(  uint8 i=0;  i < top;  i++  ) {
			uint8 typ = obj.some[i]->get_typ();
			if(  typ >= obj_t::automobil  &&  typ <= obj_t::aircraft  ) {
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
		if(  file->get_version()<=110000  ) {
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
				case obj_t::bruecke:	    new_obj = new bruecke_t(file);	        break;
				case obj_t::tunnel:	    new_obj = new tunnel_t(file);	        break;
				case obj_t::pumpe:		    new_obj = new pumpe_t(file);	        break;
				case obj_t::leitung:	    new_obj = new leitung_t(file);	        break;
				case obj_t::senke:		    new_obj = new senke_t(file);	        break;
				case obj_t::zeiger:	    new_obj = new zeiger_t(file);	        break;
				case obj_t::signal:	    new_obj = new signal_t(file);   break;
				case obj_t::label:			new_obj = new label_t(file); break;
				case obj_t::crossing:		new_obj = new crossing_t(file); break;

				case obj_t::wayobj:
				{
					wayobj_t* const wo = new wayobj_t(file);
					if (wo->get_besch() == NULL) {
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
					typ = obj_t::fussgaenger;
				case obj_t::fussgaenger:
				{
					fussgaenger_t* const pedestrian = new fussgaenger_t(file);
					if (pedestrian->get_besch() == NULL) {
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
					typ = obj_t::verkehr;
				case obj_t::verkehr:
				{
					stadtauto_t* const car = new stadtauto_t(file);
					if (car->get_besch() == NULL) {
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
				case obj_t::monoraildepot:
					new_obj = new monoraildepot_t(file);
					break;
				case obj_t::old_tramdepot:
					typ = obj_t::tramdepot;
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
					haus_tile_besch_t const* const tile = gb.get_tile();
					if(  tile  ) {
						switch (tile->get_besch()->get_extra()) {
							case monorail_wt: bd = new monoraildepot_t( gb.get_pos(), gb.get_besitzer(), tile); break;
							case tram_wt:     bd = new tramdepot_t(     gb.get_pos(), gb.get_besitzer(), tile); break;
							default:          bd = new bahndepot_t(     gb.get_pos(), gb.get_besitzer(), tile); break;
						}
					}
					else {
						bd = new bahndepot_t( gb.get_pos(), gb.get_besitzer(), NULL );
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
				case obj_t::pillar:
				{
					pillar_t *p = new pillar_t(file);
					if(p->get_besch()!=NULL  &&  p->get_besch()->get_pillar()!=0) {
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
					if(  !b->get_besch()  ) {
						// is there a replacement possible
						if(  const baum_besch_t *besch = baum_t::random_tree_for_climate( world()->get_climate_at_height(current_pos.z) )  ) {
							b->set_besch( besch );
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
					if(groundobj->get_besch() == NULL) {
						// do not remove from this position, since there will be nothing
						groundobj->set_flag(obj_t::not_on_map);
						// not use entferne, since it would try to lookup besch
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
					if (movingobj->get_besch() == NULL) {
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
				case obj_t::roadsign:
				{
					roadsign_t *rs = new roadsign_t(file);
					if(rs->get_besch()==NULL) {
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
				add(new_obj);
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
				||  (env_t::server  &&  new_obj->get_typ()==obj_t::baum  &&  file->get_version()>=110001)
			) {
				// these objects are simply not saved
			}
			else {
				save[max_object_index++] = new_obj;
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



/* Dumps a short info about the things on this tile
 *  @author prissi
 */
void objlist_t::dump() const
{
	if(capacity==0) {
//		DBG_MESSAGE("objlist_t::dump()","empty");
		return;
	}
	else if(capacity==1) {
		DBG_MESSAGE("objlist_t::dump()","one object \'%s\' owned by sp %p", obj.one->get_name(), obj.one->get_besitzer() );
		return;
	}

	DBG_MESSAGE("objlist_t::dump()","%i objects", top );
	for(uint8 n=0; n<top; n++) {
		DBG_MESSAGE( obj.some[n]->get_name(), "at %i owned by sp %p", n, obj.some[n]->get_besitzer() );
	}
}


/** display all things, faster, but will lead to clipping errors
 *  @author prissi
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
#ifdef MULTI_THREAD
			obj.one->display( xpos, ypos, clip_num );
			obj.one->display_after(xpos, ypos, clip_num );
#else
			obj.one->display( xpos, ypos );
			obj.one->display_after( xpos, ypos, is_global );
			if(  is_global  ) {
				obj.one->clear_flag( obj_t::dirty );
			}
#endif
		}
		return;
	}

	for(  uint8 n = start_offset;  n < top;  n++  ) {
		// is there an object ?
#ifdef MULTI_THREAD
		obj.some[n]->display( xpos, ypos, clip_num );
#else
		obj.some[n]->display( xpos, ypos );
#endif
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
 * @author Dwachs
 */
#ifdef MULTI_THREAD
inline bool local_display_obj_bg(const obj_t *obj, const sint16 xpos, const sint16 ypos, const sint8 clip_num)
#else
inline bool local_display_obj_bg(const obj_t *obj, const sint16 xpos, const sint16 ypos)
#endif
{
	const bool display_obj = !obj->is_moving();
	if(  display_obj  ) {
#ifdef MULTI_THREAD
		obj->display( xpos, ypos, clip_num );
#else
		obj->display( xpos, ypos );
#endif
	}
	return display_obj;
}


#ifdef MULTI_THREAD
uint8 objlist_t::display_obj_bg( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const sint8 clip_num) const
#else
uint8 objlist_t::display_obj_bg( const sint16 xpos, const sint16 ypos, const uint8 start_offset) const
#endif
{
	if(  start_offset >= top  ) {
		return start_offset;
	}

	if(  capacity == 1  ) {
#ifdef MULTI_THREAD
		return local_display_obj_bg( obj.one, xpos, ypos, clip_num );
#else
		return local_display_obj_bg( obj.one, xpos, ypos );
#endif
	}

	for(  uint8 n = start_offset;  n < top;  n++  ) {
#ifdef MULTI_THREAD
		if(  !local_display_obj_bg( obj.some[n], xpos, ypos, clip_num )  ) {
#else
		if(  !local_display_obj_bg( obj.some[n], xpos, ypos )  ) {
#endif
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
 * @author Dwachs
 */
#ifdef MULTI_THREAD
inline bool local_display_obj_vh( obj_t *draw_obj, const sint16 xpos, const sint16 ypos, const ribi_t::ribi ribi, const bool ontile, const sint8 clip_num)
#else
inline bool local_display_obj_vh(const obj_t *draw_obj, const sint16 xpos, const sint16 ypos, const ribi_t::ribi ribi, const bool ontile)
#endif
{
	vehikel_basis_t const* const v = obj_cast<vehikel_basis_t>(draw_obj);
	aircraft_t      const*       a;
	if(  v  &&  (ontile  ||  !(a = obj_cast<aircraft_t>(v))  ||  a->is_on_ground())  ) {
		const ribi_t::ribi veh_ribi = v->get_fahrtrichtung();
		if(  ontile  ||  (veh_ribi & ribi) == ribi  ||  (ribi_t::rueckwaerts(veh_ribi) & ribi )== ribi  ||  draw_obj->get_typ() == obj_t::aircraft  ) {
			// activate clipping only for our direction masked by the ribi argument
			// use non-convex clipping (16) only if we are on the currently drawn tile or its n/w neighbours
#ifdef MULTI_THREAD
			activate_ribi_clip( ((veh_ribi|ribi_t::rueckwaerts(veh_ribi))&ribi) | (ontile  ||  ribi == ribi_t::nord  ||  ribi == ribi_t::west ? 16 : 0), clip_num );
			draw_obj->display( xpos, ypos, clip_num );
#else
			activate_ribi_clip( ((veh_ribi|ribi_t::rueckwaerts(veh_ribi))&ribi) | (ontile  ||  ribi == ribi_t::nord  ||  ribi == ribi_t::west ? 16 : 0) );
			draw_obj->display( xpos, ypos );
#endif
		}
		return true;
	}
	else {
		// if !ontile starting_offset is not correct, hence continue searching till the end
		return !ontile;
	}
}


#ifdef MULTI_THREAD
uint8 objlist_t::display_obj_vh( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const ribi_t::ribi ribi, const bool ontile, const sint8 clip_num ) const
#else
uint8 objlist_t::display_obj_vh( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const ribi_t::ribi ribi, const bool ontile ) const
#endif
{
	if(  start_offset >= top  ) {
		return start_offset;
	}

	if(  capacity <= 1  ) {
#ifdef MULTI_THREAD
		uint8 i = local_display_obj_vh( obj.one, xpos, ypos, ribi, ontile, clip_num );
		activate_ribi_clip( ribi_t::alle, clip_num );
#else
		uint8 i = local_display_obj_vh( obj.one, xpos, ypos, ribi, ontile );
		activate_ribi_clip();
#endif
		return i;
	}

	uint8 nr_v = start_offset;
	for(  uint8 n = start_offset;  n < top;  n++  ) {
#ifdef MULTI_THREAD
		if(  local_display_obj_vh( obj.some[n], xpos, ypos, ribi, ontile, clip_num )  ) {
#else
		if(  local_display_obj_vh( obj.some[n], xpos, ypos, ribi, ontile )  ) {
#endif
			nr_v = n;
		}
		else {
			break;
		}
	}
#ifdef MULTI_THREAD
	activate_ribi_clip( ribi_t::alle, clip_num );
#else
	activate_ribi_clip();
#endif
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
#ifdef MULTI_THREAD
			obj.one->display( xpos, ypos, clip_num );
#else
			obj.one->display( xpos, ypos );
#endif
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
#ifdef MULTI_THREAD
		obj.some[n]->display( xpos, ypos, clip_num );
#else
		obj.some[n]->display( xpos, ypos );
#endif
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


// start next month (good for toggling seasons)
void objlist_t::check_season(const long month)
{
	if(  0 == top  ) {
		return;
	}

	if(  capacity<=1  ) {
		// lets check here for consistency
		if(  top!=capacity  ) {
			dbg->fatal( "objlist_t::check_season()", "top not matching!" );
		}
		obj_t *check_obj = obj.one;
		if(  !check_obj->check_season(month)  ) {
			delete check_obj;
		}
	}
	else {
		// only here delete list is needed!
		slist_tpl<obj_t *>to_remove;

		for(  uint8 i=0;  i<top;  i++  ) {
			obj_t *check_obj = obj.some[i];
			if(  !check_obj->check_season(month)  ) {
				to_remove.insert( check_obj );
			}
		}

		// delete all objects, which do not want to step any more
		while(  !to_remove.empty()  ) {
			delete to_remove.remove_first();
		}
	}
}
