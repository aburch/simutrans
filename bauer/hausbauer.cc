/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "../besch/haus_besch.h"
#include "../besch/skin_besch.h"
#include "../besch/spezial_obj_tpl.h"

#include "../boden/boden.h"
#include "../boden/wasser.h"
#include "../boden/fundament.h"

#include "../dataobj/scenario.h"
#include "../dings/leitung2.h"
#include "../dings/tunnel.h"
#include "../dings/zeiger.h"

#include "../gui/karte.h"
#include "../gui/werkzeug_waehler.h"

#include "../simdebug.h"
#include "../simdepot.h"
#include "../simhalt.h"
#include "../simtools.h"
#include "../simwerkz.h"
#include "../simworld.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/weighted_vector_tpl.h"
#include "hausbauer.h"

/*
 * Die verschiedenen Gebäudegruppen sind in eigenen Listen gesammelt.
 */
static vector_tpl<const haus_besch_t*> wohnhaeuser;
static vector_tpl<const haus_besch_t*> gewerbehaeuser;
static vector_tpl<const haus_besch_t*> industriehaeuser;
vector_tpl<const haus_besch_t *> hausbauer_t::sehenswuerdigkeiten_land;
vector_tpl<const haus_besch_t *> hausbauer_t::sehenswuerdigkeiten_city;
vector_tpl<const haus_besch_t *> hausbauer_t::rathaeuser;
vector_tpl<const haus_besch_t *> hausbauer_t::denkmaeler;
vector_tpl<const haus_besch_t *> hausbauer_t::ungebaute_denkmaeler;

/*
 * Diese Tabelle ermöglicht das Auffinden einer Beschreibung durch ihren Namen
 */
static stringhashtable_tpl<const haus_besch_t*> besch_names;

/*
 * Alle Gebäude, die die Anwendung direkt benötigt, kriegen feste IDs.
 * Außerdem müssen wir dafür sorgen, dass sie alle da sind.
 */
const haus_besch_t *hausbauer_t::elevated_foundation_besch = NULL;

// all buildings with rails or connected to stops
vector_tpl<const haus_besch_t *> hausbauer_t::station_building;

// all headquarter (sorted by hq-level)
vector_tpl<const haus_besch_t *> hausbauer_t::headquarter;

static spezial_obj_tpl<haus_besch_t> spezial_objekte[] = {
	{ &hausbauer_t::elevated_foundation_besch,   "MonorailGround" },
	{ NULL, NULL }
};


