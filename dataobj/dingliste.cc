/* dingliste.cc
 *
 * author V. Meyer
 */

#include <stdio.h>

#include "../simdebug.h"
#include "dingliste.h"

#include "../dings/dummy.h"
#include "../dings/wolke.h"
#include "../dings/raucher.h"
#include "../dings/zeiger.h"
#include "../dings/baum.h"
#include "../dings/bruecke.h"
#include "../dings/pillar.h"
#include "../dings/tunnel.h"
#include "../dings/gebaeude.h"
#include "../dings/signal.h"
#ifdef LAGER_NOT_IN_USE
#include "../dings/lagerhaus.h"
#endif
#include "../dings/gebaeude.h"
#include "../dings/leitung2.h"
#include "../dings/wayobj.h"
#include "../dings/roadsign.h"

#include "../simtypes.h"
#include "../simdepot.h"
#include "../simmem.h"
#include "../simverkehr.h"
#include "../simpeople.h"
#include "../simplay.h"

#include "../besch/haus_besch.h"

#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/freelist.h"
#include "../dataobj/umgebung.h"

/* All things including ways are stored in this structure.
 * The entries are packed, i.e. the first free entry is at the top.
 * To save memory, a single element (like in the case for houses or a single tree)
 * is stored directly (obj.one) and capacity==1. Otherwise obj.some points to an
 * array.
 * The objects are sorted according to their drawing order.
 * ways are always first.
 */

// priority for entering into dingliste
// unused entries have 255
static uint8 type_to_pri[32]=
{
	255, //
	10, // baum
	100, // zeiger
	90, 90, 90,	// wolke
	2, 2, // buildings
	5, // signal
	1, 1, // bridge/tunnel
	255,
	2, 2, 2, // depots
	3, // smoke generator
	75, 2, 2, // powerlines
	5, // roadsign
	3, // pillar
	2, 2, 2, // depots
	255,
	4, // way objects (electrification)
	0, // ways (always at the top!)
	255, 255, 255, 255, 255
};

static void dl_free(void *p, uint8 size)
{
	if(size>1) {
		if(size<=16) {
			freelist_t::putback_node(sizeof(ding_t *)*size,p);
	  	}
	  	else {
			guarded_free(p);
		}
	}
}


static ding_t ** dl_alloc(uint8 size)
{
	ding_t **p=NULL;	// = NULL for compiler
	if(size>1) {
		if(size<=16) {
			p = (ding_t **)freelist_t::gimme_node(size*sizeof(ding_t *));
		}
		else {
			p = (ding_t **)guarded_malloc(size * sizeof(ding_t *));
		}
	}
	return (ding_t **)p;
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
}


void
dingliste_t::set_capacity(uint8 new_cap)
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
		memset( obj.some, 0, sizeof(ding_t*)*new_cap );
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



/**
 * @return ersten freien index
 * @author Hj. Malthaner
 */
int
dingliste_t::grow_capacity()
{
	if(capacity==0) {
		return true;
	}
	else if(capacity==1) {
		set_capacity( 4 );
		return true;
	}
	else if(capacity>=240) {
		// capacity exceeded ... (and no need for THAT many objects here ... )
		return false;
	}
	else {
		// size exeeded, needs to extent
		uint8 new_cap = ((uint16)capacity+4)&0x0FC;
		set_capacity( new_cap );
		return true;
	}
}



void
dingliste_t::shrink_capacity(uint8 o_top)
{
    // strategy: avoid free'ing mem if not neccesary. Only if we hold lots
    // of memory then free it.
	if(capacity > 16 && o_top <= 4) {
		set_capacity(o_top);
	}
}



inline
uint8 dingliste_t::intern_insert_at(ding_t *ding,uint8 pri)
{
#if 0
	memmove( obj.some+pri+1, obj.some+pri, sizeof(ding_t*)*(top-pri) );
	obj.some[pri] = ding;
	top ++;
#else
	// we have more than one object here, thus we can use obj.some exclusively!
	for(  uint8 i=top;  i>pri;  i--  ) {
		obj.some[i] = obj.some[i-1];
	}
	obj.some[pri] = ding;
	top++;
#endif
	return 1;
}



