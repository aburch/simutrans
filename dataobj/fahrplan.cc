#include <stdio.h>

#include "../simdebug.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../halthandle_t.h"
#include "../simimg.h"

#include "../gui/messagebox.h"
#include "../boden/wege/weg.h"
#include "../boden/grund.h"
#include "../dings/zeiger.h"
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

#define STOPS 32

void fahrplan_t::init()
{
    aktuell = 0;
    maxi = -1;
    eintrag.at(0).pos = koord3d(0,0,0);
    eintrag.at(0).ladegrad = 0;

    for(int i=0; i<STOPS; i++) {
	stops.at(i) = NULL;
    }

    abgeschlossen = false;

    alle_fahrplaene.insert(this);

    type = fahrplan_t::fahrplan;
}

fahrplan_t::fahrplan_t() : stops(STOPS), eintrag(STOPS)
{
	init();
}

// copy constructor
// @author hsiegeln
fahrplan_t::fahrplan_t(fahrplan_t * old) : stops(STOPS), eintrag(STOPS)
{
	if (old == NULL) {
		init();
	} else {
	    aktuell = old->aktuell;
	    maxi = old->maxi;

	    for(int i=0; i<STOPS; i++) {
	    	eintrag.at(i).pos = old->eintrag.at(i).pos;
		eintrag.at(i).ladegrad = old->eintrag.at(i).ladegrad;
		stops.at(i) = old->stops.at(i);
	    }

	    abgeschlossen = true;

	    alle_fahrplaene.insert(this);

	    type = old->type;
	}
}

fahrplan_t::fahrplan_t(loadsave_t *file) : stops(STOPS), eintrag(STOPS)
{
    for(int i=0; i<STOPS; i++) {
	stops.at(i) = NULL;
    }

    type = fahrplan_t::fahrplan;

    rdwr(file);
}


fahrplan_t::~fahrplan_t()
{
  const  bool ok = alle_fahrplaene.remove(this);

  if(ok) {
    dbg->message("fahrplan_t::~fahrplan_t()", "Schedule %p destructed", this);
  } else {
    dbg->error("fahrplan_t::~fahrplan_t()", "Schedule %p was not registered!", this);
  }
}


void
fahrplan_t::eingabe_abschliessen()
{
//    aktuell = 0;

    for(int i=0; i<STOPS; i++) {
	if(stops.at(i) != NULL) {
	    delete stops.at(i);
	    stops.at(i) = NULL;
	}
    }

    abgeschlossen = true;
}

void
fahrplan_t::eingabe_beginnen()
{
    abgeschlossen = false;
}


bool
fahrplan_t::insert(karte_t *welt, koord3d k)
{
    if(maxi < STOPS-1) {

	const grund_t *gr = welt->lookup(k);

	if(ist_halt_erlaubt(gr)) {

	    if(aktuell < 0 || maxi < 0) {
		// neuer Fahrplan
		aktuell = 0;
		maxi = 0;
		eintrag.at(0).pos = k;
		eintrag.at(0).ladegrad = 0;
	    } else if(maxi == 0) {
		aktuell = 0;

		if(k != eintrag.at(0).pos) {
		    eintrag.at(1) = eintrag.at(0);
		    eintrag.at(0).pos = k;
		    eintrag.at(0).ladegrad = 0;
		}
		maxi ++;
	    } else {
		// maxi ist hier größer als 0

		// wir müssen prüfen, ob der eintrag ungleich
		// dem aktuellen und ungleich dem vorgänger ist

		const int akt_pruef = aktuell;
		const int vor_pruef = aktuell > 0 ? aktuell-1 : maxi;


		if(k != eintrag.at(akt_pruef).pos &&
		   k != eintrag.at(vor_pruef).pos) {

		    // Einträge verschieben
		    for(int i=maxi; i>=aktuell; i--) {
			eintrag.at(i+1) = eintrag.at(i);
		    }
		    // neuen Eintrag einfügen
		    eintrag.at(aktuell).pos = k;
		    eintrag.at(aktuell).ladegrad = 0;
		    maxi ++;
		}
	    }

	    return true;
	} else {
	    zeige_fehlermeldung(welt);
	    return false;
	}

    } else {
	// kein Eintrag mehr frei
	return false;
    }
}

bool
fahrplan_t::append(karte_t *welt, koord3d k)
{
    if(maxi < STOPS-1) {

	const grund_t *gr = welt->lookup(k);

	if(ist_halt_erlaubt(gr)) {

	    if(aktuell < 0 || maxi < 0) {
			// neuer fahrplan
			aktuell = 0;
			maxi = 0;
	        eintrag.at(0).pos = k;
			eintrag.at(0).ladegrad = 0;
	    } else {
		if(k != eintrag.at(0).pos &&
		   k != eintrag.at(maxi).pos) {

		    maxi ++;
		    eintrag.at(maxi).pos = k;
		    eintrag.at(maxi).ladegrad = 0;
		}
	    }

	    return true;
	} else {
	    zeige_fehlermeldung(welt);
	    return false;
	}

    } else {
	// kein Eintrag mehr frei
	return false;
    }
}



