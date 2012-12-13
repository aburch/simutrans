/*
 * author V. Meyer
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


static void dl_free(ding_t** p, uint8 size)
{
	assert(size > 1);
	if (size <= 16) {
		freelist_t::putback_node(sizeof(*p) * size, p);
	}
	else {
		guarded_free(p);
	}
}


static ding_t** dl_alloc(uint8 size)
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
	obj.one = NULL;
	capacity = 0;
	top = 0;
}


dingliste_t::~dingliste_t()
{
	if(capacity>1) {
		dl_free(obj.some, capacity);
	}
	obj.some = NULL;
	capacity = top = 0;
}


void dingliste_t::set_capacity(uint16 new_cap)
{
	// DBG_MESSAGE("dingliste_t::set_capacity()", "old cap=%d, new cap=%d", capacity, new_cap);

	if(new_cap>=255) {
		new_cap = 254;
	}

	// a single object is stored differentially
	if(new_cap==0) {
		if(capacity>1) {
			dl_free( obj.some, capacity );
		}
		capacity = 0;
		top = 0;
	}
	else if(new_cap==1) {
		if(capacity>1) {
			ding_t *tmp=NULL;
			// we have an obj to save into the list
			tmp = obj.some[0];
			dl_free( obj.some, capacity );
			obj.one = tmp;
			assert(top<2);
			capacity = 1;
		}
		else if(capacity==0) {
			assert(obj.one  &&  top==0);
		}
	}
	// a single object is stored differentially
	else if(capacity<=1  &&  new_cap>1) {
		ding_t *tmp=obj.one;
		obj.some = dl_alloc(new_cap);
		MEMZERON(obj.some, new_cap);
		obj.some[0] = tmp;
		capacity = new_cap;
		assert(top<=1);
	}
	else {
		// if we reach here, new_cap>1 and (capacity==0 or capacity>1)
		// ensure, we do not lose anything

		// get memory
		ding_t **tmp = dl_alloc(new_cap);

		// free old memory
		if(obj.some) {
			memcpy( tmp, obj.some, sizeof(ding_t *)*top );
			dl_free(obj.some, capacity);
		}
		obj.some = tmp;
	}
	capacity = new_cap;
}



bool dingliste_t::grow_capacity()
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
		// size exeeded, needs to extent
		uint16 new_cap = ((uint16)capacity+4)&0x0FC;
		set_capacity( new_cap );
		return true;
	}
}



void dingliste_t::shrink_capacity(uint8 o_top)
{
	// strategy: avoid free'ing mem if not neccesary. Only if we hold lots
	// of memory then free it.
	if(capacity > 16 && o_top <= 4) {
		set_capacity(o_top);
	}
}



inline void dingliste_t::intern_insert_at(ding_t* ding, uint8 pri)
{
	// we have more than one object here, thus we can use obj.some exclusively!
	for(  uint8 i=top;  i>pri;  i--  ) {
		obj.some[i] = obj.some[i-1];
	}
	obj.some[pri] = ding;
	top++;
}



// this will automatically give the right order for citycars and the like ...
bool dingliste_t::intern_add_moving(ding_t* ding)
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
	if(top>=index+count) {
		std::sort(&obj.some[index], &obj.some[index+count], compare_trees);
	}
}


bool dingliste_t::add(ding_t* ding)
{
	if(capacity==0) {
		// the first one save direct
		obj.one = ding;
		top = 1;
		capacity = 1;
		return true;
	}

	if(top>=capacity  &&  !grow_capacity()) {
		// memory exceeded
		return false;
	}
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

	uint8 i;
	for(  i=0;  i<top  &&  pri>type_to_pri[obj.some[i]->get_typ()];  i++  )
		;
	// now i contains the position, where we either insert of just add ...
	if(i==top) {
		obj.some[top] = ding;
		top++;
	}
	else {
		if(pri==baum_pri) {
			/* trees are a little tricky, since they cast a shadow
			 * therefore the y-order must be correct!
			 */
			for(  ;  i<top;  i++) {
				baum_t const* const tree = ding_cast<baum_t>(obj.some[i]);
				if (!tree  ||  compare_trees(ding, tree)) {
					break;
				}
			}
		}
		else if (pri == pillar_pri) {
			// pillars have to be sorted wrt their y-offset, too.
			for(  ;  i<top;  i++) {
				pillar_t const* const pillar = ding_cast<pillar_t>(obj.some[i]);
				if (!pillar  ||  ding->get_yoff()  > pillar->get_yoff() ) {
					break;
				}
			}
		}
		intern_insert_at(ding, i);
	}
	// then correct the upper border
	return true;
}



