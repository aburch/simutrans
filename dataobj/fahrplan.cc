/* completely overhauled by prissi Oct-2005 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simwin.h"
#include "../simtypes.h"
#include "../simworld.h"
#include "../simhalt.h"
#include "../simimg.h"

#include "../gui/messagebox.h"
#include "../bauer/hausbauer.h"
#include "../besch/haus_besch.h"
#include "../boden/grund.h"
#include "../dings/gebaeude.h"
#include "../dings/zeiger.h"
#include "../simdepot.h"
#include "loadsave.h"

#include "fahrplan.h"

#include "../tpl/slist_tpl.h"


/**
 * Manche Methoden müssen auf alle Fahrpläne angewandt werden
 * deshalb verwaltet die Klasse eine Liste aller Fahrpläne
 * @author Hj. Malthaner
 */
static slist_tpl<schedule_t *> alle_fahrplaene;

void schedule_t::init()
{
	aktuell = 0;
	abgeschlossen = false;
	alle_fahrplaene.insert(this);
	type = schedule_t::fahrplan;
}



schedule_t::schedule_t(schedule_t * old)
{
	assert(0);	//obsolete!
	if (old == NULL) {
		init();
	}
	else {
		aktuell = old->aktuell;
		type = old->type;
		my_waytype = old->my_waytype;

		for(unsigned i=0; i<old->eintrag.get_count(); i++) {
			eintrag.append(old->eintrag[i]);
		}
		abgeschlossen = true;
		alle_fahrplaene.insert(this);
	}
}

schedule_t::schedule_t(loadsave_t *file)
{
	type = schedule_t::fahrplan;
	rdwr(file);
	if(file->is_loading()) {
		cleanup();
	}
}


schedule_t::~schedule_t()
{
	const  bool ok = alle_fahrplaene.remove(this);
	if(ok) {
		DBG_MESSAGE("fahrplan_t::~fahrplan_t()", "Schedule %p destructed", this);
	}
	else {
		dbg->error("fahrplan_t::~fahrplan_t()", "Schedule %p was not registered!", this);
	}
}



// copy all entries from schedule src to this and adjusts aktuell
void schedule_t::copy_from(const schedule_t *src)
{
	// make sure, we can access both
	if(src==NULL) {
		dbg->error("fahrplan_t::copy_to()","cannot copy from NULL");
		return;
	}
	eintrag.clear();
	for(unsigned i=0; i<src->eintrag.get_count(); i++) {
		eintrag.append(src->eintrag[i]);
	}
	if(aktuell>=(int)eintrag.get_count()) {
		aktuell = eintrag.get_count()-1;
	}
	if(aktuell<0) {
		aktuell = 0;
	}
	// do not touch abgeschlossen!
}



bool schedule_t::ist_halt_erlaubt(const grund_t *gr) const
{
	// first: check, if we can go here
	bool ok = gr->hat_weg(my_waytype);
	if(  !ok  ) {
		if(  my_waytype==air_wt  ) {
			// everywhere is ok but not on stops (we have to load at airports only ...)
			ok = !gr->get_halt().is_bound();
		}
		else if(  my_waytype==water_wt  &&  gr->get_typ()==grund_t::wasser  ) {
			ok = true;
		}
		else if(  my_waytype==tram_wt  ) {
			// tram rails are track iternally
			ok = gr->hat_weg(track_wt);
		}
	}

	if(  ok  ) {
		// ok, we can go here; but we must also check, that we are not entring a foreign depot
		depot_t *dp = gr->get_depot();
		ok &=  (dp==NULL  ||  dp->get_tile()->get_besch()->get_extra()==my_waytype);
	}

	return ok;
}



bool schedule_t::insert(const grund_t* gr, uint8 ladegrad, uint8 waiting_time_shift )
{
	aktuell = max(aktuell,0);
#ifndef _MSC_VER
	struct linieneintrag_t stop = { gr->get_pos(), ladegrad, waiting_time_shift };
#else
	struct linieneintrag_t stop;
	stop.pos = gr->get_pos();
	stop.ladegrad = ladegrad;
	stop.waiting_time_shift = waiting_time_shift;
#endif
	// stored in minivec, so wie have to avoid adding too many
	if(eintrag.get_count()>=254) {
		create_win( new news_img("Maximum 254 stops\nin a schedule!\n"), w_time_delete, magic_none);
		return false;
	}

	if(ist_halt_erlaubt(gr)) {
		eintrag.insert_at(aktuell, stop);
		aktuell ++;
		return true;
	}
	else {
		// too many stops or wrong kind of stop
		create_win( new news_img(fehlermeldung()), w_time_delete, magic_none);
		return false;
	}
}



bool schedule_t::append(const grund_t* gr, uint8 ladegrad, uint8 waiting_time_shift)
{
	aktuell = max(aktuell,0);
#ifndef _MSC_VER
	struct linieneintrag_t stop = { gr->get_pos(), ladegrad, waiting_time_shift };
#else
	struct linieneintrag_t stop;
	stop.pos = gr->get_pos();
	stop.ladegrad = ladegrad;
	stop.waiting_time_shift = waiting_time_shift;
#endif

	// stored in minivec, so wie have to avoid adding too many
	if(eintrag.get_count()>=254) {
		create_win( new news_img("Maximum 254 stops\nin a schedule!\n"), w_time_delete, magic_none);
		return false;
	}

	if(ist_halt_erlaubt(gr)) {
		eintrag.append(stop, 4);
		return true;
	}
	else {
		DBG_MESSAGE("fahrplan_t::append()","forbidden stop at %i,%i,%i",gr->get_pos().x, gr->get_pos().x, gr->get_pos().z );
		// error
		create_win( new news_img(fehlermeldung()), w_time_delete, magic_none);
		return false;
	}
}



