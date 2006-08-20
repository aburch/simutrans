/**
 * simverkehr.cc
 *
 * Bewegliche Objekte fuer Simutrans.
 * Transportfahrzeuge sind in simvehikel.h definiert, da sie sich
 * stark von den hier definierten Fahrzeugen fuer den Individualverkehr
 * unterscheiden.
 *
 * Hj. Malthaner
 *
 * April 2000
 */

#include "simdebug.h"
#include "boden/grund.h"
#include "boden/wege/weg.h"
#include "simworld.h"
#include "simverkehr.h"
#include "simtools.h"
#include "simmem.h"
#include "tpl/slist_mit_gewichten_tpl.h"

#include "dataobj/loadsave.h"
#include "dataobj/umgebung.h"

#include "besch/stadtauto_besch.h"

#include "simimg.h"

slist_mit_gewichten_tpl<const stadtauto_besch_t *> stadtauto_t::liste;
stringhashtable_tpl<const stadtauto_besch_t *> stadtauto_t::table;

bool stadtauto_t::register_besch(const stadtauto_besch_t *besch)
{
    liste.append(besch);
    table.put(besch->gib_name(), besch);

    return true;
}

bool stadtauto_t::laden_erfolgreich()
{
    if(liste.count() == 0) {
	DBG_MESSAGE("stadtauto_t", "No citycars found - feature disabled");
    }
    return true;
}

int stadtauto_t::gib_anzahl_besch()
{
    return liste.count();
}


stadtauto_t::stadtauto_t(karte_t *welt, loadsave_t *file)
 : verkehrsteilnehmer_t(welt)
{
    rdwr(file);

    welt->sync_add(this);
}

stadtauto_t::stadtauto_t(karte_t *welt, koord3d pos)
 : verkehrsteilnehmer_t(welt, pos)
{
    besch = liste.gib_gewichted(simrand(liste.gib_gesamtgewicht()));
}


/**
 * Ensures that this object is removed correctly from the list
 * of sync steppable things!
 * @author Hj. Malthaner
 */
stadtauto_t::~stadtauto_t()
{
    // just to be sure we are removed from this list!
    welt->sync_remove(this);
}




void stadtauto_t::rdwr(loadsave_t *file)
{
    verkehrsteilnehmer_t::rdwr(file);

    const char *s = NULL;

    if(file->is_saving()) {
	s = besch->gib_name();
    }
    file->rdwr_str(s, "N");
    if(file->is_loading()) {
	besch = table.get(s);

	if(besch == 0 && liste.count() > 0) {
	  dbg->warning("stadtauto_t::rdwr()", "Object '%s' not found in table, trying first stadtauto object type",s);
	  besch = liste.at(0);
	}
	guarded_free(const_cast<char *>(s));

	if(besch == 0) {
	  dbg->fatal("stadtauto_t::rdwr()", "loading game with private cars, but no private car objects found in PAK files.");
	}
    }
}


void stadtauto_t::calc_bild()
{
    if(welt->lookup(gib_pos())->ist_tunnel()) {
	setze_bild(0, -1);
    } else {
	setze_bild(0,
		   besch->gib_bild_nr(ribi_t::gib_dir(gib_fahrtrichtung())));
    }
}


void verkehrsteilnehmer_t::calc_current_speed()
{
  const weg_t * weg = welt->lookup(gib_pos())->gib_weg(weg_t::strasse);

  const int speed_limit = weg ? kmh_to_speed(weg->gib_max_speed()) : max_speed;

  current_speed = speed_limit < max_speed ? speed_limit : max_speed;
}


verkehrsteilnehmer_t::verkehrsteilnehmer_t(karte_t *welt) :
   vehikel_basis_t(welt)
{
    setze_besitzer( welt->gib_spieler(1) );
    max_speed = 0;
    current_speed = 0;
}


verkehrsteilnehmer_t::verkehrsteilnehmer_t(karte_t *welt, koord3d pos) :
   vehikel_basis_t(welt, pos)
{
    // V.Meyer: weg_position_t changed to grund_t::get_neighbour()
    grund_t *from = welt->lookup(pos);
    grund_t *to;

    // int ribi = from->gib_weg_ribi(weg_t::strasse);
    ribi_t::ribi liste[4];
    int count = 0;

    max_speed = kmh_to_speed(80);
    current_speed = 50;

    weg_next = simrand(1024);
    hoff = 0;

    // verfügbare ribis in liste eintragen
    for(int r = 0; r < 4; r++) {
	if(from->get_neighbour(to, weg_t::strasse, koord::nsow[r])) {
	    liste[count++] = ribi_t::nsow[r];
	}
    }
    fahrtrichtung = count ? liste[simrand(count)] : ribi_t::nsow[simrand(4)];

    switch(fahrtrichtung) {
    case ribi_t::nord:
	dx = 2;
	dy = -1;
	break;
    case ribi_t::sued:
	dx = -2;
	dy = 1;
	break;
    case ribi_t::ost:
	dx = 2;
	dy = 1;
	break;
    case ribi_t::west:
	dx = -2;
	dy = -1;
	break;
    }
    if(count) {
	from->get_neighbour(to, weg_t::strasse, fahrtrichtung);
	naechstes_feld = to->gib_pos();
    } else {
	naechstes_feld = welt->lookup(pos.gib_2d() + koord(fahrtrichtung))->gib_kartenboden()->gib_pos();
    }
    setze_besitzer( welt->gib_spieler(1) );
}