static bool compare_haus_besch(const haus_besch_t* a, const haus_besch_t* b)
{
	int diff = a->get_level() - b->get_level();
	if (diff == 0) {
		/* Gleiches Level - wir führen eine künstliche, aber eindeutige Sortierung
		 * über den Namen herbei. */
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}

static bool compare_hq_besch(const haus_besch_t* a, const haus_besch_t* b)
{
	// the headquarter level is in the extra-variable
	int diff = a->get_extra() - b->get_extra();
	if (diff == 0) {
		diff = a->get_level() - b->get_level();
	}
	if (diff == 0) {
		/* Gleiches Level - wir führen eine künstliche, aber eindeutige Sortierung
		 * über den Namen herbei. */
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}


static bool compare_station_besch(const haus_besch_t* a, const haus_besch_t* b)
{
	int diff = a->get_enabled() - b->get_enabled();
	if (diff == 0) {
		diff = a->get_level() - b->get_level();
	}
	if (diff == 0) {
		/* Gleiches Level - wir führen eine künstliche, aber eindeutige Sortierung
		 * über den Namen herbei. */
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}


bool hausbauer_t::alles_geladen()
{
	FOR(stringhashtable_tpl<haus_besch_t const*>, const& i, besch_names) {
		haus_besch_t const* const besch = i.value;
		switch(besch->get_typ()) {
			case gebaeude_t::wohnung:
				wohnhaeuser.insert_ordered(besch,compare_haus_besch);
				break;
			case gebaeude_t::industrie:
				industriehaeuser.insert_ordered(besch,compare_haus_besch);
				break;
			case gebaeude_t::gewerbe:
				gewerbehaeuser.insert_ordered(besch,compare_haus_besch);
				break;

			case gebaeude_t::unbekannt:
			switch (besch->get_utyp()) {
				case haus_besch_t::denkmal:
					denkmaeler.insert_ordered(besch,compare_haus_besch);
					break;
				case haus_besch_t::attraction_land:
					sehenswuerdigkeiten_land.insert_ordered(besch,compare_haus_besch);
					break;
				case haus_besch_t::firmensitz:
					headquarter.insert_ordered(besch,compare_hq_besch);
					break;
				case haus_besch_t::rathaus:
					rathaeuser.insert_ordered(besch,compare_haus_besch);
					break;
				case haus_besch_t::attraction_city:
					sehenswuerdigkeiten_city.insert_ordered(besch,compare_haus_besch);
					break;

				case haus_besch_t::fabrik:
					break;

				case haus_besch_t::hafen:
				case haus_besch_t::hafen_geb:
				case haus_besch_t::depot:
				case haus_besch_t::generic_stop:
				case haus_besch_t::generic_extension:
					station_building.insert_ordered(besch,compare_station_besch);
					break;

				case haus_besch_t::weitere:
					if(strcmp(besch->get_name(),"MonorailGround")==0) {
						// foundation for elevated ways
						elevated_foundation_besch = besch;
						break;
					}
				default:
					// obsolete object, usually such pak set will not load properly anyway (old objects should be catched before!)
					dbg->error("hausbauer_t::alles_geladen()","unknown subtype %i of \"%s\" ignored",besch->get_utyp(),besch->get_name());
			}
		}
	}

	// now sort them according level
	warne_ungeladene(spezial_objekte, 1);
	return true;
}


bool hausbauer_t::register_besch(haus_besch_t *besch)
{
	::register_besch(spezial_objekte, besch);

	// avoid duplicates with same name
	const haus_besch_t *old_besch = besch_names.get(besch->get_name());
	if(old_besch) {
		dbg->warning( "hausbauer_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
		besch_names.remove(besch->get_name());
		werkzeug_t::general_tool.remove( old_besch->get_builder() );
		delete old_besch->get_builder();
		delete old_besch;
	}
	// probably need a tools, if it has a cursor
	const skin_besch_t *sb = besch->get_cursor();
	if(  sb  &&  sb->get_bild_nr(1)!=IMG_LEER) {
		werkzeug_t *wkz;
		if(  besch->get_utyp()==haus_besch_t::depot  ) {
			wkz = new wkz_depot_t();
		}
		else if(  besch->get_utyp()==haus_besch_t::firmensitz  ) {
			wkz = new wkz_headquarter_t();
		}
		else {
			wkz = new wkz_station_t();
		}
		wkz->set_icon( besch->get_cursor()->get_bild_nr(1) );
		wkz->cursor = besch->get_cursor()->get_bild_nr(0),
		wkz->set_default_param(besch->get_name());
		werkzeug_t::general_tool.append( wkz );
		besch->set_builder( wkz );
	}
	else {
		besch->set_builder( NULL );
	}
	besch_names.put(besch->get_name(), besch);

	/* supply the tiles with a pointer back to the matchin description
	 * this is needed, since each building is build of seperate tiles,
	 * even if it is part of the same description (haus_besch_t)
	 */
	const int max_index = besch->get_all_layouts()*besch->get_groesse().x*besch->get_groesse().y;
	for( int i=0;  i<max_index;  i++  ) {
		const_cast<haus_tile_besch_t *>(besch->get_tile(i))->set_besch(besch);
	}

	return true;
}



// the tools must survice closing ...
static stringhashtable_tpl<wkz_station_t *> station_tool;
static stringhashtable_tpl<wkz_depot_t *> depot_tool;

// all these menus will need a waytype ...
void hausbauer_t::fill_menu(werkzeug_waehler_t* wzw, haus_besch_t::utyp utyp, waytype_t wt, sint16 /*sound_ok*/, const karte_t* welt)
{
	// check if scenario forbids this
	uint16 toolnr = 0;
	switch(utyp) {
		case haus_besch_t::depot:
			toolnr = WKZ_DEPOT | GENERAL_TOOL;
			break;
		case haus_besch_t::hafen:
		case haus_besch_t::generic_stop:
		case haus_besch_t::generic_extension:
			toolnr = WKZ_STATION | GENERAL_TOOL;
			break;
		default: ;
	}
	if (toolnr > 0  &&  !welt->get_scenario()->is_tool_allowed(welt->get_active_player(), toolnr, wt)) {
		return;
	}

	const uint16 time = welt->get_timeline_year_month();
DBG_DEBUG("hausbauer_t::fill_menu()","maximum %i",station_building.get_count());
	FOR(vector_tpl<haus_besch_t const*>, const besch, station_building) {
//		DBG_DEBUG("hausbauer_t::fill_menu()", "try to add %s (%p)", besch->get_name(), besch);
		if(  besch->get_utyp()==utyp  &&  besch->get_builder()  &&  (utyp==haus_besch_t::firmensitz  ||  besch->get_extra()==(uint16)wt)  ) {
			if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {
				wzw->add_werkzeug( besch->get_builder() );
			}
		}
	}
}


// new map => reset monument list to ensure every monument appears only once per map
void hausbauer_t::neue_karte()
{
	ungebaute_denkmaeler.clear();
	FOR(vector_tpl<haus_besch_t const*>, const i, denkmaeler) {
		ungebaute_denkmaeler.append(i);
	}
}



void hausbauer_t::remove( karte_t *welt, spieler_t *sp, gebaeude_t *gb )
{
	const haus_tile_besch_t *tile  = gb->get_tile();
	const haus_besch_t *hb = tile->get_besch();
	const uint8 layout = tile->get_layout();

	// get startpos and size
	const koord3d pos = gb->get_pos() - koord3d( tile->get_offset(), 0 );
	koord size = tile->get_besch()->get_groesse( layout );
	koord k;

	if(tile->get_besch()->get_utyp()==haus_besch_t::firmensitz) {
		gb->get_besitzer()->add_headquarter( tile->get_besch()->get_extra(), koord::invalid );
	}

	// then remove factory
	fabrik_t *fab = gb->get_fabrik();
	if(fab) {
		// first remove fabrik_t pointers
		for(k.y = 0; k.y < size.y; k.y ++) {
			for(k.x = 0; k.x < size.x; k.x ++) {
				grund_t *gr = welt->lookup(koord3d(k,0)+pos);
				// for buildings with holes the hole could be on a different height ->gr==NULL
				if (gr) {
					gebaeude_t *gb_part = gr->find<gebaeude_t>();
					if(gb_part) {
						// there may be buildings with holes, so we only remove our or the hole!
						if(gb_part->get_tile()  ==  hb->get_tile(layout, k.x, k.y)) {
							gb_part->set_fab( NULL );
							planquadrat_t *plan = welt->access( k+pos.get_2d() );
							for (size_t i = plan->get_haltlist_count(); i-- != 0;) {
								halthandle_t halt = plan->get_haltlist()[i];
								halt->remove_fabriken( fab );
								plan->remove_from_haltlist( welt, halt );
							}
						}
					}
				}
			}
		}
		// tell players of the deletion
		for(uint8 i=0; i<MAX_PLAYER_COUNT; i++) {
			spieler_t *sp = welt->get_spieler(i);
			if (sp) {
				sp->notify_factory(spieler_t::notify_delete, fab);
			}
		}
		// remove all transformers
		for(k.y = -1; k.y < size.y+1;  k.y ++) {
			for(k.x = -1; k.x < size.x+1;  k.x ++) {
				grund_t *gr = NULL;
				if (0<=k.x  &&  k.x<size.x  &&  0<=k.y  &&  k.y<size.y) {
					// look below factory
					gr = welt->lookup(koord3d(k,-1) + pos);
				}
				else {
					// find transformers near factory
					gr = welt->lookup_kartenboden(k + pos.get_2d());
				}
				if (gr) {
					senke_t *sk = gr->find<senke_t>();
					if (sk  &&  sk->get_factory() == fab) {
						sk->mark_image_dirty(sk->get_bild(), 0);
						delete sk;
					}
					pumpe_t* pp = gr->find<pumpe_t>();
					if (pp  &&  pp->get_factory() == fab) {
						pp->mark_image_dirty(pp->get_bild(), 0);
						delete pp;
					}
					// remove tunnel
					if( (sk!=NULL ||  pp!=NULL)  &&  gr->ist_im_tunnel()  &&  gr->get_top()<=1  ) {
						if (tunnel_t *t = gr->find<tunnel_t>()) {
							t->entferne( t->get_besitzer() );
							delete t;
						}
						const koord p = gr->get_pos().get_2d();
						welt->lookup_kartenboden(p)->clear_flag(grund_t::marked);
						// remove ground
						welt->access(p)->boden_entfernen(gr);
						delete gr;
					}
				}
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
				if(gb_part  &&  gb_part->get_tile() == hb->get_tile(layout, k.x, k.y)) {
					// ok, now we can go on with deletion
					gb_part->entferne( sp );
					delete gb_part;
					// if this was a station building: delete ground
					if(gr->get_halt().is_bound()) {
						haltestelle_t::remove(welt, sp, gr->get_pos());
					}
					// and maybe restore land below
					if(gr->get_typ()==grund_t::fundament) {
						const koord newk = k+pos.get_2d();
						sint8 new_hgt;
						const uint8 new_slope = welt->recalc_natural_slope(newk,new_hgt);
						// test for ground at new height
						const grund_t *gr2 = welt->lookup(koord3d(newk,new_hgt));
						if((gr2 == NULL  ||  gr2 == gr) &&  new_slope!=hang_t::flach) {
							// and for ground above new sloped tile
							gr2 = welt->lookup(koord3d(newk, new_hgt+1));
						}
						bool ground_recalc = true;
						if(gr2  &&  gr2!=gr) {
							// there is another ground below or above
							// => do not change height, keep foundation
							welt->access(newk)->kartenboden_setzen( new boden_t(welt, gr->get_pos(), hang_t::flach ) );
							ground_recalc = false;
						}
						else if(  new_hgt<=welt->get_grundwasser()  &&  new_slope==hang_t::flach  ) {
							welt->access(newk)->kartenboden_setzen(new wasser_t(welt, koord3d(newk,new_hgt) ) );
						}
						else {
							if(  gr->get_grund_hang()==new_slope  ) {
								ground_recalc = false;
							}
							welt->access(newk)->kartenboden_setzen(new boden_t(welt, koord3d(newk,new_hgt), new_slope) );
						}
						// there might be walls from foundations left => thus some tiles may needs to be redraw
						if(ground_recalc) {
							if(grund_t *gr = welt->lookup_kartenboden(newk+koord::ost)) {
								gr->calc_bild();
							}
							if(grund_t *gr = welt->lookup_kartenboden(newk+koord::sued)) {
								gr->calc_bild();
							}
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
	dim = besch->get_groesse(org_layout);
	bool needs_ground_recalc = false;

	for(k.y = 0; k.y < dim.y; k.y ++) {
		for(k.x = 0; k.x < dim.x; k.x ++) {
//DBG_DEBUG("hausbauer_t::baue()","get_tile() at %i,%i",k.x,k.y);
			const haus_tile_besch_t *tile = besch->get_tile(layout, k.x, k.y);
			// here test for good tile
			if (tile == NULL || (
						besch->get_utyp() != haus_besch_t::hafen &&
						tile->get_hintergrund(0, 0, 0) == IMG_LEER &&
						tile->get_vordergrund(0, 0)    == IMG_LEER
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
				gb->set_fab((fabrik_t *)param);
			}
			// try to fake old building
			else if(welt->get_zeit_ms() < 2) {
				// Hajo: after staring a new map, build fake old buildings
				gb->add_alter(10000);
			}
			grund_t *gr = welt->lookup_kartenboden(pos.get_2d() + k);
			if(gr->ist_wasser()) {
				gr->obj_add(gb);
			} else if (besch->get_utyp() == haus_besch_t::hafen) {
				// its a dock!
				gr->obj_add(gb);
			}
			else {
				// very likely remove all
				leitung_t *lt = NULL;
				if(!gr->hat_wege()) {
					lt = gr->find<leitung_t>();
					if(lt) {
						gr->obj_remove(lt);
					}
					gr->obj_loesche_alle(sp);	// alles weg außer vehikel ...
				}
				needs_ground_recalc |= gr->get_grund_hang()!=hang_t::flach;
				grund_t *gr2 = new fundament_t(welt, gr->get_pos(), gr->get_grund_hang());
				welt->access(gr->get_pos().get_2d())->boden_ersetzen(gr, gr2);
				gr = gr2;
//DBG_DEBUG("hausbauer_t::baue()","ground count now %i",gr->obj_count());
				gr->obj_add( gb );
				if(lt) {
					gr->obj_add( lt );
				}
				if(needs_ground_recalc  &&  welt->is_within_limits(pos.get_2d()+k+koord(1,1))  &&  (k.y+1==dim.y  ||  k.x+1==dim.x)) {
					welt->lookup_kartenboden(pos.get_2d()+k+koord(1,0))->calc_bild();
					welt->lookup_kartenboden(pos.get_2d()+k+koord(0,1))->calc_bild();
					welt->lookup_kartenboden(pos.get_2d()+k+koord(1,1))->calc_bild();
				}
			}
			gb->set_pos( gr->get_pos() );
			if(besch->ist_ausflugsziel()) {
				welt->add_ausflugsziel( gb );
			}
			if(besch->get_typ() == gebaeude_t::unbekannt) {
				if(station_building.is_contained(besch)) {
					(*static_cast<halthandle_t *>(param))->add_grund(gr);
				}
				if (besch->get_utyp() == haus_besch_t::hafen) {
					// its a dock!
					gb->set_yoff(0);
				}
			}
			gr->calc_bild();
			reliefkarte_t::get_karte()->calc_map_pixel(gr->get_pos().get_2d());
		}
	}
	// remove only once ...
	if(besch->get_utyp()==haus_besch_t::denkmal) {
		ungebaute_denkmaeler.remove( besch );
	}
	return first_building;
}


gebaeude_t *hausbauer_t::neues_gebaeude(karte_t *welt, spieler_t *sp, koord3d pos, int built_layout, const haus_besch_t *besch, void *param)
{
	uint8 corner_layout = 6;	// assume single building (for more than 4 layouts)

	// adjust layout of neighbouring building
	if(besch->get_utyp()>=8  &&  besch->get_all_layouts()>1) {

		int layout = built_layout & 9;

		// detect if we are connected at far (north/west) end
		sint8 offset = welt->lookup( pos )->get_weg_yoff()/TILE_HEIGHT_STEP;
		koord3d checkpos = pos+koord3d( (layout & 1 ? koord::ost : koord::sued), offset);
		grund_t * gr = welt->lookup( checkpos );
		if(!gr) {
			// check whether bridge end tile
			grund_t * gr_tmp = welt->lookup( pos+koord3d( (layout & 1 ? koord::ost : koord::sued),offset - 1) );
			if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 1) {
				gr = gr_tmp;
			}
		}
		if(gr) {
			gebaeude_t* gb = gr->find<gebaeude_t>();
			if(gb==NULL  &&  welt->lookup(checkpos.get_2d())->get_halt().is_bound()) {
				// no building on same level, but halt nearby => we will this this
				const planquadrat_t *pl = welt->lookup(checkpos.get_2d());
				for(  uint8 i=0;  i<pl->get_boden_count();  i++  ) {
					gr = pl->get_boden_bei(i);
					if(gr->is_halt()) {
						break;
					}
				}
				gb = gr->find<gebaeude_t>();
			}
			if(gb  &&  gb->get_tile()->get_besch()->get_utyp()>=8) {
				corner_layout &= ~2; // clear near bit
				if(gb->get_tile()->get_besch()->get_all_layouts()>4) {
					koord xy = gb->get_tile()->get_offset();
					uint8 layoutbase = gb->get_tile()->get_layout();
					if((layoutbase & 1) == (layout & 1)) {
						layoutbase &= 0xb; // clear near bit on neighbour
						gb->set_tile( gb->get_tile()->get_besch()->get_tile(layoutbase, xy.x, xy.y), false );
					}
				}
			}
		}

		// detect if near (south/east) end
		gr = welt->lookup( pos+koord3d( (layout & 1 ? koord::west : koord::nord), offset) );
		if(!gr) {
			// check whether bridge end tile
			grund_t * gr_tmp = welt->lookup( pos+koord3d( (layout & 1 ? koord::west : koord::nord),offset - 1) );
			if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 1) {
				gr = gr_tmp;
			}
		}
		if(gr) {
			gebaeude_t* gb = gr->find<gebaeude_t>();
			if(gb  &&  gb->get_tile()->get_besch()->get_utyp()>=8) {
				corner_layout &= ~4; // clear far bit
				if(gb->get_tile()->get_besch()->get_all_layouts()>4) {
					koord xy = gb->get_tile()->get_offset();
					uint8 layoutbase = gb->get_tile()->get_layout();
					if((layoutbase & 1) == (layout & 1)) {
						layoutbase &= 0xd; // clear far bit on neighbour
						gb->set_tile( gb->get_tile()->get_besch()->get_tile(layoutbase, xy.x, xy.y), false );
					}
				}
			}
		}
	}

	// adjust layouts of the new building
	if(besch->get_all_layouts()>4) {
		built_layout = (corner_layout | (built_layout&9) ) % besch->get_all_layouts();
	}

	const haus_tile_besch_t *tile = besch->get_tile(built_layout, 0, 0);
	gebaeude_t *gb;
	if(  besch->get_utyp() == haus_besch_t::depot  ) {
		switch(  besch->get_extra()  ) {
			case track_wt:
				gb = new bahndepot_t(welt, pos, sp, tile);
				break;
			case tram_wt:
				gb = new tramdepot_t(welt, pos, sp, tile);
				break;
			case monorail_wt:
				gb = new monoraildepot_t(welt, pos, sp, tile);
				break;
			case maglev_wt:
				gb = new maglevdepot_t(welt, pos, sp, tile);
				break;
			case narrowgauge_wt:
				gb = new narrowgaugedepot_t(welt, pos, sp, tile);
				break;
			case road_wt:
				gb = new strassendepot_t(welt, pos, sp, tile);
				break;
			case water_wt:
				gb = new schiffdepot_t(welt, pos, sp, tile);
				break;
			case air_wt:
				gb = new airdepot_t(welt, pos, sp, tile);
				break;
			default:
				dbg->fatal("hausbauer_t::neues_gebaeude()","waytpe %i has no depots!", besch->get_extra() );
				break;
		}
	}
	else {
		gb = new gebaeude_t(welt, pos, sp, tile);
	}
//DBG_MESSAGE("hausbauer_t::neues_gebaeude()","building stop pri=%i",pri);

	// remove pointer
	grund_t *gr = welt->lookup(pos);
	zeiger_t* zeiger = gr->find<zeiger_t>();
	if(  zeiger  ) {
		gr->obj_remove(zeiger);
		zeiger->set_flag(ding_t::not_on_map);
	}

	gr->obj_add(gb);

	if(  station_building.is_contained(besch)  &&  besch->get_utyp()!=haus_besch_t::depot  ) {
		// is a station/bus stop
		(*static_cast<halthandle_t *>(param))->add_grund(gr);
		gr->calc_bild();
	}
	else {
		gb->calc_bild();
	}

	if(besch->ist_ausflugsziel()) {
		welt->add_ausflugsziel( gb );
	}
	reliefkarte_t::get_karte()->calc_map_pixel(gb->get_pos().get_2d());

	return gb;
}



const haus_tile_besch_t *hausbauer_t::find_tile(const char *name, int org_idx)
{
	int idx = org_idx;
	const haus_besch_t *besch = besch_names.get(name);
	if(besch) {
		const int size = besch->get_h()*besch->get_b();
		if(  idx >= besch->get_all_layouts()*size  ) {
			idx %= besch->get_all_layouts()*size;
			DBG_MESSAGE("gebaeude_t::rdwr()","%s using tile %i instead of %i",name,idx,org_idx);
		}
		return besch->get_tile(idx);
	}
//	DBG_MESSAGE("hausbauer_t::find_tile()","\"%s\" not in hashtable",name);
	return NULL;
}



const haus_besch_t* hausbauer_t::get_random_station(const haus_besch_t::utyp utype, const waytype_t wt, const uint16 time, const uint8 enables)
{
	weighted_vector_tpl<const haus_besch_t*> stops;

	FOR(vector_tpl<haus_besch_t const*>, const besch, station_building) {
		if(besch->get_utyp()==utype  &&  besch->get_extra()==wt  &&  (enables==0  ||  (besch->get_enabled()&enables)!=0)) {
			if( !besch->can_be_built_aboveground()) {
				continue;
			}
			// ok, now check timeline
			if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {
				stops.append(besch,max(1,16-besch->get_level()*besch->get_b()*besch->get_h()));
			}
		}
	}
	return stops.empty() ? 0 : pick_any_weighted(stops);
}



const haus_besch_t* hausbauer_t::get_special(uint32 bev, haus_besch_t::utyp utype, uint16 time, bool ignore_retire, climate cl)
{
	weighted_vector_tpl<const haus_besch_t *> auswahl(16);

	vector_tpl<const haus_besch_t*> *list = NULL;
	switch(utype) {
		case haus_besch_t::rathaus:
			list = &rathaeuser;
			break;
		case haus_besch_t::attraction_city:
			list = &sehenswuerdigkeiten_city;
			break;
		default:
			return NULL;
	}
	FOR(vector_tpl<haus_besch_t const*>, const besch, *list) {
		// extra data contains number of inhabitants for building
		if(besch->get_extra()==bev) {
			if(cl==MAX_CLIMATES  ||  besch->is_allowed_climate(cl)) {
				// ok, now check timeline
				if(time==0  ||  (besch->get_intro_year_month()<=time  &&  (ignore_retire  ||  besch->get_retire_year_month() > time)  )  ) {
					auswahl.append(besch, besch->get_chance());
				}
			}
		}
	}
	if (auswahl.empty()) {
		return 0;
	}
	else if(auswahl.get_count()==1) {
		return auswahl.front();
	}
	// now there is something to choose
	return pick_any_weighted(auswahl);
}



/**
 * tries to find something that matches this entry
 * it will skip and jump, and will never return zero, if there is at least a single valid entry in the list
 * @author Hj. Malthaner
 */
static const haus_besch_t* get_aus_liste(const vector_tpl<const haus_besch_t*>& liste, int level, uint16 time, climate cl)
{
	weighted_vector_tpl<const haus_besch_t *> auswahl(16);

//	DBG_MESSAGE("hausbauer_t::get_aus_liste()","target level %i", level );
	const haus_besch_t *besch_at_least=NULL;
	FOR(vector_tpl<haus_besch_t const*>, const besch, liste) {
		if(	besch->is_allowed_climate(cl)  &&
			besch->get_chance()>0  &&
			(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time))) {
			besch_at_least = besch;
		}

		const int thislevel = besch->get_level();
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

		if(thislevel==level  &&  besch->get_chance()>0) {
			if(cl==MAX_CLIMATES  ||  besch->is_allowed_climate(cl)) {
				if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {
//				DBG_MESSAGE("hausbauer_t::get_aus_liste()","appended %s at %i", besch->get_name(), thislevel );
					auswahl.append(besch, besch->get_chance());
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
	return pick_any_weighted(auswahl);
}


const haus_besch_t* hausbauer_t::get_gewerbe(int level, uint16 time, climate cl)
{
	return get_aus_liste(gewerbehaeuser, level, time, cl);
}


const haus_besch_t* hausbauer_t::get_industrie(int level, uint16 time, climate cl)
{
	return get_aus_liste(industriehaeuser, level, time, cl);
}


const haus_besch_t* hausbauer_t::get_wohnhaus(int level, uint16 time, climate cl)
{
	return get_aus_liste(wohnhaeuser, level, time, cl);
}

const haus_besch_t* hausbauer_t::get_headquarter(int level, uint16 time)
{
	FOR(vector_tpl<haus_besch_t const*>, const besch, hausbauer_t::headquarter) {
		if (besch->get_extra() == level  &&  !besch->is_future(time)  &&  !besch->is_retired(time)) {
			return besch;
		}
	}
	return NULL;
}


// get a random object
const haus_besch_t *hausbauer_t::waehle_aus_liste(vector_tpl<const haus_besch_t *> &liste, uint16 time, bool ignore_retire, climate cl)
{
	if (!liste.empty()) {
		// previously just returned a random object; however, now we do als look at the chance entry
		weighted_vector_tpl<const haus_besch_t *> auswahl(16);
		FOR(vector_tpl<haus_besch_t const*>, const besch, liste) {
			if((cl==MAX_CLIMATES  ||  besch->is_allowed_climate(cl))  &&  besch->get_chance()>0  &&  (time==0  ||  (besch->get_intro_year_month()<=time  &&  (ignore_retire  ||  besch->get_retire_year_month()>time)  )  )  ) {
//				DBG_MESSAGE("hausbauer_t::get_aus_liste()","appended %s at %i", besch->get_name(), thislevel );
				auswahl.append(besch, besch->get_chance());
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
		return pick_any_weighted(auswahl);
	}
	return NULL;
}



const vector_tpl<const haus_besch_t*>* hausbauer_t::get_list(const haus_besch_t::utyp typ)
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
