/*
 * hausbauer.cc
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <string.h>

#include "../simdebug.h"
#include "hausbauer.h"
#include "../simworld.h"
#include "../simwerkz.h"
#include "../simdepot.h"
#include "../simhalt.h"
#include "../simtools.h"
#include "../dings/gebaeude.h"
#include "../boden/grund.h"
#include "../boden/fundament.h"

#include "../besch/haus_besch.h"
#include "../besch/spezial_obj_tpl.h"

// Hajo: these are needed to build the menu entries
#include "../gui/werkzeug_parameter_waehler.h"
#include "../besch/skin_besch.h"
#include "../dataobj/translator.h"

#include "../tpl/weighted_vector_tpl.h"

#ifdef _MSC_VER
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif


/*
 * Die verschiedenen Gebäudegruppen sind in eigenen Listen gesammelt.
 */
slist_tpl<const haus_besch_t *> hausbauer_t::alle;
slist_tpl<const haus_besch_t *> hausbauer_t::wohnhaeuser;
slist_tpl<const haus_besch_t *> hausbauer_t::gewerbehaeuser;
slist_tpl<const haus_besch_t *> hausbauer_t::industriehaeuser;
slist_tpl<const haus_besch_t *> hausbauer_t::fabriken;
slist_tpl<const haus_besch_t *> hausbauer_t::sehenswuerdigkeiten;
slist_tpl<const haus_besch_t *> hausbauer_t::spezials;
slist_tpl<const haus_besch_t *> hausbauer_t::denkmaeler;
slist_tpl<const haus_besch_t *> hausbauer_t::ungebaute_denkmaeler;

/*
 * Diese Tabelle ermöglicht das Auffinden einer Beschreibung durch ihren Namen
 */
stringhashtable_tpl<const haus_besch_t *> hausbauer_t::besch_names;

/*
 * Alle Gebäude, die die Anwendung direkt benötigt, kriegen feste IDs.
 * Außerdem müssen wir dafür sorgen, dass sie alle da sind.
 */
 // the is needed, since it has only one exist and no road below ...
const haus_besch_t *hausbauer_t::frachthof_besch = NULL;

const haus_besch_t *hausbauer_t::bahn_depot_besch = NULL;
const haus_besch_t *hausbauer_t::tram_depot_besch = NULL;
const haus_besch_t *hausbauer_t::str_depot_besch = NULL;
const haus_besch_t *hausbauer_t::sch_depot_besch = NULL;
const haus_besch_t *hausbauer_t::monorail_depot_besch = NULL;
const haus_besch_t *hausbauer_t::monorail_foundation_besch = NULL;
const haus_besch_t *hausbauer_t::muehle_besch = NULL;

slist_tpl<const haus_besch_t *> hausbauer_t::train_stops;
slist_tpl<const haus_besch_t *> hausbauer_t::car_stops;
slist_tpl<const haus_besch_t *> hausbauer_t::ship_stops;
slist_tpl<const haus_besch_t *> hausbauer_t::ship_channel_stops;
slist_tpl<const haus_besch_t *> hausbauer_t::post_offices;
slist_tpl<const haus_besch_t *> hausbauer_t::station_building;
slist_tpl<const haus_besch_t *> hausbauer_t::headquarter;


static spezial_obj_tpl<haus_besch_t> spezial_objekte[] = {
    { &hausbauer_t::frachthof_besch,   "CarStop" },
    { &hausbauer_t::bahn_depot_besch,   "TrainDepot" },
    { &hausbauer_t::tram_depot_besch,   "TramDepot" },
    { &hausbauer_t::monorail_depot_besch,   "MonorailDepot" },
    { &hausbauer_t::monorail_foundation_besch,   "MonorailGround" },
    { &hausbauer_t::str_depot_besch,	"CarDepot" },
    { &hausbauer_t::sch_depot_besch,	"ShipDepot" },
    { NULL, NULL }
};


bool hausbauer_t::alles_geladen()
{
    warne_ungeladene(spezial_objekte, 10);
    return true;
}