// this will automatically give the right order for citycars and the like ...
uint8
dingliste_t::intern_add_moving(ding_t *ding)
{
	// we are more than one object, thus we exclusively use obj.some here!
	// it would be nice, if also the objects are inserted according to their priorities as
	// vehicles types (number returned by gib_typ()). However, this would increase
	// the calculation even further. :(

	// find out about the first car etc. moving thing.
	// We can start to insert things after this index.
	uint8 start=0;
	while(start<top  &&   !obj.some[start]->is_moving()) {
		start ++;
	}

	// if we have two ways, the way at index 0 is ALWAYS the road!
	// however ships and planes may be where not way is below ...
	if(start!=0  &&  obj.some[0]->is_way()  &&  ((weg_t *)obj.some[0])->gib_waytype()==road_wt) {

		const uint8 fahrtrichtung = ((vehikel_t*)ding)->gib_fahrtrichtung();

		// this is very complicated:
		// we may have many objects in two lanes (actually five with tram and pedestrians)
		if(umgebung_t::drive_on_left) {

			// driving on left side
			if(fahrtrichtung<4) {	// nord, nordwest

				if((fahrtrichtung&(~ribi_t::suedost))==0) {
					// if we are going east we must be drawn as the first in east direction
					return intern_insert_at(ding,start);
				}
				else {
					// we must be drawn before south or west (thus insert after)
					for(uint8 i=start;  i<top;  i++  ) {
						if((((const vehikel_t*)obj.some[i])->gib_fahrtrichtung()&ribi_t::suedwest)!=0) {
							return intern_insert_at(ding,i);
						}
					}
					// nothing going southwest
					obj.some[top++] = ding;
					return true;
				}

			}
			else {
				// going south, west or the rest
				if((fahrtrichtung&(~ribi_t::suedost))==0) {
					// if we are going south or southeast we must be drawn as the first in east direction (after nord and nordeast)
					for(uint8 i=start;  i<top;  i++  ) {
						const ding_t *dt = obj.some[i];
						if(dt  &&  dt->is_moving()  &&  (((const vehikel_t*)dt)->gib_fahrtrichtung()&ribi_t::suedwest)!=0) {
							return intern_insert_at(ding,i);
						}
					}
				}
				// nothing going southeast
				obj.some[top++] = ding;
				return true;
			}
		}
		else {
			// driving on right side
			if(fahrtrichtung<4) {	// nord, ost, nordost

				if((fahrtrichtung&(~ribi_t::suedost))==0) {

					// if we are going east we must be drawn as the first in east direction (after nord and nordeast)
					for(uint8 i=start;  i<top;  i++  ) {
						if( (((const vehikel_t*)obj.some[i])->gib_fahrtrichtung()&ribi_t::nordost)!=0) {
							return intern_insert_at(ding,i);
						}
					}
					// nothing going to the east
				}
				// we must be drawn before south or west (thus append after)
				obj.some[top++] = ding;
				return true;

			}
			else {
				// going south, west or the rest

				if((fahrtrichtung&(~ribi_t::suedost))==0) {
					// going south or southeast, insert as first in this dirs
					return intern_insert_at(ding,start);
				}
				else {
					for(uint8 i=start;  i<top;  i++  ) {
						// west or northwest: append after all westwards
						if((((const vehikel_t*)obj.some[i])->gib_fahrtrichtung()&ribi_t::suedwest)==0) {
							return intern_insert_at(ding,i);
						}
					}
					// nothing going to nordeast
					obj.some[top++] = ding;
					return true;
				}
			}

		}	// right side/left side

	}
	else {
		// ok, we have to sort vehicles for correct overlapping,
		// but all vehicles are of the same typ, since this is track/channel etc. ONLY!

		// => much simpler to handle
		if((((vehikel_t*)ding)->gib_fahrtrichtung()&(~ribi_t::suedost))==0) {
			// if we are going east or south, we must be drawn before (i.e. put first)
			return intern_insert_at(ding,start);
		}
		else {
			// for north east we must be draw last
			obj.some[top++] = ding;
			return true;
		}
	}
	return false;
}



