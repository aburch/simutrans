/* dingliste.cc
 *
 * author V. Meyer
 */

#include <stdio.h>

#include "../simdebug.h"
#include "dingliste.h"

#include "../dings/wolke.h"
#include "../dings/raucher.h"
#include "../dings/zeiger.h"
#include "../dings/baum.h"
#include "../dings/bruecke.h"
#include "../dings/tunnel.h"
#include "../dings/gebaeudefundament.h"
#include "../dings/gebaeude.h"
#include "../dings/signal.h"
#ifdef LAGER_NOT_IN_USE
#include "../dings/lagerhaus.h"
#endif
#include "../dings/gebaeude.h"
//#include "../dings/leitung.h"
#include "../dings/leitung2.h"
#include "../dings/oberleitung.h"

#include "../mm/mempool.h"

#include "../simdepot.h"
#include "../simmem.h"
#include "../simverkehr.h"
#include "../simpeople.h"
#include "../dataobj/translator.h"
#include "../besch/haus_besch.h"

#include "loadsave.h"


static mempool_t *pool = new mempool_t(8*sizeof(ding_t *));


static void dl_free(void *p, unsigned int size)
{
  assert((size & 7) == 0);

  // printf("free %d\n", size);

  if(size == 8) {
    pool->free(p);
  } else {
    guarded_free(p);
  }
}


static ding_t ** dl_alloc(unsigned int size)
{
  void *p = 0;

  assert((size & 7) == 0);

  // printf("alloc %d\n", size);

  if(size == 8) {
    p = pool->alloc();
  } else {
    p = guarded_malloc(size * sizeof(ding_t *));
  }

  return (ding_t **)p;
}


dingliste_t::dingliste_t()
{
    obj = 0;
    capacity = 0;
    top = 0;
}


dingliste_t::~dingliste_t()
{
    if(obj) {
	dl_free(obj, capacity);
    }

    // Paranoia
    obj = 0;
    capacity = 0;
    top = 0;
}


void
dingliste_t::set_capacity(const int new_cap)
{
    // dbg->message("dingliste_t::set_capacity()", "old cap=%d, new cap=%d", capacity, new_cap);

    // Speicherplatz besorgen
    ding_t **tmp = dl_alloc(new_cap);
    int i;

    const int end = MIN(capacity, new_cap);

    // objekte kopieren
    for(i=0; i<end; i++) {
	tmp[i] = obj[i];
    }

    // rest mit NULL init
    for(i=capacity; i<new_cap; i++) {
	tmp[i] = NULL;
    }

    if(obj) {
	dl_free(obj, capacity);
    }
    obj = tmp;
    capacity = new_cap;
}



/**
 * @return ersten freien index
 * @author Hj. Malthaner
 */
int
dingliste_t::grow_capacity(const int pri)
{
    int new_cap = 0;

    if(pri >= capacity) {
	// wir brauchen neue eintraege
	new_cap = pri + 1;
    } else {
	// scannen ob noch was frei ist

	for(int i=pri; i<capacity; i++) {
	    if(obj[i] == NULL) {
		// platz gefunden
		return i;
	    }
	}

	// min. ein neuer Eintrag;
	new_cap = capacity + 1;
    }

    // wenn wir hier sind, dann muss erweitert werden

    const int first_free = MAX(capacity, pri);

    // neue kapazitaet aufrunden
    new_cap = (new_cap + 7) & 0xFFFFFFF8;

    set_capacity( new_cap );

    return first_free;
}

void
dingliste_t::shrink_capacity(const int o_top)
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

    if(capacity > 32 && o_top < 8) {
	set_capacity(8);
    }
}


int
dingliste_t::add(ding_t *ding, int pri)
{
    if(ding) {
	const int index = grow_capacity(pri);

	if(obj[index] == NULL) {

	    obj[index] = ding;
	    ding = NULL;

	    if(index >= top-1) {
		top = index+1;
	    }
	}

	assert(top <= capacity);

	return ding == NULL;
    } else {
	return true;
    }
}


int
dingliste_t::remove(ding_t *ding, spieler_t *sp)
{
    int i;
    bool found = false;
    int last = -1;

    assert(ding != NULL);

    for(i=0; i<capacity; i++) {
	if(obj[i] != NULL && obj[i] != ding) {
	    last = i;
	}

	if(obj[i] == ding) {
	    obj[i] = NULL;
	    found = true;

	    ding->entferne(sp);

	    if(i == top-1) {
		top = last+1;
	    }

	    break;
	}
    }

    shrink_capacity(top);

    assert(top <= capacity);

    return found;
}

