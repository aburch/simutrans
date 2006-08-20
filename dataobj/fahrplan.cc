/* completely overhauled by prissi Oct-2005 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../halthandle_t.h"
#include "../simimg.h"

#include "../gui/messagebox.h"
#include "../bauer/hausbauer.h"
#include "../besch/haus_besch.h"
#include "../boden/wege/weg.h"
#include "../boden/grund.h"
#include "../dings/gebaeude.h"
#include "../dings/zeiger.h"
#include "../simdepot.h"
#include "loadsave.h"

#include "fahrplan.h"
#include "linie.h"

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

fahrplan_t::fahrplan_t() : eintrag(0)
{
	init();
}

// copy constructor
// @author hsiegeln
fahrplan_t::fahrplan_t(fahrplan_t * old) : eintrag(0)
{
	if (old == NULL) {
		init();
	}
	else {
		aktuell = old->aktuell;
		type = old->type;

		for(unsigned i=0; i<old->eintrag.get_count(); i++) {
			eintrag.append(old->eintrag.at(i));
		}
		abgeschlossen = true;
		alle_fahrplaene.insert(this);
	}
}

fahrplan_t::fahrplan_t(loadsave_t *file) : eintrag(0)
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
		eintrag.append(src->eintrag.get(i));
	}
	if(aktuell>=(int)eintrag.get_count()) {
		aktuell = eintrag.get_count()-1;
	}
	if(aktuell<0) {
		aktuell = 0;
	}
	// do not touch abgeschlossen!
}


bool
fahrplan_t::insert(karte_t *welt, const grund_t *gr,int ladegrad)
{
	aktuell = max(aktuell,0);
	struct linieneintrag_t stop = { gr->gib_pos(), ladegrad, 0 };

	if(ist_halt_erlaubt(gr)) {
		bool ok= eintrag.insert_at(aktuell,stop);
		if(!ok) {
			dbg->warning("fahrplan_t::insert()","max stop exceeded!");
			create_win(-1, -1, 60, new nachrichtenfenster_t(welt, "Maximum 254 stops\nin a schedule!\n"), w_autodelete);
		}
		// ok
		return true;
	}
	else {
		// too many stops or wrong kind of stop
		zeige_fehlermeldung(welt);
		return false;
	}
}



bool
fahrplan_t::append(karte_t *welt, const grund_t *gr,int ladegrad)
{
	aktuell = max(aktuell,0);
	struct linieneintrag_t stop = { gr->gib_pos(), ladegrad, 0 };

	if(ist_halt_erlaubt(gr)) {
		bool ok= eintrag.append(stop,4);
		if(!ok) {
			dbg->warning("fahrplan_t::append()","max stop exceeded!");
			create_win(-1, -1, 60, new nachrichtenfenster_t(welt, "Maximum 254 stops\nin a schedule!\n"), w_autodelete);
		}
		// ok
		return true;
	}
	else {
		DBG_MESSAGE("fahrplan_t::append()","forbidden stop at %i,%i,%i",gr->gib_pos().x, gr->gib_pos().x, gr->gib_pos().z );
		// error
		zeige_fehlermeldung(welt);
		return false;
	}
}



// cleanup a schedule
bool
fahrplan_t::cleanup()
{
	if(eintrag.get_count()==0) {
		return true;	// nothing to check
	}

	// first and last must not be the same!
	koord3d lastpos=eintrag.get( eintrag.get_count()-1 ).pos;
	bool ok=true;
  	// now we have to check all entries ...
	for(unsigned i=0; i<eintrag.get_count(); i++) {
		if(eintrag.get(i).pos==lastpos) {
			// ingore double entries just one after the other
			ok = false;
			eintrag.remove_at(i);
			if(i>(unsigned)aktuell) {
				aktuell --;
			}
			i--;
		}
		else if(eintrag.get(i).pos==koord3d::invalid) {
			// ingore double entries just one after the other
			ok = false;
			eintrag.remove_at(i);
		}
		else {
			// next pos for check
			lastpos = eintrag.get(i).pos;
		}
	}
	if((unsigned)aktuell>eintrag.get_count()-1) {
		aktuell = eintrag.get_count()-1;
	}
	return ok;
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
	long dummy=aktuell;
	file->rdwr_long(dummy, " ");
	aktuell = (sint16)dummy;

	long maxi=eintrag.get_count();
	file->rdwr_long(maxi, " ");
	DBG_MESSAGE("fahrplan_t::rdwr()","read schedule %p with %i entries",this,maxi);

	if(file->is_loading()) {
		if(file->get_version()<86010) {
			// old array had different maxi-counter
			maxi ++;
		}

		for(int i=0; i<maxi; i++) {
			koord3d pos;
			pos.rdwr(file);
			file->rdwr_long(dummy, "\n");

			struct linieneintrag_t stop = { pos, (sint8)dummy, 0 };
			eintrag.append(stop);
		}
		abgeschlossen = true;
	}
	else {
		// saving
		for(unsigned i=0; i<eintrag.get_count(); i++) {
			eintrag.at(i).pos.rdwr(file);
			dummy = eintrag.at(i).ladegrad;
			file->rdwr_long(dummy, "\n");
		}
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
		if (fpl->eintrag.get(i).pos!=eintrag.get(i).pos) {	// ladegrad ignored?
			return false;
		}
	}
	return true;
}



void
fahrplan_t::add_return_way(karte_t *)
{
	for( int maxi = ((int)eintrag.get_count())-2;  maxi>0;  maxi--  ) {
		eintrag.append( eintrag.at(maxi) );
	}
}


fahrplan_t::fahrplan_type fahrplan_t::get_type(karte_t * welt) const
{
	if(eintrag.get_count()==0) {
		return type;
	}
	const grund_t *gr = welt->lookup(eintrag.get(0).pos);
	if ( type != fahrplan_t::fahrplan ) {
		//
	} else if (gr->gib_weg(weg_t::strasse) != NULL) {
		return fahrplan_t::autofahrplan;
	} else if (gr->gib_weg(weg_t::schiene) != NULL) {
		return fahrplan_t::zugfahrplan;
	} else if (gr->gib_weg(weg_t::wasser) != NULL) {
		return fahrplan_t::schifffahrplan;
	} else {
		return fahrplan_t::fahrplan;
	}
	return type;
}


bool
zugfahrplan_t::ist_halt_erlaubt(const grund_t *gr) const
{
DBG_MESSAGE("zugfahrplan_t::ist_halt_erlaubt()","Checking for stop");
	if(gr->gib_weg(weg_t::schiene)==NULL) {
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

void
zugfahrplan_t::zeige_fehlermeldung(karte_t *welt) const
{
    create_win(-1, -1, 60, new nachrichtenfenster_t(welt, "Zughalt muss auf\nSchiene liegen!\n"), w_autodelete);
}

bool
autofahrplan_t::ist_halt_erlaubt(const grund_t *gr) const
{
	if(gr->gib_weg(weg_t::strasse)==NULL) {
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

void
autofahrplan_t::zeige_fehlermeldung(karte_t *welt) const
{
    create_win(-1, -1, 60, new nachrichtenfenster_t(welt, "Autohalt muss auf\nStrasse liegen!\n"), w_autodelete);
}

bool
schifffahrplan_t::ist_halt_erlaubt(const grund_t *gr) const
{
    return gr!=NULL &&  (gr->ist_wasser()  ||  gr->gib_weg(weg_t::wasser));
}

void
schifffahrplan_t::zeige_fehlermeldung(karte_t *welt) const
{
    create_win(-1, -1, 60, new nachrichtenfenster_t(welt, "Schiffhalt muss im\nWasser liegen!\n"), w_autodelete);
}

bool
airfahrplan_t::ist_halt_erlaubt(const grund_t *gr) const
{
	return gr->gib_weg(weg_t::luft)!=NULL  ||  !gr->gib_halt().is_bound();
}

void
airfahrplan_t::zeige_fehlermeldung(karte_t *welt) const
{
    create_win(-1, -1, 60, new nachrichtenfenster_t(welt, "Flugzeughalt muss auf\nRunway liegen!\n"), w_autodelete);
}

bool
monorailfahrplan_t::ist_halt_erlaubt(const grund_t *gr) const
{
	return gr->gib_weg(weg_t::monorail)!=NULL;
}

void
monorailfahrplan_t::zeige_fehlermeldung(karte_t *welt) const
{
    create_win(-1, -1, 60, new nachrichtenfenster_t(welt, "Monorailhalt muss auf\nMonorail liegen!\n"), w_autodelete);
}