void hausbauer_t::insert_sorted(slist_tpl<const haus_besch_t *> &liste, const haus_besch_t *besch)
{
	slist_iterator_tpl<const haus_besch_t *>  iter(liste);
	int pos = 0;

	while(iter.next()) {
		int diff;

		diff = iter.get_current()->gib_level() - besch->gib_level();
		if(diff == 0) {
			// Gleiches Level - wir führen eine künstliche, aber eindeutige
			// Sortierung über den Namen herbei.
			diff = strcmp(iter.get_current()->gib_name(), besch->gib_name());
		}
		if(diff > 0) {
			liste.insert(besch, pos);
			return;
		}
		pos++;
	}
	liste.append(besch);
}


bool hausbauer_t::register_besch(const haus_besch_t *besch)
{
    alle.append(besch);

    ::register_besch(spezial_objekte, besch);

    switch(besch->gib_typ()) {
    case gebaeude_t::wohnung:
	insert_sorted(wohnhaeuser, besch);
	break;
    case gebaeude_t::industrie:
	insert_sorted(industriehaeuser, besch);
	break;
    case gebaeude_t::gewerbe:
	insert_sorted(gewerbehaeuser, besch);
	break;
    case gebaeude_t::unbekannt:
	switch(besch->gib_utyp()) {
	case fabrik:
	    fabriken.append(besch);
	    break;
	case denkmal:
	    denkmaeler.append(besch);
	    break;
	case sehenswuerdigkeit:
	    sehenswuerdigkeiten.append(besch);
	    break;
	case firmensitz:
		headquarter.append(besch);
		break;
	case rathaus:
	  // printf("Rathaus mit bev=%d\n", besch->gib_bauzeit());
	case special:
	    spezials.append(besch);
	    break;

	case weitere:
	{
		int checkpos=strlen(besch->gib_name());
		if(  strcmp("BusStop",besch->gib_name()+checkpos-7)==0  ) {
DBG_DEBUG("hausbauer_t::register_besch()","Bus %s",besch->gib_name());
			car_stops.append(besch);
		}
		else if(  strcmp("TrainStop",besch->gib_name()+checkpos-9)==0  ) {
DBG_DEBUG("hausbauer_t::register_besch()","Bf %s",besch->gib_name());
			train_stops.append(besch);
		}
		else if(  strcmp("ShipStop",besch->gib_name()+checkpos-8)==0  ) {
DBG_DEBUG("hausbauer_t::register_besch()","Ship %s",besch->gib_name());
			ship_stops.append(besch);
		}
		else if(  strcmp("ChannelStop",besch->gib_name()+checkpos-11)==0  ) {
DBG_DEBUG("hausbauer_t::register_besch()","Ship (channel) %s",besch->gib_name());
			ship_channel_stops.append(besch);
		}
		else if(  strcmp("PostOffice",besch->gib_name()+checkpos-10)==0  ) {
DBG_DEBUG("hausbauer_t::register_besch()","Post %s",besch->gib_name());
			post_offices.append(besch);
		}
		else if(  strcmp("StationBlg",besch->gib_name()+checkpos-10)==0  ) {
DBG_DEBUG("hausbauer_t::register_besch()","Station building %s",besch->gib_name());
			station_building.append(besch);
		}
	}
	break;
	default:
	    return false;
	}
    }
    const haus_tile_besch_t *tile;
    int i = 0;

    while((tile = besch->gib_tile(i++)) != NULL) {
	const_cast<haus_tile_besch_t *>(tile)->setze_besch(besch);
    }

    if(besch_names.get(besch->gib_name())) {
    	dbg->fatal("hausbauer_t::register_Besch()", "building %s duplicated", besch->gib_name());
    }
    besch_names.put(besch->gib_name(), besch);
    return true;
}



/**
 * Fill menu with icons of given stops from the list
 * @author Hj. Malthaner
 */
