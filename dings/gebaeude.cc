/*
 * hausanim.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */
#include <string.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simfab.h"
#include "../simimg.h"
#include "../simhalt.h"
#include "../simwin.h"
#include "../simcity.h"
#include "../simcosts.h"
#include "../simplay.h"
#include "../simmem.h"
#include "../simtools.h"
#include "../simdebug.h"
#include "../simintr.h"
#include "../simskin.h"

#include "../boden/grund.h"


#include "../besch/haus_besch.h"
#include "../bauer/warenbauer.h"

#include "../utils/cbuffer_t.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"

#include "../gui/stadt_info.h"
#include "../gui/fabrik_info.h"

#include "gebaeude.h"


const int gebaeude_t::ALT = 10000;

bool gebaeude_t::hide = false;



/**
 * Initializes all variables with save, usable values
 * @author Hj. Malthaner
 */
void gebaeude_t::init(const haus_tile_besch_t *t)
{
    tile = t;
    fab = 0;

    anim_time = 0;
    tourist_time = 0;

    sync = false;
    count = 0;
    zeige_baugrube = t ? !t->gib_besch()->ist_ohne_grube() : false;
}


gebaeude_t::gebaeude_t(karte_t *welt) : ding_t(welt)
{
    init(0);
}


gebaeude_t::gebaeude_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
    init(0);

    rdwr(file);
}

gebaeude_t::gebaeude_t(karte_t *welt, koord3d pos,spieler_t *sp, const haus_tile_besch_t *t) :
    ding_t(welt, pos)
{
    setze_besitzer( sp );

    tile = t;
    calc_yoff();
    renoviere();

    init(t);
    if(gib_besitzer()) {
	gib_besitzer()->add_maintenance(umgebung_t::maint_building);
    }
}

/**
 * Destructor. Removes this from the list of sync objects if neccesary.
 *
 * @author Hj. Malthaner
 */
gebaeude_t::~gebaeude_t()
{
    if(sync) {
	sync = false;
	welt->sync_remove(this);
    }

    // Hajo: if the PAK file was removed we end up with buildings
    // that have no tile description. Thus we need to check that
    // case here ...
    if(tile && tile->gib_besch() &&
       tile->gib_besch()->ist_ausflugsziel()) {
      welt->remove_ausflugsziel(this);
    }

    count = 0;
    anim_time = 0;
    if(gib_besitzer()) {
	gib_besitzer()->add_maintenance(-umgebung_t::maint_building);
    }
}

void
gebaeude_t::calc_yoff()
{
    const grund_t *gr = welt->lookup(gib_pos());

    if(gr && !gr->ist_wasser()) {

	const int ht = welt->get_slope(gib_pos().gib_2d());

	if(ht != 0 ) {				// wenn uneben
	    setze_yoff( -16 );			// dann sockel
        } else {
	    setze_yoff( 0 );
	}
    } else {
	setze_yoff( 0 );
    }
}

void
gebaeude_t::add_alter(int a)
{
    insta_zeit -= a;

    if(welt->gib_zeit_ms() - insta_zeit < 5000) {
	if(zeige_baugrube) {
	    zeige_baugrube = false;
	    set_flag(dirty);
	}
    }
}

void
gebaeude_t::renoviere()
{
    insta_zeit = welt->gib_zeit_ms();
    zeige_baugrube = true;
}


ding_info_t *gebaeude_t::new_info()
{
  if (fab) {
      return new fabrik_info_t(fab, this, welt);

  } else {
    return ding_t::new_info();
  }
}


void
gebaeude_t::zeige_info()
{
    //
    // Für die Anzeige ist bei mehrteiliggen Gebäuden immer
    // das erste laut Layoutreihenfolge zuständig.
    // Sonst gibt es für eine 2x2-Fabrik bis zu 4 Infofenster.
    koord k = tile->gib_offset();
    if(k != koord(0, 0)) {
	ding_t *gb = welt->lookup(gib_pos() - k)->obj_bei(1);

	if(gb)
	    gb->zeige_info();	// an den ersten deligieren
    }
    else {
	DBG_MESSAGE("gebaeude_t::zeige_info()", "at %d,%d - name is '%s'", gib_pos().x, gib_pos().y, gib_name());

	if(!tile->gib_besch()->ist_ohne_info()) {

	  if(ist_rathaus()) {

	    create_win(-1, -1, -1,
		       new stadt_info_t(welt->suche_naechste_stadt(gib_pos().gib_2d())),
		       w_autodelete,
		       magic_city_info_t);
	  }
//	  else {
	    ding_t::zeige_info();
//	  }
	}
    }
}

/**
 * Should only be called after everything is set up to play
 * the animation actually. Sets sync flag and register/deregisters
 * this as a sync object
 *
 * @author Hj. Malthaner
 */
