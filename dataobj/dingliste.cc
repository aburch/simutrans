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
//#include "../dings/gebaeudefundament.h"
#include "../dings/gebaeude.h"
#include "../dings/signal.h"
#ifdef LAGER_NOT_IN_USE
#include "../dings/lagerhaus.h"
#endif
#include "../dings/gebaeude.h"
//#include "../dings/leitung.h"
#include "../dings/leitung2.h"
#include "../dings/oberleitung.h"
#include "../dings/roadsign.h"

#include "../simdepot.h"
#include "../simmem.h"
#include "../simverkehr.h"
#include "../simpeople.h"

#include "../besch/haus_besch.h"

#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/freelist.h"


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
    // Paranoia
    obj.one = 0;
    capacity = 0;
    top = 0;
}


void
dingliste_t::set_capacity(unsigned new_cap)
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
	}
	else if(new_cap==1) {
		if(capacity>1) {
			ding_t *tmp=NULL;
			// we have an obj to save into the list
			if(top>0) {
				tmp = obj.some[top-1];
			}
			dl_free( obj.some, capacity );
			obj.one = tmp;
		}
		else if(capacity==0) {
			obj.one = NULL;
		}
		capacity = 1;
	}
	// a single object is stored differentially
	else if(capacity==1  &&  new_cap>1) {
		ding_t *tmp=obj.one;
		if(new_cap<top) {
			new_cap = top;
		}
		obj.some = dl_alloc(new_cap);
		memset( obj.some, 0, sizeof(ding_t*)*new_cap );
		obj.some[top-1] = tmp;
		capacity = new_cap;
	}
	else {
		// if we reach here, new_cap>1 and (capacity==0 or capacity>1)
		// ensure, we do not lose anything
		if(new_cap<top) {
			new_cap = top;
		}

		// get memory
		ding_t **tmp = dl_alloc(new_cap);

		// copy list
		const uint8 end = MIN(capacity, new_cap);
		uint8 i=0;
		for(; i<end; i++) {
			tmp[i] = obj.some[i];
		}
		// init list
		for(  ;  i<new_cap;  i++  ) {
			tmp[i] = 0;
		}
		// free old memory
		if(obj.some) {
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
dingliste_t::grow_capacity(uint8 pri)
{
	if(capacity==0) {
		// nothing in the list?
		// since the first one comes for free, we can always add it
		capacity = 1;
		// paranoia
		top = 0;
		obj.one = NULL;
		return pri;
	}
	else if(capacity==1) {
		uint8 new_cap = (top>pri+1) ? top : pri;
		set_capacity( (new_cap+4)&0xFC );
		return (top-1==pri) ? pri+1 : pri;
	}
	else if(capacity==254) {
		// scannen ob noch was frei ist
		for(int i=pri; i<capacity; i++) {
			if(obj.some[i] == NULL) {
				// platz gefunden
				return i;
			}
		}
		// scannen ob noch was frei ist
		for(int i=0; i<pri; i++) {
			if(obj.some[i] == NULL) {
				// platz gefunden
				return i;
			}
		}
		return pri;	// well, this place is occupied, but may be best choice anyway ...
	}
	else {
		int new_cap = 0;
		if(pri >= capacity) {
			// wir brauchen neue eintraege
			new_cap = pri + 1;
		}
		else {

			// scannen ob noch was frei ist
			for(int i=pri; i<capacity; i++) {
				if(obj.some[i] == NULL) {
					// platz gefunden
					return i;
				}
			}

			// size exeeded, needs to extent
			new_cap = max(capacity + 1,255);
		}

		// wenn wir hier sind, dann muss erweitert werden
		const int first_free = max(capacity, pri);

		// neue kapazitaet auf vier aufrunden
		new_cap = (new_cap + 3) & 0xFFFC;
		set_capacity( new_cap );

		return first_free;
	}
}



void
dingliste_t::shrink_capacity(uint8 o_top)
{
    // if we go for speed we should not free this array
    // because roads and railroads often chnage to 0 objects
    // but need such an array again whn a train/truck arrives

    // if memory should be preserved this can be helpful

/*
    if(capacity > 0 && last_index == -1) {
	delete[] obj;
	obj = NULL;
	capacity = 0;
    } else
*/

    // strategy: avoid free'ing mem if not neccesary. Only if we hold lots
    // of memory then free it.
	if(capacity > 16 && o_top <= 4) {
		set_capacity(o_top);
	}
}



uint8
dingliste_t::add(ding_t *ding, uint8 pri)
{
	if(ding) {
		const uint8 index = grow_capacity(pri);

		// either save direct
		if(capacity==1) {
			obj.one = ding;
			top = index+1;
			return true;
		}

		// or in the array
		if(obj.some[index] == NULL) {

			obj.some[index] = ding;
			ding = NULL;

			if(index >= top) {
				top = index+1;
			}
		}
		return ding == NULL;
	}
	return false;
}



// take the thing out from the list
// use this only for temperary removing
// since it does not shrink list or checks for ownership
ding_t *
dingliste_t::remove_at(uint8 pos)
{
	ding_t *dt = NULL;;
	if(capacity==0) {
		//
	}
	else if(capacity==1) {
		if(top-1==pos) {
			dt = obj.one;
			obj.one = NULL;
			top = 0;
		}
	}
	else {
		if(pos<top) {
			dt = obj.some[pos];
			obj.some[pos] = 0;
			if(top-1==pos) {
				top --;
			}
		}
	}
	return dt;
}



uint8
dingliste_t::remove(ding_t *ding, spieler_t *sp)
{
	uint8 i, number=0;
	bool found = false;
	int last = -1;

	assert(ding != NULL);

	if(capacity<=1) {
		if(capacity==1  &&  obj.one==ding) {
			found = true;
		}
		obj.one = NULL;
		capacity = 0;
		top = 0;
		return found;
	}

	for(i=0; i<capacity; i++) {
		if(obj.some[i]!=NULL && obj.some[i]!=ding) {
			last = i;
			number ++;
		}

		if(obj.some[i]==ding) {
			obj.some[i] = NULL;
			found = true;

			if(i == top-1) {
				top = last+1;
			}

			break;
		}
	}

	return found;
}



bool
dingliste_t::loesche_alle(spieler_t *sp)
{

	if(capacity>1) {
		for(uint8 i=0; i<top; i++) {
			ding_t *dt = obj.some[i];
			if(dt) {
				dt->entferne(sp);
				delete dt;
				obj.some[i] = NULL;
			}
		}
	}
	else {
		if(capacity==1  &&  obj.one!=NULL) {
			ding_t *dt = obj.one;
			if(dt) {
				dt->entferne(sp);
				delete dt;
			}
		}
		obj.one = NULL;
	}

	top = 0;
	shrink_capacity(top);

    return true;
}



/* returns the text of an error message, if obj could not be removed */
const char *
dingliste_t::kann_alle_entfernen(const spieler_t *sp) const
{
	if(capacity==0) {
		return NULL;
	}
	else if(capacity==1) {
		if(obj.one!=NULL) {
			return obj.one->ist_entfernbar(sp);
		}
	}
	else {
		const char * msg = NULL;

		for(uint8 i=0; i<top; i++) {
			if(obj.some[i] != NULL) {
				msg = obj.some[i]->ist_entfernbar(sp);
				if(msg != NULL) {
					break;
				}
			}
		}
		return msg;
	}
}



/* check for obj */
bool
dingliste_t::ist_da(ding_t *ding) const
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
		return (top-1==start  &&  obj.one->gib_typ()==typ) ? obj.one : NULL;
	}
	else {
		// else we have to search the list
		for(uint8 i=start; i<top; i++) {
			ding_t * tmp = obj.some[i];
			if(tmp && tmp->gib_typ()==typ) {
				return tmp;
			}
		}
	}
	return NULL;
}