void hausbauer_t::fill_menu(werkzeug_parameter_waehler_t *wzw,
	slist_tpl<const haus_besch_t *>&stops,
	int (* werkzeug)(spieler_t *, karte_t *, koord, value_t),
	const int sound_ok,
	const int sound_ko,
	const int cost,
	const uint16 time)
{
DBG_DEBUG("hausbauer_t::fill_menu()","maximum %i",stops.count());
	for( unsigned i=0;  i<stops.count();  i++  ) {
		char buf[128];
		const haus_besch_t *besch=stops.at(i);

DBG_DEBUG("hausbauer_t::fill_menu()","try at pos %i to add %s(%p)",i,besch->gib_name(),besch);
		if(besch->gib_cursor()->gib_bild_nr(1) != -1) {
			if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {

				// only add items with a cursor
DBG_DEBUG("hausbauer_t::fill_menu()","at pos %i add %s",i,besch->gib_name());
				sprintf(buf, "%s, %d$",translator::translate(besch->gib_name()),cost/(-100));

				wzw->add_param_tool(werkzeug,
				  (const void *)besch,
				  karte_t::Z_PLAN,
				  sound_ok,
				  sound_ko,
				  besch->gib_cursor()->gib_bild_nr(1),
				  besch->gib_cursor()->gib_bild_nr(0),
				  buf );
			}
		}
	}
}




/**
 * hausbauer_t::neue_karte:
 *
 * Teilt dem Hausbauer mit, dass eine neue Karte geladen oder generiert wird.
 * In diesem Fall müssen wir die Liste der ungebauten Denkmäler wieder füllen.
 *
 * @author V. Meyer
 */
void hausbauer_t::neue_karte()
{
    slist_iterator_tpl<const haus_besch_t *>  iter(denkmaeler);

    ungebaute_denkmaeler.clear();

    while(iter.next()) {
	ungebaute_denkmaeler.append(iter.get_current());
    }
}




/**
 * hausbauer_t::umbauen:
 *
 * Ein Gebäude wird umgebaut, d.h. es bekommt einen neuen Typ und damit ein neues Bild.
 * Die Baugrube wird gesetzt.
 *
 * @param welt
 * @param gb
 * @param besch
 *
 * @author V. Meyer
 */
void hausbauer_t::umbauen(karte_t *welt,
                        gebaeude_t *gb,
			const haus_besch_t *besch)
{
    const haus_tile_besch_t *tile = besch->gib_tile(0, 0, 0);

    gb->setze_tile(tile);
    gb->renoviere();

    // Hajo: after staring a new map, build fake old buildings
    if(welt->gib_zeit_ms() < 2) {
	gb->add_alter(10000);
    }
    gb->setze_sync( true );
}



/**
 * hausbauer_t::baue:
 *
 * DIE Funktion um einen gebaeude_t zu bauen.
 * Sie kann zum einen mehrteilige Gebäude bauen, zum anderen
 * kennt sie die diversen Besonderheiten und nimmt
 * entsprechende Einstellungen vor.
 *
 * @param welt
 * @param sp
 * @param pos
 * @param layout
 * @param besch
 * @param clear
 * @param fab
 *
 * @author V. Meyer
 */
