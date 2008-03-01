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
static slist_tpl<fahrplan_t *> alle_fahrplaene;

void fahrplan_t::init()
{
	aktuell = 0;
	abgeschlossen = false;
	alle_fahrplaene.insert(this);
	type = fahrplan_t::fahrplan;
}

fahrplan_t::fahrplan_t()
{
	init();
}


fahrplan_t::fahrplan_t(fahrplan_t * old)
{
	if (old == NULL) {
		init();
	}
	else {
		aktuell = old->aktuell;
		type = old->type;

		for(unsigned i=0; i<old->eintrag.get_count(); i++) {
			eintrag.append(old->eintrag[i]);
		}
		abgeschlossen = true;
		alle_fahrplaene.insert(this);
	}
}

fahrplan_t::fahrplan_t(loadsave_t *file)
{
	type = fahrplan_t::fahrplan;
	rdwr(file);
	if(file->is_loading()) {
		cleanup();
	}
}


fahrplan_t::~fahrplan_t()
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
void fahrplan_t::copy_from(const fahrplan_t *src)
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


bool fahrplan_t::insert(const grund_t* gr, uint8 ladegrad, uint8 waiting_time_shift )
{
	aktuell = max(aktuell,0);
#ifndef _MSC_VER
	struct linieneintrag_t stop = { gr->gib_pos(), ladegrad, waiting_time_shift };
#else
	struct linieneintrag_t stop;
	stop.pos = gr->gib_pos();
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
		return true;
	}
	else {
		// too many stops or wrong kind of stop
		zeige_fehlermeldung();
		return false;
	}
}



bool fahrplan_t::append(const grund_t* gr, uint8 ladegrad, uint8 waiting_time_shift)
{
	aktuell = max(aktuell,0);
#ifndef _MSC_VER
	struct linieneintrag_t stop = { gr->gib_pos(), ladegrad, waiting_time_shift };
#else
	struct linieneintrag_t stop;
	stop.pos = gr->gib_pos();
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
		DBG_MESSAGE("fahrplan_t::append()","forbidden stop at %i,%i,%i",gr->gib_pos().x, gr->gib_pos().x, gr->gib_pos().z );
		// error
		zeige_fehlermeldung();
		return false;
	}
}



// cleanup a schedule
void fahrplan_t::cleanup()
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
fahrplan_t::remove()
{
	bool ok=eintrag.remove_at(aktuell);
	if( aktuell>=(int)eintrag.get_count()) {
		aktuell = eintrag.get_count()-1;
	}
	return ok;
}



void
fahrplan_t::rdwr(loadsave_t *file)
{
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



void fahrplan_t::rotate90( sint16 y_size )
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
fahrplan_t::matches(const fahrplan_t *fpl)
{
	if(fpl == NULL) {
		return false;
	}
	// same pointer => equal!
	if(this==fpl) {
		return true;
	}
	// unequal count => not equal
  	if(fpl->eintrag.get_count()!=eintrag.get_count()) {
  		return false;
  	}
  	// now we have to check all entries ...
	for(unsigned i = 0; i<eintrag.get_count(); i++) {
		if (fpl->eintrag[i].pos != eintrag[i].pos) { // ladegrad ignored!
			return false;
		}
	}
	return true;
}


void fahrplan_t::add_return_way()
{
	if(eintrag.get_count()<127) {
		for( int maxi = ((int)eintrag.get_count())-2;  maxi>0;  maxi--  ) {
			eintrag.append(eintrag[maxi]);
		}
	}
}


bool
zugfahrplan_t::ist_halt_erlaubt(const grund_t *gr) const
{
DBG_MESSAGE("zugfahrplan_t::ist_halt_erlaubt()","Checking for stop");
	if(!gr->hat_weg(track_wt)) {
		// no track
		return false;
	}
DBG_MESSAGE("zugfahrplan_t::ist_halt_erlaubt()","track ok");
	const depot_t *dp = gr->gib_depot();
	if(dp==NULL) {
		// empty track => ok
		return true;
	}
	// test for no street depot (may happen with trams)
	if(dp->gib_tile()->gib_besch()==hausbauer_t::str_depot_besch) {
		return false;
	}
	return true;
}


void zugfahrplan_t::zeige_fehlermeldung() const
{
	create_win( new news_img("Zughalt muss auf\nSchiene liegen!\n"), w_time_delete, magic_none);
}


bool
autofahrplan_t::ist_halt_erlaubt(const grund_t *gr) const
{
	if(!gr->hat_weg(road_wt)) {
		// no road
		return false;
	}
	const depot_t *gb = gr->gib_depot();
	if(gb==NULL) {
		// empty road => ok
		return true;
	}
	// test for no railway depot (may happen with trams)
	if(gb->gib_tile()->gib_besch()==hausbauer_t::bahn_depot_besch  ||  gb->gib_tile()->gib_besch()==hausbauer_t::tram_depot_besch) {
		return false;
	}
	return true;
}


void autofahrplan_t::zeige_fehlermeldung() const
{
	create_win( new news_img("Autohalt muss auf\nStrasse liegen!\n"), w_time_delete, magic_none);
}


bool
schifffahrplan_t::ist_halt_erlaubt(const grund_t *gr) const
{
    return gr!=NULL &&  (gr->ist_wasser()  ||  gr->hat_weg(water_wt));
}


void schifffahrplan_t::zeige_fehlermeldung() const
{
	create_win( new news_img("Schiffhalt muss im\nWasser liegen!\n"), w_time_delete, magic_none);
}


bool
airfahrplan_t::ist_halt_erlaubt(const grund_t *gr) const
{
	bool hat_halt = haltestelle_t::gib_halt(gr->get_welt(),gr->gib_pos().gib_2d()).is_bound();
	return hat_halt ? gr->hat_weg(air_wt) : true;
}


void airfahrplan_t::zeige_fehlermeldung() const
{
	create_win( new news_img("Flugzeughalt muss auf\nRunway liegen!\n"), w_time_delete, magic_none);
}


bool
monorailfahrplan_t::ist_halt_erlaubt(const grund_t *gr) const
{
	return gr->hat_weg(monorail_wt);
}


void monorailfahrplan_t::zeige_fehlermeldung() const
{
	create_win( new news_img("Monorailhalt muss auf\nMonorail liegen!\n"), w_time_delete, magic_none);
}