uint8
dingliste_t::count() const
{
	if(capacity<=1) {
		return (obj.one!=NULL);
	}
	else {
		uint8 i, n=0;
		for(i=0; i<top; i++) {
			if(obj.some[i]!=NULL) {
				n++;
			}
		}
		return n;
	}
}



void dingliste_t::rdwr(karte_t *welt, loadsave_t *file, koord3d current_pos)
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
			uint8 pri = (i>=255) ? 254: (uint8)i;	// try to read as many as possible
			ding_t::typ typ = (ding_t::typ)file->rd_obj_id();

			// DBG_DEBUG("dingliste_t::laden()", "Thing type %d", typ);

			if(typ == -1) {
				continue;
			}

			ding_t *d = NULL;

			switch(typ) {
				case ding_t::sync_wolke:	    d = new sync_wolke_t (welt, file);	        break;
				case ding_t::async_wolke:	    d = new async_wolke_t (welt, file);	        break;
				case ding_t::verkehr:	    d = new stadtauto_t (welt, file);		break;
				case ding_t::fussgaenger:	    d = new fussgaenger_t (welt, file);	        break;
				case ding_t::bahndepot:	    d = new bahndepot_t (welt, file);	        break;
				case ding_t::strassendepot:	    d = new strassendepot_t (welt, file);       break;
				case ding_t::schiffdepot:	    d = new schiffdepot_t (welt, file);	        break;
				case ding_t::bruecke:	    d = new bruecke_t (welt, file);	        break;
				case ding_t::tunnel:	    d = new tunnel_t (welt, file);	        break;
				case ding_t::pumpe:		    d = new pumpe_t (welt, file);	        break;
				case ding_t::leitung:	    d = new leitung_t (welt, file);	        break;
				case ding_t::senke:		    d = new senke_t (welt, file);	        break;
				case ding_t::oberleitung:	    d = new oberleitung_t (welt, file);	        break;
				case ding_t::zeiger:	    d = new zeiger_t (welt, file);	        break;
				case ding_t::roadsign:	    d = new roadsign_t (welt, file);	        break;

				// check for pillars
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
					pri = 0;
				}
				break;

				// will be ignored, was only used before 86.09
				case ding_t::gebaeudefundament: d = new dummy_ding_t(welt, file); delete d; d=NULL;  break;

				// only factories can smoke; but then, the smoker is reinstated after loading
				case ding_t::raucher: d = new raucher_t (welt, file); delete d; d = NULL; break;