void hausbauer_t::baue(karte_t *welt,
                       spieler_t *sp,
                       koord3d pos,
		       int layout,
		       const haus_besch_t *besch,
		       bool clear,
		       void *param)
{
	koord k;
	koord dim;
	int count = 0;

	layout = besch->layout_anpassen(layout);
	dim = besch->gib_groesse(layout);

	if(besch->ist_fabrik()) {
		count = simrand(1024);
	}
	for(k.y = 0; k.y < dim.y; k.y ++) {
		for(k.x = 0; k.x < dim.x; k.x ++) {
//DBG_DEBUG("hausbauer_t::baue()","get_tile() at %i,%i",k.x,k.y);
			const haus_tile_besch_t *tile = besch->gib_tile(layout, k.x, k.y);
			// here test for good tile
			if(tile==NULL) {
				DBG_MESSAGE("hausbauer_t::baue()","get_tile() at %i,%i",k.x,k.y);
				continue;
			}
			gebaeude_t *gb = new gebaeude_t(welt, pos + k, sp, tile);

			if(besch->ist_fabrik()) {
//DBG_DEBUG("hausbauer_t::baue()","setze_fab() at %i,%i",k.x,k.y);
				gb->setze_fab((fabrik_t *)param);
			}
			// try to fake old building
			else if(welt->gib_zeit_ms() < 2) {
				// Hajo: after staring a new map, build fake old buildings
				gb->add_alter(10000);
			}
			grund_t *gr = welt->lookup(pos.gib_2d() + k)->gib_kartenboden();
			if(gr->ist_wasser()) {
				ding_t *dt = gr->obj_takeout(0);	// ensure, pos 0 is empty
				gr->obj_pri_add(gb, 0);
				if(dt) {
					gr->obj_pri_add(gb, 1);
				}
			}
			else if( ship_stops.contains(besch) ) {
				// its a dock!
				gr->obj_add(gb);
				gr->setze_besitzer(sp);
			} else {
				if(clear) {
					gr->obj_loesche_alle(sp);	// alles weg
				}
				grund_t *gr2 = new fundament_t(welt, gr->gib_pos());

//				gb->setze_bild(0, tile->gib_hintergrund(0, 0));	//prissi:???
				welt->access(gr->gib_pos().gib_2d())->boden_ersetzen(gr, gr2);
				gr = gr2;
				gr->obj_add( gb );
//DBG_DEBUG("hausbauer_t::baue()","ground count now %i",gr->obj_count());
				gr->setze_besitzer(sp);
				gr ->calc_bild();
			}
			if(besch->ist_ausflugsziel()) {
				welt->add_ausflugsziel( gb );
			}
			gb->setze_sync( true );
			if(besch->gib_typ() == gebaeude_t::unbekannt) {
				if(besch->ist_fabrik()) {
					gb->setze_count(count);
					gb->setze_anim_time(0);
				}
				else if(besch->ist_rathaus()) {
					gb->setze_besitzer(sp);
				}
				else if(besch->ist_firmensitz()) {
					gb->setze_besitzer(sp);
				}
				else if(post_offices.contains(besch)) {
					(*static_cast<halthandle_t *>(param))->add_grund(gr);
					(*static_cast<halthandle_t *>(param))->set_post_enabled( true );
				}
				else if(station_building.contains(besch)) {
					(*static_cast<halthandle_t *>(param))->add_grund(gr);
				}
				else if( ship_stops.contains(besch) ) {
					// its a dock!
					gb->setze_yoff(0);
				}
			}
		}
	}
}

gebaeude_t *hausbauer_t::neues_gebaeude(karte_t *welt,
                        spieler_t *sp,
                        koord3d pos,
			int layout,
			const haus_besch_t *besch,
			void *param)
{
    if(besch->gib_groesse(layout) != koord(1, 1)) {
	dbg->fatal("hausbauer_t::neues_gebaeude()","building %s is not 1*1", besch->gib_name());
	return NULL;
    }
    gebaeude_t *gb;
    const haus_tile_besch_t *tile = besch->gib_tile(layout, 0, 0);
    int pri = 0;
    if(besch == bahn_depot_besch) {
	gb = new bahndepot_t(welt, pos, sp, tile);
	pri = PRI_DEPOT;
    } else if(besch == tram_depot_besch) {
			gb = new bahndepot_t(welt, pos, sp, tile);
			pri = PRI_DEPOT;
    } else if(besch == monorail_depot_besch) {
			gb = new bahndepot_t(welt, pos, sp, tile);
			pri = PRI_DEPOT;
		} else if(besch == str_depot_besch) {
	gb = new strassendepot_t(welt, pos, sp, tile);
	pri = PRI_DEPOT;
    } else if(besch == sch_depot_besch) {
	gb = new schiffdepot_t(welt, pos, sp, tile);
	pri = PRI_DEPOT;
    } else {
	gb = new gebaeude_t(welt, pos, sp, tile);
    }

    grund_t *gr = welt->lookup(pos);

    gr->obj_pri_add(gb, pri);

    if(train_stops.contains(besch)  ||  car_stops.contains(besch) ||  ship_channel_stops.contains(besch) ||  besch==hausbauer_t::frachthof_besch) {
    	// is a station/bus stop
	(*static_cast<halthandle_t *>(param))->add_grund(gr);
	gr->calc_bild();
    }

    gb->setze_sync( true );
    if(besch->ist_ausflugsziel()) {
	welt->add_ausflugsziel( gb );
    }

    return gb;
}