void gebaeude_t::setze_sync(bool yesno)
{
    if(yesno) {
	// already sync? and worth animating ?
	if(!sync && tile->gib_phasen() > 1) {
	    // no
	    sync = true;
	    anim_time = simrand(300);
	    count = simrand(tile->gib_phasen());
	    welt->sync_add(this);
	}
    } else {
	sync = false;

	// always deregister ... doesn't hurt if we were not registered.
	welt->sync_remove(this);
    }
}


bool
gebaeude_t::step(long delta_t)
{
	// still under construction?
	if(zeige_baugrube) {
		if(welt->gib_zeit_ms() - insta_zeit > 5000) {
			zeige_baugrube = false;
			set_flag(dirty);
		}
	}

	// generate passengers for attractions
	if(tile->gib_besch()->ist_ausflugsziel()) {

		tourist_time += delta_t;

//		const int waiting_time = 5000
		if(tourist_time > 5000) {
			tourist_time -= 5000;

			INT_CHECK("gebaeude 228");

			// erzeuge ein paar passagiere
			const vector_tpl<halthandle_t> & halt_list = welt->suche_nahe_haltestellen(gib_pos().gib_2d(), 4, 0, 8);

			if(halt_list.get_count() > 0) {
				const array_tpl<stadt_t *>* staedte = welt->gib_staedte();
				const stadt_t *stadt = staedte->get(simrand(welt->gib_einstellungen()->gib_anzahl_staedte()));

				// create up to level passengers
				for(int j=0; j<tile->gib_besch()->gib_level();  j+=17  ) {
					const koord ziel = stadt->gib_zufallspunkt();

					ware_t pax (warenbauer_t::passagiere);
					pax.menge = (delta_t&3)+1;
					pax.setze_zielpos( ziel );

					for(uint32 i=0; i<halt_list.get_count(); i++) {
						halthandle_t halt = halt_list.get(i);

						halt->suche_route(pax, halt);

						// printf("7 Tourists generated\n");

						if(halt->gib_ware_summe(warenbauer_t::passagiere) > (halt->gib_grund_count() << 7)) {
							// Hajo: Station crowded:
							// some are appalled and will not try other
							// stations
							halt->add_pax_unhappy(pax.menge);

						} else if(pax.gib_ziel() != koord::invalid) {

							halt->liefere_an(pax);
							halt->add_pax_happy(pax.menge);
						}
						else {

							halt->add_pax_no_route(pax.menge);
						}
					}
					INT_CHECK("gebaeude 254");
				}
			}
		}
	} // Ausflugsziel

	// factory produce and passengers!
	if(fab != NULL) {
		fab->step(delta_t);
		INT_CHECK("gebaeude 250");
	}

	// create pedestrians?
	if(umgebung_t::fussgaenger) {
		if(simrand(100) <= tile->gib_besch()->gib_level()) {
			haltestelle_t::erzeuge_fussgaenger(welt, gib_pos(), 1);
			INT_CHECK("gebaeude 269");
		}
	}


  if(!sync && tile->gib_phasen() > 1) {
    count ++;

    if(count >= tile->gib_phasen()) {
      count = 0;
	}
    set_flag(dirty);
  }


  return true;
}

int
gebaeude_t::gib_bild() const
{
  if(hide &&
     // !haltestelle_t::gib_halt(welt, gib_pos().gib_2d()).is_bound() &&
     gib_haustyp() != unbekannt) {
    return skinverwaltung_t::baugrube->gib_bild_nr(0);
  }

  if(zeige_baugrube)  {
    return skinverwaltung_t::baugrube->gib_bild_nr(0);
  } else {
    return tile->gib_hintergrund(count, 0);
  }
}



int
gebaeude_t::gib_bild(int nr) const
{
    if(zeige_baugrube || hide) {
	return -1;
    } else {
	return tile->gib_hintergrund(count, nr);
    }
}

int
gebaeude_t::gib_after_bild() const
{
    if(zeige_baugrube) {
	return -1;
    } else {
	return tile->gib_vordergrund(count, 0);
    }
}

int
gebaeude_t::gib_after_bild(int nr) const
{
    if(zeige_baugrube) {
	return -1;
    } else {
	return tile->gib_vordergrund(count, nr);
    }
}

void gebaeude_t::setze_count(int count)
{
    this->count = count % tile->gib_phasen();
}

// calculate also the size
int gebaeude_t::gib_passagier_level() const
{
	koord dim = tile->gib_besch()->gib_groesse();
	return tile->gib_besch()->gib_level()*dim.x*dim.y;
}

int gebaeude_t::gib_post_level() const
{
	koord dim = tile->gib_besch()->gib_groesse();
	return tile->gib_besch()->gib_post_level()*dim.x*dim.y;
}


/**
 * @return eigener Name oder Name der Fabrik falls Teil einer Fabrik
 * @author Hj. Malthaner
 */