// this routine will automatically obey the correct order
// of things during insert into dingliste
uint8
dingliste_t::add(ding_t *ding)
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

	// vehicles need a special order
	if(ding->is_moving()) {
		return intern_add_moving(ding);
		return 1;
	}

	// now insert it a the correct place
	const uint8 pri=type_to_pri[ding->gib_typ()];

	// roads must be first!
	if(pri==0  &&  ((weg_t *)ding)->gib_waytype()==road_wt) {
		return intern_insert_at(ding, 0);
	}

	uint8 i;
	for(  i=0;  i<top  &&  pri>=type_to_pri[obj.some[i]->gib_typ()];  i++  )
		;
	// now i contains the position, where we either insert of just add ...
	if(i==top) {
		obj.some[top] = ding;
		top++;
	}
	else {
		intern_insert_at(ding, i);
	}
	// then correct the upper border
	return true;
}




// take the thing out from the list
// use this only for temperary removing
// since it does not shrink list or checks for ownership
ding_t *
dingliste_t::remove_last()
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


uint8 dingliste_t::remove(const ding_t* ding)
{
	if(capacity==0) {
		return false;
	} else if(capacity==1) {
		if(obj.one==ding) {
			obj.one = NULL;
			capacity = 0;
			top = 0;
			return true;
		}
		return false;
	}

	// we keep the array dense!
	for(uint8 i=0; i<top; i++) {

		if(obj.some[i]==ding) {
			// found it!
			top--;
			while(i<top) {
				obj.some[i] = obj.some[i+1];
				i++;
			}
			obj.some[top] = NULL;
			return true;
		}

	}

	return false;
}



bool
dingliste_t::loesche_alle(spieler_t *sp, uint8 offset)
{
	if(top<=offset) {
		return true;
	}

	// something to delete?
	bool ok=false;

	if(capacity>1) {
		for( unsigned i=offset;  i<top;  i++  ) {
			ding_t *dt = obj.some[i];
			dt->entferne(sp);
			dt->set_flag(ding_t::not_on_map);
			delete dt;
			obj.some[i] = NULL;
			ok = true;
		}
		top = offset;
	}
	else {
		if(capacity==1) {
			ding_t *dt = obj.one;
			dt->entferne(sp);
			dt->set_flag(ding_t::not_on_map);
			delete dt;
			ok = true;
			obj.one = NULL;
			capacity = top = 0;
		}
	}
	shrink_capacity(top);

	return ok;
}