const haus_besch_t *hausbauer_t::finde_in_liste(slist_tpl<const haus_besch_t *>  &liste, utyp utype, const char *name)
{
	if(name) {
		slist_iterator_tpl<const haus_besch_t *>  iter(liste);

		while(iter.next()) {
			if(!strcmp(iter.get_current()->gib_name(), name) && (utype == unbekannt || iter.get_current()->gib_utyp() == utype)) {
				return iter.get_current();
			}
		}
	}
	return NULL;
}



const haus_tile_besch_t *hausbauer_t::find_tile(const char *name, int idx)
{
    const haus_besch_t *besch = besch_names.get(name);

    if(besch)
	return besch->gib_tile(idx);
    else
	return NULL;
}


// timeline routines
// for time==0 these routines behave like the previous ones

const haus_besch_t *hausbauer_t::gib_special_intern(int bev, utyp utype,uint16 time)
{
	slist_iterator_tpl<const haus_besch_t *> iter(spezials);

	while(iter.next()) {
		const haus_besch_t *besch = iter.get_current();
		if(  (bev == -1 || besch->gib_bauzeit() == bev) && besch->gib_utyp() == utype) {
			// ok, now check timeline
			if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {
				return besch;
			}
		}
	}
	return NULL;
}

/**
 * tries to find something that matches this entry
 * it will skip and jump, and will never return zero, if there is at least a single valid entry in the list
 * @author Hj. Malthaner
 */
const haus_besch_t * hausbauer_t::gib_aus_liste(slist_tpl<const haus_besch_t *> &liste, int level, uint16 time)
{
	weighted_vector_tpl<const haus_besch_t *> auswahl(16);

//	DBG_MESSAGE("hausbauer_t::gib_aus_liste()","target level %i", level );
	const haus_besch_t *besch_at_least=NULL;
	slist_iterator_tpl <const haus_besch_t *> iter (liste);

	while(iter.next()) {
		const haus_besch_t *besch = iter.get_current();

		if(besch->gib_chance()>0  &&  (time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time))) {
			besch_at_least = iter.get_current();
		}

		const int thislevel = besch->gib_level();

		if(thislevel>level) {
			if(auswahl.get_count()==0) {
				// continue with search ...
				level = thislevel;
			}
			else {
				// ok, we found something
				break;
			}
		}

		if(thislevel==level  &&  besch->gib_chance()>0) {
			if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {
//					DBG_MESSAGE("hausbauer_t::gib_aus_liste()","appended %s at %i", besch->gib_name(), thislevel );
				auswahl.append(besch,besch->gib_chance(),4);
			}
		}

	}

	if(auswahl.get_sum_weight()==0) {
		// this is some level below, but at least it is something
		return besch_at_least;
	}
	if(auswahl.get_count()==1) {
		return auswahl.at(0);
	}
	// now there is something to choose
	return auswahl.at_weight( simrand(auswahl.get_sum_weight()) );
}



// get a random object
const haus_besch_t *hausbauer_t::waehle_aus_liste(slist_tpl<const haus_besch_t *> &liste, uint16 time)
{
	if(liste.count()) {
		// previously just returned a random object; however, now we do als look at the chance entry
		weighted_vector_tpl<const haus_besch_t *> auswahl(16);
		slist_iterator_tpl <const haus_besch_t *> iter (liste);

		while(iter.next()) {
			const haus_besch_t *besch = iter.get_current();
			if(besch->gib_chance()>0  &&  (time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time))) {
//				DBG_MESSAGE("hausbauer_t::gib_aus_liste()","appended %s at %i", besch->gib_name(), thislevel );
				auswahl.append(besch,besch->gib_chance(),4);
			}
		}
		// now look, what we have got ...
		if(auswahl.get_sum_weight()==0) {
			return NULL;
		}
		if(auswahl.get_count()==1) {
			return auswahl.at(0);
		}
		// now there is something to choose
		return auswahl.at_weight( simrand(auswahl.get_sum_weight()) );
	}
	return NULL;
}
