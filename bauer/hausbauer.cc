/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <algorithm>
#include <string.h>

#include "../besch/haus_besch.h"
#include "../besch/skin_besch.h"
#include "../besch/spezial_obj_tpl.h"

#include "../boden/boden.h"
#include "../boden/fundament.h"

#include "../dataobj/translator.h"

#include "../dings/leitung2.h"
#include "../dings/zeiger.h"

#include "../gui/karte.h"
#include "../gui/werkzeug_parameter_waehler.h"

#include "../simdebug.h"
#include "../simdepot.h"
#include "../simhalt.h"
#include "../simskin.h"
#include "../simtools.h"
#include "../simwerkz.h"
#include "../simworld.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/weighted_vector_tpl.h"
#include "../utils/simstring.h"
#include "hausbauer.h"

/*
 * Die verschiedenen Gebäudegruppen sind in eigenen Listen gesammelt.
 */
static vector_tpl<const haus_besch_t*> wohnhaeuser;
static vector_tpl<const haus_besch_t*> gewerbehaeuser;
static vector_tpl<const haus_besch_t*> industriehaeuser;
slist_tpl<const haus_besch_t *> hausbauer_t::sehenswuerdigkeiten_land;
slist_tpl<const haus_besch_t *> hausbauer_t::sehenswuerdigkeiten_city;
slist_tpl<const haus_besch_t *> hausbauer_t::rathaeuser;
slist_tpl<const haus_besch_t *> hausbauer_t::denkmaeler;
slist_tpl<const haus_besch_t *> hausbauer_t::ungebaute_denkmaeler;

/*
 * Diese Tabelle ermöglicht das Auffinden einer Beschreibung durch ihren Namen
 */
static stringhashtable_tpl<const haus_besch_t*> besch_names;

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

vector_tpl<const haus_besch_t *> hausbauer_t::station_building;
vector_tpl<const haus_besch_t *> hausbauer_t::headquarter;
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


static bool compare_haus_besch(const haus_besch_t* a, const haus_besch_t* b)
{
	int diff = a->gib_level() - b->gib_level();
	if (diff == 0) {
		/* Gleiches Level - wir führen eine künstliche, aber eindeutige Sortierung
		 * über den Namen herbei. */
		diff = strcmp(a->gib_name(), b->gib_name());
	}
	return diff < 0;
}


bool hausbauer_t::alles_geladen()
{
	std::sort(wohnhaeuser.begin(),      wohnhaeuser.end(),      compare_haus_besch);
	std::sort(gewerbehaeuser.begin(),   gewerbehaeuser.end(),   compare_haus_besch);
	std::sort(industriehaeuser.begin(), industriehaeuser.end(), compare_haus_besch);
	std::sort(station_building.begin(), station_building.end(), compare_haus_besch);
	std::sort(headquarter.begin(),      headquarter.end(),      compare_haus_besch);
	warne_ungeladene(spezial_objekte, 10);
	return true;
}