bool
fahrplan_t::remove()
{
    if(maxi > -1 && aktuell >= 0 && aktuell <= maxi) {

	for(int i=aktuell; i<maxi; i++) {
	    eintrag.at(i) = eintrag.at(i+1);
	}
	eintrag.at(maxi).pos = koord3d::invalid;
	eintrag.at(maxi).ladegrad = 0;

	maxi --;

	if(aktuell > maxi) {
	    aktuell = maxi;
	}

	return true;
    } else {
	return false;
    }
}

void
fahrplan_t::rdwr(loadsave_t *file)
{
    file->rdwr_int(aktuell, " ");
    file->rdwr_int(maxi, " ");

    if(file->is_loading()) {
	abgeschlossen = true;
    }
    for(int i=0; i<=maxi; i++) {
	eintrag.at(i).pos.rdwr( file );

	file->rdwr_int(eintrag.at(i).ladegrad, "\n");
    }

    if(file->is_loading()) {
      // Hajo: enable this once the line management is ready to be used
      // linie_t::vereinige(&eintrag);
    }
}


/*
 * compare this fahrplan with another, passed in fahrplan
 * @author hsiegeln
 */
bool
fahrplan_t::matches(fahrplan_t *fpl) {
  int MAX_STOPS = STOPS; // max stops

  if (fpl == NULL) {
    return false;
  }

  for (int i = 0; i<MAX_STOPS; i++) {
    if (fpl->eintrag.at(i).pos != eintrag.at(i).pos) {
      return false;
    }
  }
  return true;
}

/**
 * set the current stop of the fahrplan
 * if new value is bigger than stops available, the max stop will be used
 * @author hsiegeln
 */
void
fahrplan_t::set_aktuell(int new_aktuell)
{
	// if the passed value is greater than available stops, set current stop to highes stop available
	// otherwise set value as expected
	if (maxi < new_aktuell)
	{
		aktuell = maxi;
	} else {
		aktuell = new_aktuell;
	}
}

void
fahrplan_t::add_return_way(karte_t * welt)
{
	for (int i=maxi; i>0; i--) {
		append(welt, eintrag.at(i).pos);
	    eintrag.at(maxi).ladegrad = eintrag.at(i).ladegrad;
	}
}


fahrplan_t::fahrplan_type fahrplan_t::get_type(karte_t * welt) const
{
  const grund_t *gr = welt->lookup(eintrag.get(0).pos);
  if ( type != fahrplan_t::fahrplan ) {
    return type;
  } else if (gr->gib_weg(weg_t::strasse) != NULL) {
    return fahrplan_t::autofahrplan;
  } else if (gr->gib_weg(weg_t::schiene) != NULL) {
    return fahrplan_t::zugfahrplan;
  } else if (gr->gib_weg(weg_t::wasser) != NULL) {
    return fahrplan_t::schifffahrplan;
  } else {
    return fahrplan_t::fahrplan;
  }
}


bool
zugfahrplan_t::ist_halt_erlaubt(const grund_t *gr) const
{
    return gr->gib_weg(weg_t::schiene) != NULL;
}

void
zugfahrplan_t::zeige_fehlermeldung(karte_t *welt) const
{
    create_win(-1, -1, 60, new nachrichtenfenster_t(welt, "Zughalt muss auf\nSchiene liegen!\n"), w_autodelete);
}

bool
autofahrplan_t::ist_halt_erlaubt(const grund_t *gr) const
{
    return gr->gib_weg(weg_t::strasse) != NULL;
}

void
autofahrplan_t::zeige_fehlermeldung(karte_t *welt) const
{
    create_win(-1, -1, 60, new nachrichtenfenster_t(welt, "Autohalt muss auf\nStraße liegen!\n"), w_autodelete);
}

bool
schifffahrplan_t::ist_halt_erlaubt(const grund_t *gr) const
{
    return gr != NULL &&
	   gr->ist_wasser() /* &&
	   gr->gib_weg_ribi_unmasked(weg_t::wasser)
			    */
	   ;
}

void
schifffahrplan_t::zeige_fehlermeldung(karte_t *welt) const
{
    create_win(-1, -1, 60, new nachrichtenfenster_t(welt, "Schiffhalt muss im\nWasser liegen!\n"), w_autodelete);
}