// take the thing out from the list
// use this only for temperary removing
// since it does not shrink list or checks for ownership
ding_t *dingliste_t::remove_last()
{
	ding_t *d=NULL;
	if(capacity==0) {
		// nothing
	}
	else if(capacity==1) {
		d = obj.one;
		obj.one = NULL;
		capacity = top = 0;
	}
	else {
		if(top>0) {
			top --;
			d = obj.some[top];
			obj.some[top] = NULL;
		}
	}
	return d;
}


bool dingliste_t::remove(const ding_t* ding)
{
	if(  capacity == 0  ) {
		return false;
	}
	else if(  capacity == 1  ) {
		if(  obj.one == ding  ) {
			obj.one = NULL;
			capacity = 0;
			top = 0;
			return true;
		}
		return false;
	}

	// we keep the array dense!
	for(  uint8 i=0;  i<top;  i++  ) {
		if(  obj.some[i] == ding  ) {
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
void local_delete_object(ding_t *ding, spieler_t *sp)
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


bool dingliste_t::loesche_alle(spieler_t *sp, uint8 offset)
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
const char *dingliste_t::kann_alle_entfernen(const spieler_t *sp, uint8 offset) const
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
void dingliste_t::calc_bild()
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
bool dingliste_t::ist_da(const ding_t* ding) const
{
	if(capacity<=1) {
		return obj.one==ding;
	}
	else {
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
	if(capacity==0) {
		return NULL;
	}
	else if(capacity==1) {
		return obj.one->get_typ()!=typ ? NULL : obj.one;
	}
	else if(start<top) {
		// else we have to search the list
		for(uint8 i=start; i<top; i++) {
			ding_t * tmp = obj.some[i];
			if(tmp->get_typ()==typ) {
				return tmp;
			}
		}
	}
	return NULL;
}



ding_t *dingliste_t::get_leitung() const
{
	if(capacity==0) {
		return NULL;
	}
	else if(capacity==1) {
		if(obj.one->get_typ()>=ding_t::leitung  &&  obj.one->get_typ()<=ding_t::senke) {
			return obj.one;
		}
	}
	else if(top>0) {
		// else we have to search the list
		for(uint8 i=0; i<top; i++) {
			uint8 typ = obj.some[i]->get_typ();
			if(typ>=ding_t::leitung  &&  typ<=ding_t::senke) {
				return obj.some[i];
			}
		}
	}
	return NULL;
}



ding_t *dingliste_t::get_convoi_vehicle() const
{
	if(capacity==0) {
		return NULL;
	}
	else if(capacity==1) {
		// could
		uint8 t = obj.one->get_typ();
		if(  t==ding_t::aircraft  ||  t==ding_t::schiff  ) {
			return obj.one;
		}
	}
	else if(top>0) {
		// else we have to search the list
		for(uint8 i=0; i<top; i++) {
			uint8 typ = obj.some[i]->get_typ();
			if(  typ>=ding_t::automobil  &&  typ<=ding_t::aircraft  ) {
				return obj.some[i];
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
	if(capacity==0) {
//		DBG_MESSAGE("dingliste_t::dump()","empty");
		return;
	}
	else if(capacity==1) {
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
	if(capacity==0) {
		return;
	}
	else if(capacity==1) {
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

	if(capacity==1) {
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

	if(capacity==1) {
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
	if(capacity==0) {
		return;
	}
	else if(capacity==1) {
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
void dingliste_t::check_season(const long month)
{
	slist_tpl<ding_t *>loeschen;

	if(capacity==0) {
		return;
	}
	else if(capacity==1) {
		ding_t *d = obj.one;
		if (!d->check_season(month)) {
			loeschen.insert( d );
		}
	}
	else {
		for(uint8 i=0; i<top; i++) {
			ding_t *d = obj.some[i];
			if (!d->check_season(month)) {
				loeschen.insert( d );
			}
		}
	}

	// delete all objects, which do not want to step anymore
	while (!loeschen.empty()) {
		delete loeschen.remove_first();
	}
}