/* returns the text of an error message, if obj could not be removed */
const char *
dingliste_t::kann_alle_entfernen(const spieler_t *sp) const
{
	if(capacity==0) {
		return NULL;
	}
	else if(capacity==1) {
		return obj.one->ist_entfernbar(sp);
	}
	else {
		const char * msg = NULL;

		for(uint8 i=0; i<top; i++) {
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
void
dingliste_t::calc_bild()
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



ding_t *
dingliste_t::suche(ding_t::typ typ,uint8 start) const
{
	if(capacity==0) {
		return NULL;
	}
	else if(capacity==1) {
		return obj.one->gib_typ()!=typ ? NULL : obj.one;
	}
	else if(start<top) {
		// else we have to search the list
		for(uint8 i=start; i<top; i++) {
			ding_t * tmp = obj.some[i];
			if(tmp->gib_typ()==typ) {
				return tmp;
			}
		}
	}
	return NULL;
}



void
dingliste_t::rdwr(karte_t *welt, loadsave_t *file, koord3d current_pos)
{
	long max_object_index;

	if(file->is_saving()) {
		max_object_index = top-1;
	}
	file->rdwr_long(max_object_index, "\n");

	if(max_object_index>254) {
		dbg->error("dingliste_t::laden()","Too many objects (%i) at (%i,%i), some vehicle may not appear immediately.",max_object_index,current_pos.x,current_pos.y);
	}

	for(int i=0; i<=max_object_index; i++) {
		if(file->is_loading()) {
			ding_t::typ typ = (ding_t::typ)file->rd_obj_id();
			// DBG_DEBUG("dingliste_t::laden()", "Thing type %d", typ);

			if(typ == -1) {
				continue;
			}

			ding_t *d = NULL;

			switch(typ) {
				case ding_t::old_verkehr: typ = ding_t::verkehr;
				case ding_t::verkehr:	    d = new stadtauto_t (welt, file);		break;
				case ding_t::old_fussgaenger: typ = ding_t::fussgaenger;
				case ding_t::fussgaenger: d = new fussgaenger_t (welt, file);	        break;
				case ding_t::bruecke:	    d = new bruecke_t (welt, file);	        break;
				case ding_t::tunnel:	    d = new tunnel_t (welt, file);	        break;
				case ding_t::pumpe:		    d = new pumpe_t (welt, file);	        break;
				case ding_t::leitung:	    d = new leitung_t (welt, file);	        break;
				case ding_t::senke:		    d = new senke_t (welt, file);	        break;
				case ding_t::wayobj:	    d = new wayobj_t (welt, file);	        break;
				case ding_t::zeiger:	    d = new zeiger_t (welt, file);	        break;
				case ding_t::signal:	    d = new signal_t (welt, file);   break;
				case ding_t::old_roadsign: typ = ding_t::roadsign;
				case ding_t::roadsign:	  d = new roadsign_t (welt, file); break;

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

				case ding_t::bahndepot:
				{
					// for compatibilty reasons ...
					gebaeude_t *gb = new gebaeude_t(welt, file);
					if(gb->gib_tile()->gib_besch()==hausbauer_t::monorail_depot_besch) {
						monoraildepot_t *md = new monoraildepot_t(welt,gb->gib_pos(),(spieler_t *)NULL,gb->gib_tile());
						md->rdwr_vehicles(file);
						d = md;
					}
					else if(gb->gib_tile()->gib_besch()==hausbauer_t::tram_depot_besch) {
						tramdepot_t *td = new tramdepot_t(welt,gb->gib_pos(),(spieler_t *)NULL,gb->gib_tile());
						td->rdwr_vehicles(file);
						d = td;
					}
					else {
						bahndepot_t *bd = new bahndepot_t(welt,gb->gib_pos(),(spieler_t *)NULL,gb->gib_tile());
						bd->rdwr_vehicles(file);
						d = bd;
					}
					d->setze_besitzer( gb->gib_besitzer() );
					gb->gib_besitzer()->add_maintenance(umgebung_t::maint_building);
					typ = d->gib_typ();
					delete gb;
				}
				break;

				// check for pillars
				case ding_t::old_pillar:
					typ = ding_t::pillar;
				case ding_t::pillar:
				{
					pillar_t *p = new pillar_t(welt, file);
					if(p->gib_besch()!=NULL  &&  p->gib_besch()->gib_pillar()!=0) {
						d = p;
					}
					else {
						// has no pillar ...
						delete p;
					}
				}
				break;

				case ding_t::baum:
				{
					baum_t *b = new baum_t(welt, file);
					if(!b->gib_besch()) {
						delete b;
					}
					d = b;
				}
				break;

				case ding_t::gebaeude:
				{
					// Wenn es ein Gebäude nicht mehr gibt, geänderte Tabfiles
					// lassen wir es einfach weg. Das gibt zwar Fundamente ohne
					// Aufbau, stürzt aber nicht ab.
					gebaeude_t *gb = new gebaeude_t (welt, file);
					if(!gb->gib_tile()) {
						delete gb;
						gb  = 0;
					}
					d = gb;
				}
				break;

				// will be ignored, was only used before 86.09
				case ding_t::old_gebaeudefundament: d = new dummy_ding_t(welt, file); delete d; d=NULL;  break;

				// only factories can smoke; but then, the smoker is reinstated after loading
				case ding_t::raucher: 	d = new raucher_t (welt, file); delete d; d = NULL; break;

				// wolke saves wrong images; but new smoke will emerge anyway ...
				case ding_t::sync_wolke:	    d = new sync_wolke_t (welt, file); delete d; d=NULL; break;
				case ding_t::async_wolke:	    d = new async_wolke_t (welt, file); delete d; d=NULL; break;

#ifdef LAGER_NOT_IN_USE
				case ding_t::lagerhaus:	    d = new lagerhaus_t (welt, file);	        break;
#endif

				default:
					dbg->warning("dingliste_t::laden()", "Error while loading game:");
					dbg->fatal("dingliste_t::laden()", "Unknown object type '%d'", typ);
			}

			if(d  &&  d->gib_typ()!=typ) {
				DBG_DEBUG("dingliste_t::rdwr()","typ error: %i instead %i",d->gib_typ(),typ);
				DBG_DEBUG("dingliste_t::rdwr()","typ error on %i,%i, object ignored!",d->gib_pos().x, d->gib_pos().y);
				d = NULL;
			}

			if(d  &&  d->gib_pos()!=current_pos) {
				DBG_DEBUG("dingliste_t::rdwr()","position error: %i,%i instead %i,%i",d->gib_pos().x,d->gib_pos().y,current_pos.x,current_pos.y);
				DBG_DEBUG("dingliste_t::rdwr()","loaded object ignored!");
				d = NULL;
			}

			if(d) {
				add(d);
			}
		}
		else {
			// here is the saving part ...
			ding_t *d=bei(i);
			assert(d);
			if(d->gib_pos()==current_pos) {
				if(d->gib_typ()!=ding_t::raucher  &&  !d->is_way()) {
					bei(i)->rdwr(file);
				}
				else {
					file->wr_obj_id(-1);
				}
			}
			else if(bei(i)->gib_pos().gib_2d()==current_pos.gib_2d()) {
				// ok, just error in z direction => we will correct it
				DBG_DEBUG("dingliste_t::rdwr()","position error: z pos corrected on %i,%i from %i to %i",bei(i)->gib_pos().x,bei(i)->gib_pos().y,bei(i)->gib_pos().z,current_pos.z);
				bei(i)->setze_pos( current_pos );
				bei(i)->rdwr(file);
			}
			else {
				DBG_DEBUG("dingliste_t::rdwr()","position error: %i,%i instead %i,%i",bei(i)->gib_pos().x,bei(i)->gib_pos().y,current_pos.x,current_pos.y);
				DBG_DEBUG("dingliste_t::rdwr()","object not saved!");
				file->wr_obj_id(-1);
			}
		}
	}
}



/* display all things, much faster to do it here ...
 * reset_dirty will be only true for the main display; all miniworld windows should still reset main window ...
 *  @author prissi
 */
void dingliste_t::display_dinge( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const bool reset_dirty ) const
{
	if(capacity==0) {
		return;
	}
	else if(capacity==1) {
		if(start_offset==0) {
			obj.one->display(xpos, ypos, reset_dirty );
			obj.one->display_after(xpos, ypos, reset_dirty );
			if(reset_dirty) {
				obj.one->clear_flag(ding_t::dirty);
			}
		}
		return;
	}

	for(uint8 n=start_offset; n<top; n++) {
		// ist dort ein objekt ?
		obj.some[n]->display(xpos, ypos, reset_dirty );
	}
	// foreground (needs to be done backwards!
	for(int n=top-1; n>=0;  n--) {
		obj.some[n]->display_after(xpos, ypos, reset_dirty );
		obj.some[n]->clear_flag(ding_t::dirty);
	}
}



// animation, waiting for crossing, all things that could take a while should be done in a step
void
dingliste_t::step(const long delta_t, const int steps)
{
	static slist_tpl<ding_t *>loeschen;

	if(capacity==0) {
		return;
	}
	else if(capacity==1) {
		ding_t *d = obj.one;
		const int freq = d->step_frequency;
		if(freq!=0  &&  (steps&freq)==0  &&  d->step(delta_t*freq)==false) {
			loeschen.insert( d );
		}
	}
	else {
		for(uint8 i=0; i<top; i++) {
			ding_t *d = obj.some[i];
			const int freq = d->step_frequency;
			if(freq!=0  &&  (steps&freq)==0  &&  d->step(delta_t*freq)==false) {
				loeschen.insert( d );
			}
		}
	}

	// delete all objects, which do not want to step anymore
	while(loeschen.count()) {
		delete loeschen.remove_first();
	}
}




// start next month (good for toogling a seasons)
void
dingliste_t::check_season(const long month)
{
	static slist_tpl<ding_t *>loeschen;

	if(capacity==0) {
		return;
	}
	else if(capacity==1) {
		ding_t *d = obj.one;
		if(d->check_season(month)==false) {
			loeschen.insert( d );
		}
	}
	else {
		for(uint8 i=0; i<top; i++) {
			ding_t *d = obj.some[i];
			if(d->check_season(month)==false) {
				loeschen.insert( d );
			}
		}
	}

	// delete all objects, which do not want to step anymore
	while(loeschen.count()) {
		delete loeschen.remove_first();
	}
}