/**
 * Öffnet ein neues Beobachtungsfenster für das Objekt.
 * @author Hj. Malthaner
 */
void verkehrsteilnehmer_t::zeige_info()
{
  if(umgebung_t::verkehrsteilnehmer_info) {
    ding_t::zeige_info();
  }
}


char *
verkehrsteilnehmer_t::info(char *buf) const
{
    *buf = 0;

    return buf;
}


void
verkehrsteilnehmer_t::hop()
{
    verlasse_feld();

    if(naechstes_feld.z == -1) {
	// Altes Savegame geladen
	naechstes_feld = welt->lookup(naechstes_feld.gib_2d())->gib_kartenboden()->gib_pos();
    }
    // V.Meyer: weg_position_t changed to grund_t::get_neighbour()
    grund_t *from = welt->lookup(naechstes_feld);
    grund_t *to;

    grund_t *liste[4];
    int count = 0;

    int ribi = from->gib_weg_ribi(weg_t::strasse);
    ribi_t::ribi gegenrichtung = ribi_t::rueckwaerts( gib_fahrtrichtung() );

    ribi = ribi & (~gegenrichtung);

    // verfügbare ribis in liste eintragen
    for(int r = 0; r < 4; r++) {
	if(!(ribi_t::nsow[r] & gegenrichtung) &&
	    from->get_neighbour(to, weg_t::strasse, koord::nsow[r])) {
	    liste[count++] = to;
	}
    }
    if(count > 0) {
	naechstes_feld = liste[simrand(count)]->gib_pos();
	fahrtrichtung = calc_richtung(gib_pos().gib_2d(), naechstes_feld.gib_2d(), dx, dy);
    } else {
	fahrtrichtung = gegenrichtung;
	dx = -dx;
	dy = -dy;

	naechstes_feld = gib_pos();
    }
    setze_pos(from->gib_pos());

    calc_current_speed();

    calc_bild();
    betrete_feld();
}


void
verkehrsteilnehmer_t::calc_bild()
{
    if(welt->lookup(gib_pos())->ist_tunnel()) {
	setze_bild(0, -1);
    } else {
	setze_bild(0, gib_bild(gib_fahrtrichtung()));
    }
}


bool verkehrsteilnehmer_t::sync_step(long delta_t)
{
  // DBG_MESSAGE("verkehrsteilnehmer_t::sync_step()", "%p called", this);

    weg_next += (current_speed*delta_t) / 64;

//    printf("verkehrsteilnehmer_t::sync_step weg=%d, weg_next=%d\n", weg, weg_next);

    // Funktionsaufruf vermeiden, wenn möglich
    // if ist schneller als Aufruf!
    if(hoff) {
	setze_yoff( gib_yoff() - hoff );
    }

    while(1024 < weg_next)
    {
	weg_next -= 1024;

	fahre();
    }

    hoff = calc_height();

    // Funktionsaufruf vermeiden, wenn möglich
    // if ist schneller als Aufruf!
    if(hoff) {
	setze_yoff( gib_yoff() + hoff );
    }

    return true;
}


void verkehrsteilnehmer_t::rdwr(loadsave_t *file)
{
  int dummy = 0;

  vehikel_basis_t::rdwr(file);

  file->rdwr_int(max_speed, "\n");
  file->rdwr_int(dummy, " ");
  file->rdwr_int(weg_next, "\n");
  file->rdwr_int(dx, " ");
  file->rdwr_int(dy, "\n");
  file->rdwr_enum(fahrtrichtung, " ");
  file->rdwr_int(hoff, "\n");

  naechstes_feld.rdwr(file);

  // Hajo: avoid endless growth of the values
  // this causes lockups near 2**32

  weg_next &= 1023;

  if(file->is_loading()) {
    calc_current_speed();

    // Hajo: pre-speed limit game had city cars with max speed of
    // 60 km/h, since city raods now have a speed limit of 50 km/h
    // we can use 80 for the cars so that they speed up on
    // intercity roads
    if(file->get_version() <= 84000 && max_speed == kmh_to_speed(60)) {
      max_speed = kmh_to_speed(80);
    }
  }
}