bool
dingliste_t::loesche_alle(spieler_t *sp)
{
    int i;

    for(i=0; i<top; i++) {
	ding_t *dt = bei(i);

	if(dt) {
	    dt->entferne(sp);
	    // der destruktor sollte das objekt autom. aus der karte
	    // entfernen. Tut er aber nicht immer bei waggons!
	    delete dt;

	    // CAVE:
	    // durch entfernen auf der Karte kann obj resized worden sein!
	    if(i < top)
		obj[i] = NULL;
	}
    }

    top = 0;
    shrink_capacity(top);

    assert(top <= capacity);

    return true;
}

const char *
dingliste_t::kann_alle_entfernen(const spieler_t *sp) const
{
    int i;
    const char * msg = NULL;

    for(i=0; i<top; i++) {
	if(obj[i] != NULL) {
	    msg = obj[i]->ist_entfernbar(sp);
	    if(msg != NULL) {
		return msg;
	    }
	}
    }

    return NULL;
}

bool
dingliste_t::ist_da(ding_t *ding) const
{
    int i;

    for(i=0; i<top; i++) {
	if(obj[i] == ding) {
	    break;
	}
    }
    return (obj != NULL) && (obj[i] == ding);
}

ding_t *
dingliste_t::suche(ding_t::typ typ) const
{
  ding_t * dt = 0;

  for(int i=0; i<top; i++) {
    ding_t * tmp = bei(i);
    if(tmp && tmp->gib_typ() == typ) {
      dt = tmp;
      break;
    }
  }

  return dt;
}


int
dingliste_t::count() const
{
    int i;
    int n = 0;

    for(i=0; i<top; i++) {
	if(obj[i] != NULL) {
	    n++;
	}
    }
    return n;
}

void dingliste_t::rdwr(karte_t *welt, loadsave_t *file)
{
    int max_object_index;

    if(file->is_saving()) {
        max_object_index = top-1;
    }
    file->rdwr_int(max_object_index, "\n");

    for(int i=0; i<=max_object_index; i++) {
        if(file->is_loading()) {
            ding_t::typ typ = (ding_t::typ)file->rd_obj_id();

	    // dbg->debug("dingliste_t::laden()", "Thing type %d", typ);

    	    if(typ == -1) {
    	        continue;
	    }

	    ding_t *d = 0;

	    switch(typ) {
	    case ding_t::baum:
		{
		    baum_t *b = new baum_t(welt, file);
		    if(!b->gib_besch()) {
			delete b;
			b = 0;
		    }
		    d = b;
		}
		break;
	    case ding_t::gebaeudefundament: d = new gebaeudefundament_t (welt, file);   break;
	    case ding_t::gebaeude:
		{
		    // Wenn es ein Gebäude nicht mehr gibt, geänderte Tabfiles
		    // lassen wir es einfach weg. Das gibt zwar Fundamente ohne
		    // Aufbau, stürzt aber nicht ab.
		    gebaeude_t *gb = new gebaeude_t (welt, file);
		    if(!gb->gib_tile()) {
			delete gb;
			gb  = NULL;
		    }
		    d = gb;
		}
		break;

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
#ifdef LAGER_NOT_IN_USE
	    case ding_t::lagerhaus:	    d = new lagerhaus_t (welt, file);	        break;
#else
 	    case ding_t::lagerhaus:
 		dbg->warning("dingliste_t::laden()", "Error while loading game:");
 		dbg->fatal("dingliste_t::laden()", "Sorry, this version of simutrans does\nnot support a %s building.\nplease remove it in your old simutrans version and try again",
 		    translator::translate("Lagerhaus"));
#endif
	    case ding_t::oberleitung:	    d = new oberleitung_t (welt, file);	        break;

	    case ding_t::raucher:	    d = new raucher_t (welt, file);	        break;
	    case ding_t::zeiger:	    d = new zeiger_t (welt, file);	        break;
	    default:
		dbg->warning("dingliste_t::laden()", "Error while loading game:");
		dbg->fatal("dingliste_t::laden()", "Unknown object type '%d'", typ);
	    }

//	    printf("Loading %d,%d #%d: %s\n", d->gib_pos().x, d->gib_pos().y, i, d->gib_name());
	    if(d) {
		add(d, i);
	    }
	}
        else {
	    if( obj[i] ) {
	        obj[i]->rdwr(file);
	    } else {
	        file->wr_obj_id(-1);
	    }
        }
    }
}