#ifdef LAGER_NOT_IN_USE
				case ding_t::lagerhaus:	    d = new lagerhaus_t (welt, file);	        break;
				#else
				case ding_t::lagerhaus:
					dbg->warning("dingliste_t::laden()", "Error while loading game:");
					dbg->fatal("dingliste_t::laden()", "Sorry, this version of simutrans does\nnot support a %s building.\nplease remove it in your old simutrans version and try again",
					translator::translate("Lagerhaus"));
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
				DBG_DEBUG("dingliste_t::rdwr()","object ignored!");
				d = NULL;
			}

			//DBG_DEBUG("dingliste_t::rdwr()","Loading %d,%d #%d: %s", d->gib_pos().x, d->gib_pos().y, i, d->gib_name());
			if(d) {
				// add, if capacity not exceeded
				add(d, pri);
			}
		}
		else {
			// here is the saving part ...
			if( bei(i) &&  bei(i)->gib_pos()==current_pos  &&  bei(i)->gib_typ()!=ding_t::raucher) {
				bei(i)->rdwr(file);
			}
			else {
				if( bei(i) &&  bei(i)->gib_pos()!=current_pos) {
					DBG_DEBUG("dingliste_t::rdwr()","position error: %i,%i instead %i,%i",bei(i)->gib_pos().x,bei(i)->gib_pos().y,current_pos.x,current_pos.y);
					DBG_DEBUG("dingliste_t::rdwr()","object ignored!");
				}
				file->wr_obj_id(-1);
			}
		}
	}
}