bool hausbauer_t::register_besch(const haus_besch_t *besch)
{
	::register_besch(spezial_objekte, besch);

	switch(besch->gib_typ()) {
		case gebaeude_t::wohnung:   wohnhaeuser.push_back(besch);      break;
		case gebaeude_t::industrie: industriehaeuser.push_back(besch); break;
		case gebaeude_t::gewerbe:   gewerbehaeuser.push_back(besch);   break;

		case gebaeude_t::unbekannt:
		switch (besch->gib_utyp()) {
			case haus_besch_t::denkmal:           denkmaeler.append(besch);               break;
			case haus_besch_t::attraction_land:   sehenswuerdigkeiten_land.append(besch); break;
			case haus_besch_t::firmensitz:        headquarter.append(besch,4);              break;
			case haus_besch_t::rathaus:           rathaeuser.append(besch);               break;
			case haus_besch_t::attraction_city:   sehenswuerdigkeiten_city.append(besch); break;

			case haus_besch_t::fabrik: break;

			case haus_besch_t::weitere:
				{
				// allow for more than one air depot
					size_t checkpos=strlen(besch->gib_name());
					if(  strcmp("AirDepot",besch->gib_name()+checkpos-8)==0  ) {
DBG_DEBUG("hausbauer_t::register_besch()","AirDepot %s",besch->gib_name());
						air_depot.append(besch);
					}
				}
				break;

			default:
				// usually station buldings
				if(  besch->gib_utyp()>=haus_besch_t::bahnhof  &&  besch->gib_utyp()<=haus_besch_t::lagerhalle  ) {
					station_building.append(besch,16);
DBG_DEBUG("hausbauer_t::register_besch()","Station %s",besch->gib_name());
				}
				else {
DBG_DEBUG("hausbauer_t::register_besch()","unknown subtype %i of %s: ignored",besch->gib_utyp(),besch->gib_name());
					return false;
				}
				break;
		}
	}

	/* supply the tiles with a pointer back to the matchin description
	 * this is needed, since each building is build of seperate tiles,
	 * even if it is part of the same description (haus_besch_t)
	 */
	const int max_index = besch->gib_all_layouts()*besch->gib_groesse().x*besch->gib_groesse().y;
	for( int i=0;  i<max_index;  i++  ) {
		const_cast<haus_tile_besch_t *>(besch->gib_tile(i))->setze_besch(besch);
	}

	if(besch_names.get(besch->gib_name())) {
		dbg->fatal("hausbauer_t::register_Besch()", "building %s duplicated", besch->gib_name());
	}
	besch_names.put(besch->gib_name(), besch);
	return true;
}



