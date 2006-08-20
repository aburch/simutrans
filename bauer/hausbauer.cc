/*
 * hausbauer.cc
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <string.h>

#include "hausbauer.h"

#include "../simdebug.h"
#include "../simworld.h"
#include "../simwerkz.h"
#include "../simdepot.h"
#include "../simhalt.h"
#include "../simtools.h"

#include "../boden/grund.h"
#include "../boden/fundament.h"

#include "../besch/haus_besch.h"
#include "../besch/spezial_obj_tpl.h"
#include "../besch/skin_besch.h"

#include "../dataobj/translator.h"

#include "../dings/gebaeude.h"

// Hajo: these are needed to build the menu entries
#include "../gui/werkzeug_parameter_waehler.h"

#include "../tpl/weighted_vector_tpl.h"

#include "../utils/simstring.h"

/*
 * Die verschiedenen Gebäudegruppen sind in eigenen Listen gesammelt.
 */
slist_tpl<const haus_besch_t *> hausbauer_t::alle;
slist_tpl<const haus_besch_t *> hausbauer_t::wohnhaeuser;
slist_tpl<const haus_besch_t *> hausbauer_t::gewerbehaeuser;
slist_tpl<const haus_besch_t *> hausbauer_t::industriehaeuser;
slist_tpl<const haus_besch_t *> hausbauer_t::fabriken;
slist_tpl<const haus_besch_t *> hausbauer_t::sehenswuerdigkeiten_land;
slist_tpl<const haus_besch_t *> hausbauer_t::sehenswuerdigkeiten_city;
slist_tpl<const haus_besch_t *> hausbauer_t::rathaeuser;
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
const haus_besch_t *hausbauer_t::bahn_depot_besch = NULL;
const haus_besch_t *hausbauer_t::tram_depot_besch = NULL;
const haus_besch_t *hausbauer_t::str_depot_besch = NULL;
const haus_besch_t *hausbauer_t::sch_depot_besch = NULL;
const haus_besch_t *hausbauer_t::monorail_depot_besch = NULL;
const haus_besch_t *hausbauer_t::monorail_foundation_besch = NULL;

slist_tpl<const haus_besch_t *> hausbauer_t::station_building;
slist_tpl<const haus_besch_t *> hausbauer_t::headquarter;
slist_tpl<const haus_besch_t *> hausbauer_t::air_depot;