// cleanup a schedule
void schedule_t::cleanup()
{
	if (eintrag.empty()) return; // nothing to check

	// first and last must not be the same!
	koord3d lastpos = eintrag.back().pos;
  	// now we have to check all entries ...
	for(unsigned i=0; i<eintrag.get_count(); i++) {
		if (eintrag[i].pos == lastpos) {
			// ingore double entries just one after the other
			eintrag.remove_at(i);
			if(i<(unsigned)aktuell) {
				aktuell --;
			}
			i--;
		} else if (eintrag[i].pos == koord3d::invalid) {
			// ingore double entries just one after the other
			eintrag.remove_at(i);
		}
		else {
			// next pos for check
			lastpos = eintrag[i].pos;
		}
	}
	if((unsigned)aktuell+1>eintrag.get_count()) {
		aktuell = eintrag.get_count()-1;
	}
}



bool
schedule_t::remove()
{
	bool ok=eintrag.remove_at(aktuell);
	if( aktuell>=(int)eintrag.get_count()) {
		aktuell = eintrag.get_count()-1;
	}
	return ok;
}



void
schedule_t::rdwr(loadsave_t *file)
{
	xml_tag_t f( file, "fahrplan_t" );

	uint32 dummy=aktuell;
	file->rdwr_long(dummy, " ");
	aktuell = (sint16)dummy;

	sint32 maxi=eintrag.get_count();
	file->rdwr_long(maxi, " ");
	DBG_MESSAGE("fahrplan_t::rdwr()","read schedule %p with %i entries",this,maxi);
	eintrag.resize(max(0,maxi));

	if(file->get_version()<99012) {
		if(file->get_version()<86010) {
			// old array had different maxi-counter
			maxi ++;
		}
		for(int i=0; i<maxi; i++) {
			koord3d pos;
			pos.rdwr(file);
			file->rdwr_long(dummy, "\n");

			struct linieneintrag_t stop;
			stop.pos = pos;
			stop.ladegrad = (sint8)dummy;
			stop.waiting_time_shift = 0;
			eintrag.append(stop);
		}
	}
	else {
		// loading/saving new version
		for(sint32 i=0; i<maxi; i++) {
			if(eintrag.get_count()<=i) {
				eintrag.append( linieneintrag_t() );
				eintrag[i] .waiting_time_shift = 0;
			}
			eintrag[i].pos.rdwr(file);
			file->rdwr_byte(eintrag[i].ladegrad, "\n");
			if(file->get_version()>=99018) {
				file->rdwr_byte( eintrag[i].waiting_time_shift, "w" );
			}
		}
	}
	if(file->is_loading()) {
		abgeschlossen = true;
	}
	if(aktuell>=eintrag.get_count()) {
		dbg->error("fahrplan_t::rdwr()","aktuell %i >count %i => aktuell = 0", aktuell, eintrag.get_count() );
		aktuell = 0;
	}
}



void schedule_t::rotate90( sint16 y_size )
{
 	// now we have to rotate all entries ...
	for(unsigned i = 0; i<eintrag.get_count(); i++) {
		eintrag[i].pos.rotate90(y_size);
	}
}



/*
 * compare this fahrplan with another, passed in fahrplan
 * @author hsiegeln
 */
bool
schedule_t::matches(karte_t *welt, const schedule_t *fpl)
{
	if(fpl == NULL) {
		return false;
	}
	// same pointer => equal!
	if(this==fpl) {
		return true;
	}
	// unequal count => not equal
	const uint8 min_count = min( fpl->eintrag.get_count(), eintrag.get_count() );
	if(min_count==0  &&  fpl->eintrag.get_count()!=eintrag.get_count()) {
		return false;
	}
	// now we have to check all entries ...
	// we need to do this that complicated, because they last stop may make the difference
	unsigned f1=0, f2=0;
	while(  f1+f2<eintrag.get_count()+fpl->eintrag.get_count()  ) {
		if(f1<eintrag.get_count()  &&  f2<fpl->eintrag.get_count()  &&  fpl->eintrag[f2].pos == eintrag[f1].pos) {
			// ladegrad/waiting ignored: identical
			f1++;
			f2++;
		}
		else {
			bool ok = false;
			if(  f1<eintrag.get_count()  ) {
				grund_t *gr1 = welt->lookup(eintrag[f1].pos);
				if(  gr1->get_depot()  ) {
					// skip depot
					f1++;
					ok = true;
				}
			}
			if(  f2<fpl->eintrag.get_count()  ) {
				grund_t *gr2 = welt->lookup(fpl->eintrag[f2].pos);
				if(  gr2->get_depot()  ) {
					ok = true;
					f2++;
				}
			}
			// no depot but different => do not match!
			if(  !ok  ) {
				/* in principle we could also check for same halt; but this is dangerous,
				 * since a rebuilding of a single square might change that
				 */
				return false;
			}
		}
	}
	return f1==eintrag.get_count()  &&  f2==fpl->eintrag.get_count();
}



void schedule_t::add_return_way()
{
	if(eintrag.get_count()<127) {
		for( int maxi = ((int)eintrag.get_count())-2;  maxi>0;  maxi--  ) {
			eintrag.append(eintrag[maxi]);
		}
	}
}