void hausbauer_t::fill_menu(werkzeug_parameter_waehler_t* wzw, slist_tpl<const haus_besch_t*>& stops, tool_func werkzeug, const int sound_ok, const int sound_ko, const sint64 cost, const karte_t* welt)
{
	const uint16 time = welt->get_timeline_year_month();
DBG_DEBUG("hausbauer_t::fill_menu()","maximum %i",stops.count());
	for (slist_iterator_tpl<const haus_besch_t*> i(stops); i.next();) {
		const haus_besch_t* besch = i.get_current();
		DBG_DEBUG("hausbauer_t::fill_menu()", "try to add %s(%p)", besch->gib_name(), besch);
		if(besch->gib_cursor()->gib_bild_nr(1) != IMG_LEER) {
			if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {

				// only add items with a cursor
				DBG_DEBUG("hausbauer_t::fill_menu()", "add %s", besch->gib_name());
				char buf[128];
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



void hausbauer_t::fill_menu(werkzeug_parameter_waehler_t* wzw, haus_besch_t::utyp utyp, tool_func werkzeug, const int sound_ok, const int sound_ko, const sint64 cost, const karte_t* welt)
{
	const uint16 time = welt->get_timeline_year_month();
DBG_DEBUG("hausbauer_t::fill_menu()","maximum %i",station_building.get_count());
	for(  vector_tpl<const haus_besch_t *>::const_iterator iter = station_building.begin(), end = station_building.end();  iter != end;  ++iter  ) {
		const haus_besch_t* besch = (*iter);
//		DBG_DEBUG("hausbauer_t::fill_menu()", "try to add %s (%p)", besch->gib_name(), besch);
		if(besch->gib_utyp()==utyp  &&  besch->gib_cursor()->gib_bild_nr(1) != IMG_LEER) {
			if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {

				// only add items with a cursor
				DBG_DEBUG("hausbauer_t::fill_menu()", "add %s", besch->gib_name());
				char buf[128];
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


// new map => reset monument list to ensure every monument appears only once per map
void hausbauer_t::neue_karte()
{
	slist_iterator_tpl<const haus_besch_t *>  iter(denkmaeler);
	ungebaute_denkmaeler.clear();
	while(iter.next()) {
		ungebaute_denkmaeler.append(iter.get_current());
	}
}



void hausbauer_t::remove( karte_t *welt, spieler_t *sp, gebaeude_t *gb )
{
	const haus_tile_besch_t *tile  = gb->gib_tile();
	const haus_besch_t *hb = tile->gib_besch();

	// get startpos and size
	const koord3d pos = gb->gib_pos() - koord3d( tile->gib_offset(), 0 );
	koord size = tile->gib_besch()->gib_groesse( tile->gib_layout() );
	koord k;

	// then remove factory
	fabrik_t *fab = gb->get_fabrik();
	if(fab) {
		// first remove fabrik_t pointers
		for(k.y = 0; k.y < size.y; k.y ++) {
			for(k.x = 0; k.x < size.x; k.x ++) {
				grund_t *gr = welt->lookup(koord3d(k,0)+pos);
				gebaeude_t *gb_part = gr->find<gebaeude_t>();
				if(gb_part) {
					// there may be buildings with holes, so we only remove our or the hole!
					if(gb_part->gib_tile()->gib_besch()==hb) {
						gb_part->setze_fab( NULL );
						planquadrat_t *plan = welt->access( k+pos.gib_2d() );
						for( int i=plan->get_haltlist_count()-1;  i>=0;  i--  ) {
							halthandle_t halt = plan->get_haltlist()[i];
							halt->remove_fabriken( fab );
							plan->remove_from_haltlist( welt, halt );
						}
					}
				}
			}
		}
		// remove all transformers
		for(k.y = pos.y; k.y < pos.y+size.y;  k.y ++) {
			k.x = pos.x-1;
			grund_t *gr = welt->lookup_kartenboden(k);
			if(gr) {
				senke_t *sk = gr->find<senke_t>();
				if(sk) delete sk;
				pumpe_t *pp = gr->find<pumpe_t>();
				if(pp) delete pp;
			}
			k.x = pos.x+size.x;
			gr = welt->lookup_kartenboden(k);
			if(gr) {
				senke_t *sk = gr->find<senke_t>();
				if(sk) delete sk;
				pumpe_t *pp = gr->find<pumpe_t>();
				if(pp) delete pp;
			}
		}
		for(k.x = pos.x; k.x < pos.x+size.x;  k.x ++) {
			k.y = pos.y-1;
			grund_t *gr = welt->lookup_kartenboden(k);
			if(gr) {
				senke_t *sk = gr->find<senke_t>();
				if(sk) delete sk;
				pumpe_t *pp = gr->find<pumpe_t>();
				if(pp) delete pp;
			}
			k.y = pos.y+size.y;
			gr = welt->lookup_kartenboden(k);
			if(gr) {
				senke_t *sk = gr->find<senke_t>();
				if(sk) delete sk;
				pumpe_t *pp = gr->find<pumpe_t>();
				if(pp) delete pp;
			}
		}
		// end clean up transformers
		welt->rem_fab(fab);
	}

	// delete just our house
	for(k.y = 0; k.y < size.y; k.y ++) {
		for(k.x = 0; k.x < size.x; k.x ++) {
			grund_t *gr = welt->lookup(koord3d(k,0)+pos);
			if(gr) {
				gebaeude_t *gb_part = gr->find<gebaeude_t>();
				// there may be buildings with holes, so we only remove our!
				if(gb_part  &&  gb_part->gib_tile()->gib_besch()==hb) {
					// ok, now we can go on with deletion
					gb_part->entferne( sp );
					delete gb_part;
					// if this was a station building: delete ground
					if(gr->gib_halt().is_bound()) {
						gr->gib_halt()->rem_grund(gr);
					}
					// and maybe restore land below
					if(gr->gib_typ()==grund_t::fundament) {
						const koord newk = k+pos.gib_2d();
						const uint8 new_slope = gr->gib_hoehe()==welt->min_hgt(newk) ? 0 : welt->calc_natural_slope(newk);
						welt->access(newk)->kartenboden_setzen(new boden_t(welt, koord3d(newk,welt->min_hgt(newk) ), new_slope) );
						// there might be walls from foundations left => thus some tiles may needs to be redraw
						if(new_slope!=0) {
							if(pos.x<welt->gib_groesse_x()-1)
								welt->lookup_kartenboden(newk+koord::ost)->calc_bild();
							if(pos.y<welt->gib_groesse_y()-1)
								welt->lookup_kartenboden(newk+koord::sued)->calc_bild();
						}
					}
				}
			}
		}
	}
}



gebaeude_t* hausbauer_t::baue(karte_t* welt, spieler_t* sp, koord3d pos, int org_layout, const haus_besch_t* besch, void* param)
{
	gebaeude_t* first_building = NULL;
	koord k;
	koord dim;

	uint8 layout = besch->layout_anpassen(org_layout);
	dim = besch->gib_groesse(org_layout);
	bool needs_ground_recalc = false;

	for(k.y = 0; k.y < dim.y; k.y ++) {
		for(k.x = 0; k.x < dim.x; k.x ++) {
//DBG_DEBUG("hausbauer_t::baue()","get_tile() at %i,%i",k.x,k.y);
			const haus_tile_besch_t *tile = besch->gib_tile(layout, k.x, k.y);
			// here test for good tile
			if (tile == NULL || (
						k != koord(0, 0) &&
						besch->gib_utyp() != haus_besch_t::hafen &&
						tile->gib_hintergrund(0, 0, 0) == IMG_LEER &&
						tile->gib_vordergrund(0, 0)    == IMG_LEER
					)) {
						// may have a rotation, that is not recoverable
						DBG_MESSAGE("hausbauer_t::baue()","get_tile() empty at %i,%i",k.x,k.y);
				continue;
			}
			gebaeude_t *gb = new gebaeude_t(welt, pos + k, sp, tile);
			if (first_building == NULL) {
				first_building = gb;
			}

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
				gr->obj_add(gb);
			} else if (besch->gib_utyp() == haus_besch_t::hafen) {
				// its a dock!
				gr->obj_add(gb);
			} else {
				// very likely remove all
				leitung_t *lt = NULL;
				if(!gr->hat_wege()) {
					lt = gr->find<leitung_t>();
					if(lt) {
						gr->obj_remove(lt);
					}
					gr->obj_loesche_alle(sp);	// alles weg außer vehikel ...
				}
				needs_ground_recalc |= gr->gib_grund_hang()!=hang_t::flach;
				grund_t *gr2 = new fundament_t(welt, gr->gib_pos(), gr->gib_grund_hang());
				welt->access(gr->gib_pos().gib_2d())->boden_ersetzen(gr, gr2);
				gr = gr2;
//DBG_DEBUG("hausbauer_t::baue()","ground count now %i",gr->obj_count());
				gr->obj_add( gb );
				gb->setze_pos( gr->gib_pos() );
				if(lt) {
					gr->obj_add( lt );
				}
				if(needs_ground_recalc  &&  welt->ist_in_kartengrenzen(pos.gib_2d()+koord(1,1))  &&  (k.y+1==dim.y  ||  k.x+1==dim.x)) {
					welt->lookup(pos.gib_2d()+k+koord(1,0))->gib_kartenboden()->calc_bild();
					welt->lookup(pos.gib_2d()+k+koord(0,1))->gib_kartenboden()->calc_bild();
					welt->lookup(pos.gib_2d()+k+koord(1,1))->gib_kartenboden()->calc_bild();
				}
			}
			if(besch->ist_ausflugsziel()) {
				welt->add_ausflugsziel( gb );
			}
			if(besch->gib_typ() == gebaeude_t::unbekannt) {
				if(station_building.is_contained(besch)) {
					(*static_cast<halthandle_t *>(param))->add_grund(gr);
				}
				if (besch->gib_utyp() == haus_besch_t::hafen) {
					// its a dock!
					gb->setze_yoff(0);
				}
			}
			gr->calc_bild();
			reliefkarte_t::gib_karte()->calc_map_pixel(gr->gib_pos().gib_2d());
		}
		if(besch->gib_utyp()==haus_besch_t::denkmal) {
			ungebaute_denkmaeler.remove( besch );
		}
	}
	return first_building;
}



gebaeude_t *
hausbauer_t::neues_gebaeude(karte_t *welt, spieler_t *sp, koord3d pos, int built_layout, const haus_besch_t *besch, void *param)
{
	gebaeude_t *gb;

	uint8 corner_layout = 6;	// assume single building (for more than 4 layouts)

	// adjust layout of neighbouring building
	if(besch->gib_utyp()>=8  &&  besch->gib_all_layouts()>1) {

		int layout = built_layout & 9;

		// detect if we are connected at far (north/west) end
		sint8 offset = welt->lookup( pos )->gib_weg_yoff()/TILE_HEIGHT_STEP;
		grund_t * gr = welt->lookup( pos+koord3d( (layout & 1 ? koord::ost : koord::sued), offset) );
		if(!gr) {
			// check whether bridge end tile
			grund_t * gr_tmp = welt->lookup( pos+koord3d( (layout & 1 ? koord::ost : koord::sued),offset - 1) );
			if(gr_tmp && gr_tmp->gib_weg_yoff()/TILE_HEIGHT_STEP == 1) {
				gr = gr_tmp;
			}
		}
		if(gr) {
			gebaeude_t* gb = gr->find<gebaeude_t>();
			if(gb  &&  gb->gib_tile()->gib_besch()->gib_utyp()>=8) {
				corner_layout &= ~2; // clear near bit
				if(gb->gib_tile()->gib_besch()->gib_all_layouts()>4) {
					koord xy = gb->gib_tile()->gib_offset();
					uint8 layoutbase = gb->gib_tile()->gib_layout();
					if((layoutbase & 1) == (layout & 1)) {
						layoutbase &= 0xb; // clear near bit on neighbour
						gb->setze_tile(gb->gib_tile()->gib_besch()->gib_tile(layoutbase, xy.x, xy.y));
					}
				}
			}
		}

		// detect if near (south/east) end
		gr = welt->lookup( pos+koord3d( (layout & 1 ? koord::west : koord::nord), offset) );
		if(!gr) {
			// check whether bridge end tile
			grund_t * gr_tmp = welt->lookup( pos+koord3d( (layout & 1 ? koord::west : koord::nord),offset - 1) );
			if(gr_tmp && gr_tmp->gib_weg_yoff()/TILE_HEIGHT_STEP == 1) {
				gr = gr_tmp;
			}
		}
		if(gr) {
			gebaeude_t* gb = gr->find<gebaeude_t>();
			if(gb  &&  gb->gib_tile()->gib_besch()->gib_utyp()>=8) {
				corner_layout &= ~4; // clear far bit
				if(gb->gib_tile()->gib_besch()->gib_all_layouts()>4) {
					koord xy = gb->gib_tile()->gib_offset();
					uint8 layoutbase = gb->gib_tile()->gib_layout();
					if((layoutbase & 1) == (layout & 1)) {
						layoutbase &= 0xd; // clear far bit on neighbour
						gb->setze_tile(gb->gib_tile()->gib_besch()->gib_tile(layoutbase, xy.x, xy.y));
					}
				}
			}
		}
	}

	// adjust layouts of the new building
	if(besch->gib_all_layouts()>4) {
		built_layout = (corner_layout | (built_layout&9) ) % besch->gib_all_layouts();
	}

	const haus_tile_besch_t *tile = besch->gib_tile(built_layout, 0, 0);
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
		gb = new gebaeude_t(welt, pos, sp, tile);
	}
//DBG_MESSAGE("hausbauer_t::neues_gebaeude()","building stop pri=%i",pri);

	// remove pointer
	grund_t *gr = welt->lookup(pos);
	zeiger_t* zeiger = gr->find<zeiger_t>();
	if (zeiger) gr->obj_remove(zeiger);

	gr->obj_add(gb);

	if(station_building.is_contained(besch)) {
		// is a station/bus stop
		(*static_cast<halthandle_t *>(param))->add_grund(gr);
		gr->calc_bild();
	}

	if(besch->ist_ausflugsziel()) {
		welt->add_ausflugsziel( gb );
	}
	reliefkarte_t::gib_karte()->calc_map_pixel(gb->gib_pos().gib_2d());

	return gb;
}



const haus_tile_besch_t *hausbauer_t::find_tile(const char *name, int idx)
{
	const haus_besch_t *besch = besch_names.get(name);
	if(besch) {
		const int size = besch->gib_h()*besch->gib_b();
		if(  idx >= besch->gib_all_layouts()*size  ) {
			return NULL;
			// below: try to keep as much of the orientation as possible (not a good idea)
//			idx = (idx>2*size) ? idx%size : idx%(2*size);
		}
		return besch->gib_tile(idx);
	}
//	DBG_MESSAGE("hausbauer_t::find_tile()","\"%s\" not in hashtable",name);
	return NULL;
}



const haus_besch_t* hausbauer_t::gib_random_station(const haus_besch_t::utyp utype, const uint16 time, const uint8 enables)
{
	vector_tpl<const haus_besch_t*> stops;

	for(  vector_tpl<const haus_besch_t *>::const_iterator iter = station_building.begin(), end = station_building.end();  iter != end;  ++iter  ) {
		const haus_besch_t* besch = (*iter);
		if(besch->gib_utyp()==utype  &&  (besch->get_enabled()&enables)!=0) {
			// ok, now check timeline
			if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {
				stops.push_back(besch);
			}
		}
	}
	return stops.empty() ? NULL : stops[simrand(stops.get_count())];
}



const haus_besch_t* hausbauer_t::gib_special(int bev, haus_besch_t::utyp utype, uint16 time, climate cl)
{
	weighted_vector_tpl<const haus_besch_t *> auswahl(16);
	slist_iterator_tpl<const haus_besch_t*> iter(utype == haus_besch_t::rathaus ? rathaeuser : (bev == -1 ? sehenswuerdigkeiten_land : sehenswuerdigkeiten_city));

	while(iter.next()) {
		const haus_besch_t *besch = iter.get_current();
		if(bev == -1 || besch->gib_bauzeit()==bev) {
			if(cl==MAX_CLIMATES  ||  besch->is_allowed_climate(cl)) {
				// ok, now check timeline
				if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {
					auswahl.append(besch,besch->gib_chance(),4);
				}
			}
		}
	}
	if (auswahl.empty()) {
		return 0;
	}
	else	if(auswahl.get_count()==1) {
		return auswahl.front();
	}
	// now there is something to choose
	return auswahl.at_weight( simrand(auswahl.get_sum_weight()) );
}



/**
 * tries to find something that matches this entry
 * it will skip and jump, and will never return zero, if there is at least a single valid entry in the list
 * @author Hj. Malthaner
 */
static const haus_besch_t* gib_aus_liste(const vector_tpl<const haus_besch_t*>& liste, int level, uint16 time, climate cl)
{
	weighted_vector_tpl<const haus_besch_t *> auswahl(16);

//	DBG_MESSAGE("hausbauer_t::gib_aus_liste()","target level %i", level );
	const haus_besch_t *besch_at_least=NULL;
	for (vector_tpl<const haus_besch_t*>::const_iterator i = liste.begin(), end = liste.end(); i != end; ++i) {
		const haus_besch_t* besch = *i;
		if(	besch->is_allowed_climate(cl)  &&
			besch->gib_chance()>0  &&
			(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time))) {
			besch_at_least = besch;
		}

		const int thislevel = besch->gib_level();
		if(thislevel>level) {
			if (auswahl.empty()) {
				// continue with search ...
				level = thislevel;
			}
			else {
				// ok, we found something
				break;
			}
		}

		if(thislevel==level  &&  besch->gib_chance()>0) {
			if(cl==MAX_CLIMATES  ||  besch->is_allowed_climate(cl)) {
				if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {
//				DBG_MESSAGE("hausbauer_t::gib_aus_liste()","appended %s at %i", besch->gib_name(), thislevel );
					auswahl.append(besch,besch->gib_chance(),4);
				}
			}
		}
	}

	if(auswahl.get_sum_weight()==0) {
		// this is some level below, but at least it is something
		return besch_at_least;
	}
	if(auswahl.get_count()==1) {
		return auswahl.front();
	}
	// now there is something to choose
	return auswahl.at_weight( simrand(auswahl.get_sum_weight()) );
}


const haus_besch_t* hausbauer_t::gib_gewerbe(int level, uint16 time, climate cl)
{
	return gib_aus_liste(gewerbehaeuser, level, time, cl);
}


const haus_besch_t* hausbauer_t::gib_industrie(int level, uint16 time, climate cl)
{
	return gib_aus_liste(industriehaeuser, level, time, cl);
}


const haus_besch_t* hausbauer_t::gib_wohnhaus(int level, uint16 time, climate cl)
{
	return gib_aus_liste(wohnhaeuser, level, time, cl);
}


// get a random object
const haus_besch_t *hausbauer_t::waehle_aus_liste(slist_tpl<const haus_besch_t *> &liste, uint16 time, climate cl)
{
	if (!liste.empty()) {
		// previously just returned a random object; however, now we do als look at the chance entry
		weighted_vector_tpl<const haus_besch_t *> auswahl(16);
		slist_iterator_tpl <const haus_besch_t *> iter (liste);

		while(iter.next()) {
			const haus_besch_t *besch = iter.get_current();
			if((cl==MAX_CLIMATES  ||  besch->is_allowed_climate(cl))  &&  besch->gib_chance()>0  &&  (time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time))) {
//				DBG_MESSAGE("hausbauer_t::gib_aus_liste()","appended %s at %i", besch->gib_name(), thislevel );
				auswahl.append(besch,besch->gib_chance(),4);
			}
		}
		// now look, what we have got ...
		if(auswahl.get_sum_weight()==0) {
			return NULL;
		}
		if(auswahl.get_count()==1) {
			return auswahl.front();
		}
		// now there is something to choose
		return auswahl.at_weight( simrand(auswahl.get_sum_weight()) );
	}
	return NULL;
}



const slist_tpl<const haus_besch_t*>* hausbauer_t::get_list(const haus_besch_t::utyp typ)
{
	switch (typ) {
		case haus_besch_t::denkmal:         return &ungebaute_denkmaeler;
		case haus_besch_t::attraction_land: return &sehenswuerdigkeiten_land;
		case haus_besch_t::firmensitz:      return NULL;
		case haus_besch_t::rathaus:         return &rathaeuser;
		case haus_besch_t::attraction_city: return &sehenswuerdigkeiten_city;
		case haus_besch_t::fabrik:          return NULL;
		default:                            return NULL;
	}
}



const vector_tpl<const haus_besch_t*>* hausbauer_t::get_citybuilding_list(const gebaeude_t::typ typ)
{
	switch (typ) {
		case gebaeude_t::wohnung:   return &wohnhaeuser;
		case gebaeude_t::gewerbe:   return &gewerbehaeuser;
		case gebaeude_t::industrie: return &industriehaeuser;
		default:                    return NULL;
	}
}