const char *gebaeude_t::gib_name() const
{
    if(fab) {
	return fab->gib_name();
    }
    switch(tile->gib_besch()->gib_typ()) {
    case wohnung:
	break;//return "Wohnhaus";
    case gewerbe:
	break;//return "Gewerbehaus";
    case industrie:
	break;//return "Industriegebäude";
    default:
	switch(tile->gib_besch()->gib_utyp()) {
	case hausbauer_t::special:
	    return "Besonderes Gebäude";
	case hausbauer_t::sehenswuerdigkeit:
	    return "Sehenswürdigkeit";
	case hausbauer_t::denkmal:
	    return "Denkmal";
	case hausbauer_t::rathaus:
	    return "Rathaus";
	default:
	    break;
	}
    }
    return "Gebaeude";
}

bool gebaeude_t::ist_rathaus() const
{
    return tile->gib_besch()->ist_rathaus();
}

gebaeude_t::typ gebaeude_t::gib_haustyp() const
{
    return tile->gib_besch()->gib_typ();
}


void gebaeude_t::info(cbuffer_t & buf) const
{
  ding_t::info(buf);

  if(fab != NULL) {
    fab->info(buf);
  }
  else if(zeige_baugrube) {
    buf.append(translator::translate("Baustelle"));
    buf.append("\n");
  } else {
    const char *desc = tile->gib_besch()->gib_name();
    if(desc != NULL) {
      buf.append(translator::translate(desc));
      buf.append("\n");
    }

    buf.append(translator::translate("Passagierrate"));
    buf.append(": ");
    buf.append(gib_passagier_level());
    buf.append("\n");

    buf.append(translator::translate("Postrate"));
    buf.append(": ");
    buf.append(gib_post_level());
    buf.append("\n");

    if(gib_besitzer() == NULL) {
      buf.append(translator::translate("Wert"));
      buf.append(": ");
      buf.append(-CST_HAUS_ENTFERNEN*(tile->gib_besch()->gib_level()+1)/100);
      buf.append("$\n");
    }
  }
}


void
gebaeude_t::rdwr(loadsave_t *file)
{
    if(fabrik() == NULL || file->is_loading()) {
	// Gebaude, die zu einer Fabrik gehoeren werden beim laden
	// der Fabrik neu erzeugt
	ding_t::rdwr(file);

	char  buf[256];
	short idx;


	if(file->is_saving()) {
	    const char *s = tile->gib_besch()->gib_name();
	    idx = tile->gib_index();
	    file->rdwr_str(s, "N");
	} else {
	    file->rd_str_into(buf, "N");
	}

	file->rdwr_short(idx, "\n");
 	file->rdwr_long(insta_zeit, " ");

	if(file->is_loading()) {
	    tile = hausbauer_t::find_tile(buf, idx);
	    if(!tile) {
		DBG_MESSAGE("gebaeude_t::rwdr", "description %s for building at %d,%d not found (will be removed)!", buf, gib_pos().x, gib_pos().y);
	    }


	    // Denkmäler sollen nicht doppelt gebaut werden, daher
	    // checken wir hier über den Gebäudenamen, ob wir ein Denkmal sind.
	    // Sollte später mal hübscher gehen: entweder speichern
	    // der gebauten denkmäler
	    // oder aber eine feste Zuordnung von gebaeude_alt_t
	    // und Beschreibung.
	    if(tile && tile->gib_besch()->gib_utyp() == hausbauer_t::denkmal) {
		hausbauer_t::denkmal_gebaut(tile->gib_besch());
	    }
	}
	file->rdwr_char(sync, "\n");

	if(file->is_loading() && tile) {
    	    count = 0;
	    anim_time = 0;
	    if(sync) {
		// Sicherstellen, dass alles wieder animiert wird!
		// Ohne "sync=false" denkt setze_sync(), es dreht sich
	        // schon alles.
		sync = false;
		setze_sync(true);
	    }

	    // Hajo: rebuild tourist attraction list
	    if(tile->gib_besch()->ist_ausflugsziel()) {
	      welt->add_ausflugsziel( this );
	    }
	}
    } else {
        file->wr_obj_id(-1);
    }


    if(file->is_loading()) {
        if(gib_besitzer()) {
	    gib_besitzer()->add_maintenance(umgebung_t::maint_building);
        }
    }
}

/**
 * Methode für Echtzeitfunktionen eines Objekts.
 * @return false wenn Objekt aus der Liste der synchronen
 * Objekte entfernt werden sol
 * @author Hj. Malthaner
 */
bool gebaeude_t::sync_step(long delta_t)
{
    // DBG_MESSAGE("gebaeude_t::sync_step()", "%p, %d phases", this, phasen);

    anim_time += delta_t;

    if(anim_time > 300) {
	anim_time = 0;
	count ++;

	if(count >= tile->gib_phasen()) {
	    count = 0;
	}

	set_flag(dirty);
    }

    return true;
}


void
gebaeude_t::entferne(spieler_t *sp)
{
    if(sp != NULL && !ist_rathaus()) {
	sp->buche(CST_HAUS_ENTFERNEN*(tile->gib_besch()->gib_level()+1),
		  gib_pos().gib_2d(),
		  COST_CONSTRUCTION);
    }
}