static spezial_obj_tpl<haus_besch_t> spezial_objekte[] = {
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
				sehenswuerdigkeiten_land.append(besch);
				break;
			case firmensitz:
				headquarter.append(besch);
				break;
			case rathaus:
				// printf("Rathaus mit bev=%d\n", besch->gib_bauzeit());
				rathaeuser.append(besch);
				break;
			case special:
				sehenswuerdigkeiten_city.append(besch);
				break;
			case weitere:
				{
				// allow for more than one air depot
					int checkpos=strlen(besch->gib_name());
					if(  strcmp("AirDepot",besch->gib_name()+checkpos-8)==0  ) {
DBG_DEBUG("hausbauer_t::register_besch()","AirDepot %s",besch->gib_name());
						air_depot.append(besch);
					}
				}
				break;

			default:
				// usually station buldings
				if(besch->gib_utyp()>=bahnhof  &&  besch->gib_utyp()<=lagerhalle) {
					station_building.append(besch);
DBG_DEBUG("hausbauer_t::register_besch()","Station %s",besch->gib_name());
				}
				else {
DBG_DEBUG("hausbauer_t::register_besch()","unknown subtype %i of %s: ignored",besch->gib_utyp(),besch->gib_name());
					return false;
				}
				break;
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
	const sint64 cost,
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
				int n=sprintf(buf, "%s ",translator::translate(besch->gib_name()));
				money_to_string(buf+n, (cost*besch->gib_level()*besch->gib_b()*besch->gib_h())/-100.0);

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
 * Fill menu with icons of given stops from the list
 * @author Hj. Malthaner
 */
void hausbauer_t::fill_menu(werkzeug_parameter_waehler_t *wzw,
	hausbauer_t::utyp utyp,
	int (* werkzeug)(spieler_t *, karte_t *, koord, value_t),
	const int sound_ok,
	const int sound_ko,
	const sint64 cost,
	const uint16 time)
{
DBG_DEBUG("hausbauer_t::fill_menu()","maximum %i",station_building.count());
	for( unsigned i=0;  i<station_building.count();  i++  ) {
		char buf[128];
		const haus_besch_t *besch=station_building.at(i);

//DBG_DEBUG("hausbauer_t::fill_menu()","try at pos %i to add %s (%p)",i,besch->gib_name(),besch);
		if(besch->gib_utyp()==utyp  &&  besch->gib_cursor()->gib_bild_nr(1) != -1) {
			if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {

				// only add items with a cursor
DBG_DEBUG("hausbauer_t::fill_menu()","at pos %i add %s",i,besch->gib_name());
				int n=sprintf(buf, "%s ",translator::translate(besch->gib_name()));
				money_to_string(buf+n, (cost*besch->gib_level()*besch->gib_b()*besch->gib_h())/-100.0);

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
void hausbauer_t::umbauen(karte_t */*welt*/,gebaeude_t *gb, const haus_besch_t *besch)
{
	const haus_tile_besch_t *tile = besch->gib_tile(0, 0, 0);

	gb->setze_tile(tile);
	gb->renoviere();
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
 * The ground will be NOT set to foundation. This must be done before, if neccessary!
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
void hausbauer_t::baue(karte_t *welt, spieler_t *sp, koord3d pos, int layout, const haus_besch_t *besch, bool clear, void *param)
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
			else if( besch->gib_utyp()==hausbauer_t::hafen ) {
				// its a dock!
				gr->obj_add(gb);
				gr->setze_besitzer(sp);
			} else {
				if(clear) {
					gr->obj_loesche_alle(sp);	// alles weg
				}
				grund_t *gr2 = new fundament_t(welt, gr->gib_pos());
				welt->access(gr->gib_pos().gib_2d())->boden_ersetzen(gr, gr2);
				gr = gr2;
//DBG_DEBUG("hausbauer_t::baue()","ground count now %i",gr->obj_count());
				gr->setze_besitzer(sp);
				gr ->calc_bild();
				gr->obj_add( gb );
				gb->setze_pos( gr->gib_pos() );
				if(welt->ist_in_kartengrenzen(pos.gib_2d()+koord(1,0))) {
					welt->lookup(pos.gib_2d()+koord(1,0))->gib_kartenboden()->calc_bild();
				}
				if(welt->ist_in_kartengrenzen(pos.gib_2d()+koord(0,1))) {
					welt->lookup(pos.gib_2d()+koord(0,1))->gib_kartenboden()->calc_bild();
				}
				if(welt->ist_in_kartengrenzen(pos.gib_2d()+koord(1,1))) {
					welt->lookup(pos.gib_2d()+koord(0,1))->gib_kartenboden()->calc_bild();
				}
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
				else if(station_building.contains(besch)) {
					(*static_cast<halthandle_t *>(param))->add_grund(gr);
				}
				 if( besch->gib_utyp()==hausbauer_t::hafen ) {
					// its a dock!
					gb->setze_yoff(0);
				}
			}
		}
	}
}



/*
 * builds all kind of houses, including stops and depots
 * may change the ordering of objects in the dinglist, if the desired position is not available
 */
gebaeude_t *
hausbauer_t::neues_gebaeude(karte_t *welt, spieler_t *sp, koord3d pos, int layout, const haus_besch_t *besch, void *param)
{
	if(besch->gib_groesse(layout) != koord(1, 1)) {
		dbg->fatal("hausbauer_t::neues_gebaeude()","building %s is not 1*1", besch->gib_name());
		return NULL;
	}
	gebaeude_t *gb;
	const haus_tile_besch_t *tile = besch->gib_tile(layout, 0, 0);
	int pri = PRI_DEPOT;

	if(besch == bahn_depot_besch) {
		gb = new bahndepot_t(welt, pos, sp, tile);
	} else if(besch == tram_depot_besch) {
		gb = new tramdepot_t(welt, pos, sp, tile);
	} else if(besch == monorail_depot_besch) {
		gb = new monoraildepot_t(welt, pos, sp, tile);
	} else if(besch == str_depot_besch) {
		gb = new strassendepot_t(welt, pos, sp, tile);
	} else if(besch == sch_depot_besch) {
		gb = new schiffdepot_t(welt, pos, sp, tile);
	} else if(air_depot.contains(besch)) {
		gb = new airdepot_t(welt, pos, sp, tile);
	} else {
		pri = 0;
		gb = new gebaeude_t(welt, pos, sp, tile);
	}
//DBG_MESSAGE("hausbauer_t::neues_gebaeude()","building stop pri=%i",pri);

	grund_t *gr = welt->lookup(pos);
	if(pri==0) {
		// add it after roadsigns, bridges, and overheadwires, but before any cars ...
		for( int i=0;  i<255;  i++  ) {
			ding_t *dt=gr->obj_bei(i);
			if(dt==NULL) {
				gr->obj_pri_add(gb, i);
				break;
			}
			else if(dt->gib_typ()>=ding_t::automobil  &&  dt->gib_typ()<=ding_t::fussgaenger) {
				gr->obj_takeout(i);
				gr->obj_pri_add(gb, i);
				gr->obj_pri_add(dt, i+1);
				break;
			}
		}
	}
	else {
		// depots MUST be at position PRI_DEPOT
		ding_t *dt=gr->obj_takeout(pri);
		gr->obj_pri_add(gb, pri);
		if(dt) {
			gr->obj_pri_add(dt, pri-1);
		}
	}

	if(station_building.contains(besch)) {
		// is a station/bus stop
		(*static_cast<halthandle_t *>(param))->add_grund(gr);
		gr->calc_bild();
	}

	gb->setze_sync( true );
	if(besch->ist_ausflugsziel()) {
		welt->add_ausflugsziel( gb );
	}

//    DBG_MESSAGE("hausbauer_t::neues_gebaeude()","pri=0 %p",welt->lookup(pos)->obj_bei(0) );
//    DBG_MESSAGE("hausbauer_t::neues_gebaeude()","pri=11 %p",welt->lookup(pos)->obj_bei(PRI_DEPOT) );

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

const haus_besch_t *hausbauer_t::gib_random_station(const enum utyp utype,const uint16 time,const uint8 enables)
{
	slist_iterator_tpl<const haus_besch_t *> iter(hausbauer_t::station_building);
	slist_tpl<const haus_besch_t *>stops;

	while(iter.next()) {
		const haus_besch_t *besch = iter.get_current();
		if(besch->gib_utyp()==utype  &&  (besch->get_enabled()&enables)!=0) {
			// ok, now check timeline
			if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {
				stops.append(besch);
			}
		}
	}
	// found something?
	if(stops.count()>0) {
		if(stops.count()==1) {
			return stops.at(0);
		}
		return stops.at(simrand(stops.count()));
	}
	return NULL;
}

const haus_besch_t *hausbauer_t::gib_special_intern(int bev, utyp utype,uint16 time)
{
	weighted_vector_tpl<const haus_besch_t *> auswahl(16);
	slist_iterator_tpl<const haus_besch_t *> iter( utype==rathaus ? rathaeuser : (bev==-1 ? sehenswuerdigkeiten_land : sehenswuerdigkeiten_city) );

	while(iter.next()) {
		const haus_besch_t *besch = iter.get_current();
		if(bev == -1 || besch->gib_bauzeit() == bev) {
			// ok, now check timeline
			if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {
				auswahl.append(besch,besch->gib_chance(),4);
			}
		}
	}
	if(auswahl.get_sum_weight()==0) {
		// this is some level below, but at least it is something
		return NULL;
	}
	if(auswahl.get_count()==1) {
		return auswahl.at(0);
	}
	// now there is something to choose
	return auswahl.at_weight( simrand(auswahl.get_sum_weight()) );
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
