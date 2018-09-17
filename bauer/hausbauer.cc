/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "../descriptor/building_desc.h"
#include "../descriptor/skin_desc.h"
#include "../descriptor/spezial_obj_tpl.h"

#include "../boden/boden.h"
#include "../boden/wasser.h"
#include "../boden/fundament.h"

#include "../dataobj/scenario.h"
#include "../obj/leitung2.h"
#include "../obj/tunnel.h"
#include "../obj/zeiger.h"

#include "../gui/karte.h"
#include "../simworld.h"
#include "../gui/tool_selector.h"

#include "../simcity.h"
#include "../simdebug.h"
#include "../simdepot.h"
#include "../simsignalbox.h"
#include "../simhalt.h"
#include "../utils/simrandom.h"
#include "../utils/cbuffer_t.h"
#include "../simtool.h"
#include "../simworld.h"
#include "../simmesg.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/weighted_vector_tpl.h"
#include "../tpl/vector_tpl.h"
#include "hausbauer.h"

karte_ptr_t hausbauer_t::welt;

/*
 * The different building groups are sorted into separate lists
 */
static vector_tpl<const building_desc_t*> city_residential;
static vector_tpl<const building_desc_t*> city_commercial;
static vector_tpl<const building_desc_t*> city_industry;
vector_tpl<const building_desc_t *> hausbauer_t::attractions_land;
vector_tpl<const building_desc_t *> hausbauer_t::attractions_city;
vector_tpl<const building_desc_t *> hausbauer_t::townhalls;
vector_tpl<const building_desc_t *> hausbauer_t::monuments;
vector_tpl<const building_desc_t *> hausbauer_t::unbuilt_monuments;

/*
 * List of all registered house descriptors.
 * Allows searching for a desc by its name
 */
static stringhashtable_tpl<const building_desc_t*> desc_names;

const building_desc_t *hausbauer_t::elevated_foundation_desc = NULL;

// all buildings with rails or connected to stops
vector_tpl<const building_desc_t *> hausbauer_t::station_building;
vector_tpl<building_desc_t*> hausbauer_t::modifiable_station_buildings;

// all headquarters (sorted by hq-level)
vector_tpl<const building_desc_t *> hausbauer_t::headquarters;

/// special objects directly needed by the program
static spezial_obj_tpl<building_desc_t> const special_objects[] = {
	{ &hausbauer_t::elevated_foundation_desc,   "MonorailGround" },
	{ NULL, NULL }
};


/**
 * Compares house descriptors.
 * Order of comparison:
 *  -# by level
 *  -# by name
 * @return true if @p a < @p b
 */
static bool compare_building_desc(const building_desc_t* a, const building_desc_t* b)
{
	int diff = a->get_level() - b->get_level();
	if (diff == 0) {
		/* As a last resort, sort by name to avoid ambiguity */
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}


/**
 * Compares headquarters.
 * Order of comparison:
 *  -# by level
 *  -# by name
 * @return true if @p a < @p b
 */
static bool compare_hq_desc(const building_desc_t* a, const building_desc_t* b)
{
	// the headquarters level is in the extra-variable
	int diff = a->get_extra() - b->get_extra();
	if (diff == 0) {
		diff = a->get_level() - b->get_level();
	}
	if (diff == 0) {
		/* As a last resort, sort by name to avoid ambiguity */
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}


/**
 * Compares stations.
 * Order of comparison:
 *  -# by good category
 *  -# by capacity
 *  -# by level
 *  -# by name
 * @return true if @p a < @p b
 */
static bool compare_station_desc(const building_desc_t* a, const building_desc_t* b)
{
	int diff = a->get_enabled() - b->get_enabled();
	if(  diff == 0  ) {
		diff = a->get_capacity() - b->get_capacity();
	}
	if(  diff == 0  ) {
		diff = a->get_level() - b->get_level();
	}
	if(  diff == 0  ) {
		/* As a last resort, sort by name to avoid ambiguity */
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}


bool hausbauer_t::successfully_loaded()
{
	FOR(stringhashtable_tpl<building_desc_t const*>, const& i, desc_names) {
		building_desc_t const* const desc = i.value;

		// now insert the besch into the correct list.
		switch (desc->get_type()) {
			case building_desc_t::city_res:
				city_residential.insert_ordered(desc,compare_building_desc);
				break;
			case building_desc_t::city_ind:
				city_industry.insert_ordered(desc,compare_building_desc);
				break;
			case building_desc_t::city_com:
				city_commercial.insert_ordered(desc,compare_building_desc);
				break;

			case building_desc_t::monument:
				monuments.insert_ordered(desc,compare_building_desc);
				break;
			case building_desc_t::attraction_land:
				attractions_land.insert_ordered(desc,compare_building_desc);
				break;
			case building_desc_t::headquarters:
				headquarters.insert_ordered(desc,compare_hq_desc);
				break;
			case building_desc_t::townhall:
				townhalls.insert_ordered(desc,compare_building_desc);
				break;
			case building_desc_t::attraction_city:
				attractions_city.insert_ordered(desc,compare_building_desc);
				break;

			case building_desc_t::factory:
				break;

			case building_desc_t::signalbox:
			case building_desc_t::dock:
			case building_desc_t::flat_dock:
			case building_desc_t::depot:
			case building_desc_t::generic_stop:
			case building_desc_t::generic_extension:

				station_building.insert_ordered(desc,compare_station_desc);
				break;

			case building_desc_t::others:
				if(strcmp(desc->get_name(),"MonorailGround")==0) {
					// foundation for elevated ways
					elevated_foundation_desc = desc;
					break;
				}
				/* no break */

			default:
				// obsolete object, usually such pak set will not load properly anyway (old objects should be caught before!)
				dbg->error("hausbauer_t::successfully_loaded()","unknown subtype %i of \"%s\" ignored",desc->get_type(),desc->get_name());
		}

		// Casting away the const is nasty:
		((building_desc_t*)desc)->fix_number_of_classes();
	}

	// now sort them according level
	warn_missing_objects(special_objects);
	return true;
}


bool hausbauer_t::register_desc(building_desc_t *desc)
{
	// We might still need to fix the number of classes, so the const will
	// be casted away (temporarily) in succesfully_loaded()
	const building_desc_t* const_desc = desc;

	::register_desc(special_objects, const_desc);

	// avoid duplicates with same name
	const building_desc_t *old_desc = desc_names.get(desc->get_name());
	if(old_desc) {
		// do not overlay existing factories if the new one is not a factory
		if (old_desc->is_factory()  &&  !desc->is_factory()) {
			dbg->warning( "hausbauer_t::register_desc()", "Object %s could not be registered since it would overlay an existing factory building!", desc->get_name() );
			delete desc;
			return false;
		}
		dbg->warning( "hausbauer_t::register_desc()", "Object %s was overlaid by addon!", desc->get_name() );
		desc_names.remove(desc->get_name());
		tool_t::general_tool.remove( old_desc->get_builder() );
		delete old_desc->get_builder();
		delete old_desc;
	}
	// probably needs a tool if it has a cursor
	const skin_desc_t *sd = desc->get_cursor();
	if(  sd  &&  sd->get_image_id(1)!=IMG_EMPTY) {
		tool_t *tool;
		if(  desc->get_type()==building_desc_t::depot  ) {
			tool = new tool_depot_t();
		}
		else if(  desc->get_type()==building_desc_t::headquarters  ) {
			tool = new tool_headquarter_t();
		}
		else if(desc->is_signalbox())
		{
			tool = new tool_signalbox_t();
			modifiable_station_buildings.append(desc);
		}
		else {
			tool = new tool_build_station_t();
			modifiable_station_buildings.append(desc);
		}
		tool->set_icon( desc->get_cursor()->get_image_id(1) );
		tool->cursor = desc->get_cursor()->get_image_id(0),
		tool->set_default_param(desc->get_name());
		tool_t::general_tool.append( tool );
		desc->set_builder( tool );
	}
	else {
		desc->set_builder( NULL );
	}
	desc_names.put(desc->get_name(), const_desc);

	/* Supply the tiles with a pointer back to the matching description.
	 * This is necessary since each building consists of separate tiles,
	 * even if it is part of the same description (building_desc_t)
	 */
	const int max_index = const_desc->get_all_layouts() * const_desc->get_size().x * const_desc->get_size().y;
	for( int i=0;  i<max_index;  i++  )
	{
		const_cast<building_tile_desc_t *>(desc->get_tile(i))->set_desc(const_desc);
		const_cast<building_tile_desc_t *>(desc->get_tile(i))->set_modifiable_desc(desc);
	}

	return true;
}


void hausbauer_t::fill_menu(tool_selector_t* tool_selector, building_desc_t::btype btype, waytype_t wt, sint16 /*sound_ok*/)
{
	// check if scenario forbids this
	uint16 toolnr = 0;
	switch(btype) {
		case building_desc_t::depot:
			toolnr = TOOL_BUILD_DEPOT | GENERAL_TOOL;
			break;
		case building_desc_t::dock:
		case building_desc_t::flat_dock:
		case building_desc_t::generic_stop:
		case building_desc_t::generic_extension:
			toolnr = TOOL_BUILD_STATION | GENERAL_TOOL;
			break;
		case building_desc_t::signalbox:
			toolnr = TOOL_BUILD_SIGNALBOX | GENERAL_TOOL;
			break;
		default:
			break;
	}
	if(  toolnr > 0  &&  !welt->get_scenario()->is_tool_allowed(welt->get_active_player(), toolnr, wt)  ) {
		return;
	}

	const uint16 time = welt->get_timeline_year_month();
	DBG_DEBUG("hausbauer_t::fill_menu()","maximum %i",station_building.get_count());
	FOR(  vector_tpl<building_desc_t const*>,  const desc,  station_building  ) {
//		DBG_DEBUG("hausbauer_t::fill_menu()", "try to add %s (%p)", desc->get_name(), desc);
		if(  desc->get_type() == btype  &&  desc->get_builder()  &&  ((btype == building_desc_t::headquarters || btype == building_desc_t::signalbox) ||  desc->get_extra()==(uint16)wt)  ) {
			if(desc->is_available(time) &&
				((desc->get_allow_underground() >= 2) ||
				(desc->get_allow_underground() == 1 && (grund_t::underground_mode == grund_t::ugm_all || grund_t::underground_mode == grund_t::ugm_level)) ||
				(desc->get_allow_underground() == 0 && grund_t::underground_mode != grund_t::ugm_all)))
			{
				tool_selector->add_tool_selector( desc->get_builder() );
			}
		}
	}
}


void hausbauer_t::new_world()
{
	unbuilt_monuments.clear();
	FOR(vector_tpl<building_desc_t const*>, const i, monuments) {
		unbuilt_monuments.append(i);
	}
}


void hausbauer_t::remove( player_t *player, const gebaeude_t *gb, bool map_generation ) //gebaeude = "building" (Babelfish)
{
	const building_tile_desc_t *tile  = gb->get_tile();
	const building_desc_t *bdsc = tile->get_desc();
	const uint8 layout = tile->get_layout();
	stadt_t* const city = gb->get_stadt();

	// get start position and size
	const koord3d pos = gb->get_pos() - koord3d( tile->get_offset(), 0 );
	koord size = tile->get_desc()->get_size( layout );
	koord k;

	if( tile->get_desc()->get_type() == building_desc_t::headquarters ) {
		gb->get_owner()->add_headquarter( 0, koord::invalid );
	}
	if(tile->get_desc()->get_type()==building_desc_t::monument) {
		unbuilt_monuments.append_unique(tile->get_desc());
	}

	// then remove factory
	fabrik_t *fab = gb->get_fabrik();
	if(fab) {
		// first remove fabrik_t pointers
		for(k.y = 0; k.y < size.y; k.y ++) {
			for(k.x = 0; k.x < size.x; k.x ++) {
				const grund_t *gr = welt->lookup(koord3d(k,0)+pos);
				assert(gr);

				// for buildings with holes the hole could be on a different height ->gr==NULL
				if (gr) {
					gebaeude_t *gb_part = gr->find<gebaeude_t>();
					if(gb_part) {
						// there may be buildings with holes, so we only remove our building!
						if(gb_part->get_tile()  ==  bdsc->get_tile(layout, k.x, k.y)) {
							gb_part->set_fab( NULL );
							planquadrat_t *plan = welt->access( k+pos.get_2d() );
							// Remove factory from the halt's list of factories
							for (size_t i = plan->get_haltlist_count(); i-- != 0;) {
								nearby_halt_t nearby_halt = plan->get_haltlist()[i];
								nearby_halt.halt->remove_fabriken(fab);
							}
						}
					}
				}
			}
		}
		// tell players of the deletion
		for(uint8 i=0; i<MAX_PLAYER_COUNT; i++) {
			player_t *player = welt->get_player(i);
			if (player) {
				player->notify_factory(player_t::notify_delete, fab);
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
					if (  sk  &&  sk->get_factory()==fab  ) {
						sk->mark_image_dirty(sk->get_image(), 0);
						delete sk;
					}
					pumpe_t* pp = gr->find<pumpe_t>();
					if (  pp  &&  pp->get_factory()==fab  ) {
						pp->mark_image_dirty(pp->get_image(), 0);
						delete pp;
					}
					// remove tunnel
					if(  (sk!=NULL ||  pp!=NULL)  &&  gr->ist_im_tunnel()  &&  gr->get_top()<=1  ) {
						if (tunnel_t *t = gr->find<tunnel_t>()) {
							t->cleanup( t->get_owner() );
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
		// cleared transformers successfully, now remove factory.
		welt->rem_fab(fab);
	}


	for(k.y = 0; k.y < size.y; k.y ++) {
		for(k.x = 0; k.x < size.x; k.x ++) {
			grund_t *gr = welt->lookup(koord3d(k,0)+pos);
			if(gr) {
				gebaeude_t *gb_part = gr->find<gebaeude_t>();
				// there may be buildings with holes, so we only remove our building!
				if(gb_part  &&  gb_part->get_tile() == bdsc->get_tile(layout, k.x, k.y)) {
					gb_part->check_road_tiles(true);
				}
			}
		}
	}
	// delete our house only
	for(k.y = 0; k.y < size.y; k.y ++) {
		for(k.x = 0; k.x < size.x; k.x ++) {
			grund_t *gr = welt->lookup(koord3d(k,0)+pos);
			if(gr) {
				gebaeude_t *gb_part = gr->find<gebaeude_t>();
				if(!gb_part)
				{
					// May be a signalbox
					gb_part = (gebaeude_t*)gr->get_signalbox();
				}
				// there may be buildings with holes, so we only remove our building!
				if(  gb_part  &&  gb_part->get_tile()==bdsc->get_tile(layout, k.x, k.y)  ) {
					// ok, now we can go on with deletion
					if (gb_part->get_tile()->get_desc()->get_type() == building_desc_t::townhall)
					{
						gb_part->set_stadt(city);
					}
					gb_part->cleanup( player );
					if (city)
					{
						city->remove_gebaeude_from_stadt(gb_part, map_generation, k==koord(0,0));
						gb_part->set_stadt(NULL);
					}
					delete gb_part;
					// if this was a station building: delete ground
					if(gr->get_halt().is_bound()) {
						haltestelle_t::remove(player, gr->get_pos());
					}
					// and maybe restore land below
					if(gr->get_typ()==grund_t::fundament) {
						const koord newk = k+pos.get_2d();
						sint8 new_hgt;
						const uint8 new_slope = welt->recalc_natural_slope(newk,new_hgt);
						// test for ground at new height
						const grund_t *gr2 = welt->lookup(koord3d(newk,new_hgt));
						if(  (gr2==NULL  ||  gr2==gr) &&  new_slope!=slope_t::flat  ) {
							// and for ground above new sloped tile
							gr2 = welt->lookup(koord3d(newk, new_hgt+1));
						}
						bool ground_recalc = true;
						if(  gr2  &&  gr2!=gr  ) {
							// there is another ground below or above
							// => do not change height, keep foundation
							welt->access(newk)->kartenboden_setzen( new boden_t( gr->get_pos(), slope_t::flat ) );
							ground_recalc = false;
						}
						else if(  new_hgt <= welt->get_water_hgt(newk)  &&  new_slope == slope_t::flat  ) {
							welt->access(newk)->kartenboden_setzen( new wasser_t( koord3d( newk, new_hgt ) ) );
							welt->calc_climate( newk, true );
						}
						else {
							if(  gr->get_grund_hang() == new_slope  ) {
								ground_recalc = false;
							}
							welt->access(newk)->kartenboden_setzen( new boden_t( koord3d( newk, new_hgt ), new_slope ) );
							// climate is stored in planquadrat, and hence automatically preserved
						}
						// there might be walls from foundations left => thus some tiles may need to be redrawn
						if(ground_recalc) {
							if(grund_t *gr = welt->lookup_kartenboden(newk+koord::east)) {
								gr->calc_image();
							}
							if(grund_t *gr = welt->lookup_kartenboden(newk+koord::south)) {
								gr->calc_image();
							}
						}
					}
				}
			}
		}
	}
}


gebaeude_t* hausbauer_t::build(player_t* player, koord3d pos, int org_layout, const building_desc_t* desc, void* param)
{
	gebaeude_t* first_building = NULL;
	koord k;
	koord dim;

	uint8 layout = desc->adjust_layout(org_layout);
	dim = desc->get_size(org_layout);
	bool needs_ground_recalc = false;

	for(k.y = 0; k.y < dim.y; k.y ++) {
		for(k.x = 0; k.x < dim.x; k.x ++) {
//DBG_DEBUG("hausbauer_t::build()","get_tile() at %i,%i",k.x,k.y);
			const building_tile_desc_t *tile = desc->get_tile(layout, k.x, k.y);

			// skip empty tiles
			if (tile == NULL || (
						!(desc->get_type() == building_desc_t::dock  ||  desc->get_type() == building_desc_t::flat_dock)  &&
						tile->get_background(0, 0, 0) == IMG_EMPTY &&
						tile->get_foreground(0, 0)    == IMG_EMPTY
					)) {
						// may have a rotation that is not recoverable
						DBG_MESSAGE("hausbauer_t::build()","get_tile() empty at %i,%i",k.x,k.y);
				continue;
			}

			grund_t *gr = NULL;
			if(desc->get_allow_underground() && desc->is_signalbox())
			{
				// Note that this works properly only for signalboxes, as the underground tile needs a grund_t object,
				// which has to be added in the specific tool building this.
				// TODO: Consider making this work with station extension buildings
				gr = welt->lookup(pos);
			}

			if(!gr)
			{
				gr = welt->lookup_kartenboden(pos.get_2d() + k);
			}
			// mostly remove everything
			vector_tpl<obj_t *> keptobjs;
			if(!gr->is_water() && desc->get_type() != building_desc_t::dock && desc->get_type() != building_desc_t::flat_dock)
			{
				if(!gr->hat_wege()) {
					// save certain object types
					for (uint8 i = 0; i < gr->obj_count(); i++) {
						obj_t *const obj = gr->obj_bei(i);
						obj_t::typ const objtype = obj->get_typ();
						if (objtype == obj_t::leitung || objtype == obj_t::pillar) {
							keptobjs.append(obj);
						}
					}
					for (size_t i = 0; i < keptobjs.get_count(); i++) {
						gr->obj_remove(keptobjs[i]);
					}

					// delete everything except vehicles
					gr->obj_loesche_alle(player);
				}
				needs_ground_recalc |= gr->get_grund_hang()!=slope_t::flat;
				// Build fundament up or down?  Up is the default.
				bool build_up = true;
				if (dim.x == 1 && dim.y == 1) {
					// Consider building DOWNWARD.
					koord front_side_neighbor= koord(0,0);
					koord other_front_side_neighbor= koord(0,0);
					switch (org_layout) {
						case 12:
						case 4: // SE
							other_front_side_neighbor = koord(1,0);
							// fall through
						case 8:
						case 0: // south
							front_side_neighbor = koord(0,1);
							break;
						case 13:
						case 5: // NE
							other_front_side_neighbor = koord(0,-1);
							// fall through
						case 9:
						case 1: // east
							front_side_neighbor = koord(1,0);
							break;
						case 14:
						case 6: // NW
							other_front_side_neighbor = koord(-1,0);
							// fall through
						case 10:
						case 2: // north
							front_side_neighbor = koord(0,-1);
							break;
						case 15:
						case 7: // SW
							other_front_side_neighbor = koord(0,1);
							// fall through
						case 11:
						case 3: // west
							front_side_neighbor = koord(-1,0);
							break;
						default: // should not happen
							break;
					}
					if(  front_side_neighbor != koord(0,0)  ) {
						const grund_t* front_gr = welt->lookup_kartenboden(pos.get_2d() + front_side_neighbor);
						if(  !front_gr || (front_gr->get_weg_hang() != slope_t::flat)  ) {
							// Nothing in front, or sloped.  For a corner building, try the other front side.
							if(  other_front_side_neighbor != koord(0,0)  ) {
								const grund_t* other_front_gr = welt->lookup_kartenboden(pos.get_2d() + other_front_side_neighbor);
								if (other_front_gr && (other_front_gr->get_weg_hang() == slope_t::flat)  ) {
									// Prefer the other front side.
									front_side_neighbor = other_front_side_neighbor;
									front_gr = other_front_gr;
								}
							}
						}
						if(  front_gr  ) {
							// There really is land in front of this building
							sint8 front_z = front_gr->get_pos().z + front_gr->get_weg_yoff();
							// get_weg_yoff will change from the "ground" level to the level of
							// a flat bridge end on a slope.  (Otherwise it's zero.)
							// So this is the desired level...
							if (front_z == gr->get_pos().z &&
							    front_z > welt->get_groundwater()) {
								// Build down to meet the front side.
								build_up = false;
							}
							// Otherwise, prefer to build up.
							// We are doing the correct thing whenever the building is facing a flat road.
							// When it isn't, we are doing the right thing (digging down to the base of the
							// road) in the typical circumstance.  It looks bad on "inside corners" with
							// hills on two sides; it looks good everywhere else.
						}
					}
				}
				// Build a "fundament" to put the building on.
				grund_t *gr2 = new fundament_t(gr->get_pos(), gr->get_grund_hang(), build_up);
				welt->access(gr->get_pos().get_2d())->boden_ersetzen(gr, gr2);
				gr = gr2;
			}
//DBG_DEBUG("hausbauer_t::build()","ground count now %i",gr->obj_count());
			gebaeude_t *gb;
			if(tile->get_desc()->is_signalbox())
			{
				gb = new signalbox_t(gr->get_pos(), player, tile);
			}
			else
			{
				gb = new gebaeude_t(gr->get_pos(), player, tile);
			}

			if (first_building == NULL) {
				first_building = gb;
			}

			if(desc->is_factory()) {
				gb->set_fab((fabrik_t *)param);
			}
			// try to fake old building
			else if(welt->get_ticks() < 2) {
				// Hajo: after staring a new map, build fake old buildings
				gb->add_alter(10000ll);
			}
			gr->obj_add( gb );

			// restore saved objects
			for (size_t i = 0; i < keptobjs.get_count(); i++) {
				gr->obj_add(keptobjs[i]);
			}
			if(needs_ground_recalc  &&  welt->is_within_limits(pos.get_2d()+k+koord(1,1))  &&  (k.y+1==dim.y  ||  k.x+1==dim.x))
			{
				welt->lookup_kartenboden(pos.get_2d()+k+koord(1,0))->calc_image();
				welt->lookup_kartenboden(pos.get_2d()+k+koord(0,1))->calc_image();
				welt->lookup_kartenboden(pos.get_2d()+k+koord(1,1))->calc_image();
			}
			//gb->set_pos( gr->get_pos() );
			if(desc->is_attraction()) {
				welt->add_attraction( gb );
			}
			if(!desc->is_city_building() && !desc->is_signalbox()) {
				if(station_building.is_contained(desc))
				{
					if(desc->get_is_control_tower())
					{
						(*static_cast<halthandle_t *>(param))->add_control_tower();
						(*static_cast<halthandle_t *>(param))->recalc_status();
					}
					(*static_cast<halthandle_t *>(param))->add_grund(gr);
				}
				if(  desc->get_type() == building_desc_t::dock  ||  desc->get_type() == building_desc_t::flat_dock  ) {
					// it's a dock!
					gb->set_yoff(0);
				}
			}
			gr->calc_image();
			reliefkarte_t::get_karte()->calc_map_pixel(gr->get_pos().get_2d());
		}
	}
	// remove only once ...
	if(desc->get_type()==building_desc_t::monument) {
		unbuilt_monuments.remove( desc );
	}
	return first_building;
}


gebaeude_t *hausbauer_t::build_station_extension_depot(player_t *player, koord3d pos, int built_layout, const building_desc_t *desc, void *param)
{
	uint8 corner_layout = 6;	// assume single building (for more than 4 layouts)

	// adjust layout of neighbouring building
	if(desc->is_transport_building() &&  desc->get_all_layouts()>1) {

		int layout = built_layout & 9;

		// detect if we are connected at far (north/west) end
		sint8 offset = welt->lookup( pos )->get_weg_yoff()/TILE_HEIGHT_STEP;
		koord3d checkpos = pos+koord3d( (layout & 1 ? koord::east : koord::south), offset);
		grund_t * gr = welt->lookup( checkpos );

		if(!gr) {
			// check whether bridge end tile
			grund_t * gr_tmp = welt->lookup( pos+koord3d( (layout & 1 ? koord::east : koord::south),offset - 1) );
			if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 1) {
				gr = gr_tmp;
			}
			else {
				gr_tmp = welt->lookup( pos+koord3d( (layout & 1 ? koord::east : koord::south),offset - 2) );
				if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 2) {
					gr = gr_tmp;
				}
			}
		}

		if(gr) {
			gebaeude_t* gb = gr->find<gebaeude_t>();
			if(gb==NULL) {
				// no building on same level, check other levels
				const planquadrat_t *pl = welt->access(checkpos.get_2d());
				if (pl) {
					for(  uint8 i=0;  i<pl->get_boden_count();  i++  ) {
						gr = pl->get_boden_bei(i);
						if(gr->is_halt() && gr->get_halt().is_bound() ) {
							break;
						}
					}
				}
				gb = gr->find<gebaeude_t>();
			}
			if(  gb  &&  gb->get_tile()->get_desc()->is_transport_building() ) {
				corner_layout &= ~2; // clear near bit
				if(gb->get_tile()->get_desc()->get_all_layouts()>4) {
					koord xy = gb->get_tile()->get_offset();
					uint8 layoutbase = gb->get_tile()->get_layout();

					if((layoutbase & 1) == (layout & 1)) {
						layoutbase &= 0xb; // clear near bit on neighbour
						gb->set_tile( gb->get_tile()->get_desc()->get_tile(layoutbase, xy.x, xy.y), false );
					}
				}
			}
		}

		// detect if near (south/east) end
		gr = welt->lookup( pos+koord3d( (layout & 1 ? koord::west : koord::north), offset) );
		if(!gr) {
			// check whether bridge end tile
			grund_t * gr_tmp = welt->lookup( pos+koord3d( (layout & 1 ? koord::west : koord::north),offset - 1) );
			if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 1) {
				gr = gr_tmp;
			}
			else {
				gr_tmp = welt->lookup( pos+koord3d( (layout & 1 ? koord::west : koord::north),offset - 2) );
				if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 2) {
					gr = gr_tmp;
				}
			}
		}
		if(gr) {
			gebaeude_t* gb = gr->find<gebaeude_t>();
			if(gb  &&  gb->get_tile()->get_desc()->is_transport_building()) {
				corner_layout &= ~4; // clear far bit

				if(gb->get_tile()->get_desc()->get_all_layouts()>4) {
					koord xy = gb->get_tile()->get_offset();
					uint8 layoutbase = gb->get_tile()->get_layout();
					if((layoutbase & 1) == (layout & 1)) {
						layoutbase &= 0xd; // clear far bit on neighbour
						gb->set_tile( gb->get_tile()->get_desc()->get_tile(layoutbase, xy.x, xy.y), false );
					}
				}
			}
		}
	}

	// adjust layouts of the new building
	if(desc->get_all_layouts()>4) {
		built_layout = (corner_layout | (built_layout&9) ) % desc->get_all_layouts();
	}

	const building_tile_desc_t *tile = desc->get_tile(built_layout, 0, 0);
	gebaeude_t *gb;
	if(  desc->get_type() == building_desc_t::depot  ) {
		switch(  desc->get_extra()  ) {
			case track_wt:
				gb = new bahndepot_t(pos, player, tile);
				break;
			case tram_wt:
				gb = new tramdepot_t(pos, player, tile);
				break;
			case monorail_wt:
				gb = new monoraildepot_t(pos, player, tile);
				break;
			case maglev_wt:
				gb = new maglevdepot_t(pos, player, tile);
				break;
			case narrowgauge_wt:
				gb = new narrowgaugedepot_t(pos, player, tile);
				break;
			case road_wt:
				gb = new strassendepot_t(pos, player, tile);
				break;
			case water_wt:
				gb = new schiffdepot_t(pos, player, tile);
				break;
			case air_wt:
				gb = new airdepot_t(pos, player, tile);
				break;
			default:
				dbg->fatal("hausbauer_t::build_station_extension_depot()","waytpe %i has no depots!", desc->get_extra() );
				break;
		}
	}
	else if(desc->is_signalbox())
	{
		gb = new signalbox_t(pos, player, tile);
	}

	else {
		gb = new gebaeude_t(pos, player, tile);
	}
//DBG_MESSAGE("hausbauer_t::build_station_extension_depot()","building stop pri=%i",pri);

	// remove pointer
	grund_t *gr = welt->lookup(pos);
	zeiger_t* zeiger = gr->find<zeiger_t>();
	if(  zeiger  ) {
		gr->obj_remove(zeiger);
		zeiger->set_flag(obj_t::not_on_map);
	}

	gr->obj_add(gb);

	if(  station_building.is_contained(desc)  &&  desc->get_type()!=building_desc_t::depot && !desc->is_signalbox()) {
		// is a station/bus stop
		(*static_cast<halthandle_t *>(param))->add_grund(gr);
		gr->calc_image();
	}
	else {
		gb->calc_image();
	}

	if(desc->is_attraction()) {
		welt->add_attraction( gb );
	}

	// update minimap
	reliefkarte_t::get_karte()->calc_map_pixel(gb->get_pos().get_2d());

	return gb;
}


const building_tile_desc_t *hausbauer_t::find_tile(const char *name, int org_idx)
{
	int idx = org_idx;
	const building_desc_t *desc = desc_names.get(name);
	if(desc) {
		const int size = desc->get_y()*desc->get_x();
		if(  idx >= desc->get_all_layouts()*size  ) {
			idx %= desc->get_all_layouts()*size;
			DBG_MESSAGE("gebaeude_t::rdwr()","%s using tile %i instead of %i",name,idx,org_idx);
		}
		return desc->get_tile(idx);
	}
//	DBG_MESSAGE("hausbauer_t::find_tile()","\"%s\" not in hashtable",name);
	return NULL;
}


const building_desc_t* hausbauer_t::get_desc(const char *name)
{
	return desc_names.get(name);
}


const building_desc_t* hausbauer_t::get_random_station(const building_desc_t::btype btype, const waytype_t wt, const uint16 time, const uint16 enables)
{
	weighted_vector_tpl<const building_desc_t*> stops;

	if(  wt < 0  ) {
		return NULL;
	}

	FOR(vector_tpl<building_desc_t const*>, const desc, station_building) {
		if(  desc->get_type()== btype  &&  desc->get_extra()==(uint32)wt  &&  (enables==0  ||  (desc->get_enabled()&enables)!=0)  ) {
			// skip underground stations
			if( !desc->can_be_built_aboveground()) {
				continue;
			}
			// ok, now check timeline
			if(  desc->is_available(time)  ) {
				stops.append(desc,max(1,16-desc->get_level()*desc->get_x()*desc->get_y()));
			}
		}
	}
	return stops.empty() ? 0 : pick_any_weighted(stops);
}


const building_desc_t* hausbauer_t::get_special(uint32 bev, building_desc_t::btype btype, uint16 time, bool ignore_retire, climate cl)
{
	weighted_vector_tpl<const building_desc_t *> auswahl(16);

	vector_tpl<const building_desc_t*> *list = NULL;
	switch(btype) {
		case building_desc_t::townhall:
			list = &townhalls;
			break;
		case building_desc_t::attraction_city:
			list = &attractions_city;
			break;
		default:
			return NULL;
	}
	FOR(vector_tpl<building_desc_t const*>, const desc, *list) {
		// extra data contains number of inhabitants for building
		if(  desc->get_extra()==bev  ) {
			if(  cl==MAX_CLIMATES  ||  desc->is_allowed_climate(cl)  ) {
				// ok, now check timeline
				if(  time==0  ||  (desc->get_intro_year_month()<=time  &&  (ignore_retire  ||  desc->get_retire_year_month() > time)  )  ) {
					auswahl.append(desc, desc->get_distribution_weight());
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

const bool is_allowed_size(const building_desc_t* bldg, koord size) {
	if(size.x==-1  ||  bldg->get_size()==size) {
		return true;
	} else if(bldg->get_size().x==size.y  &&  bldg->get_size().y==size.x) {
		return true;
	} else {
		return false;
	}
}

/**
 * Try to find a suitable city building from the given list
 * This method will never return NULL if there is at least one valid entry in the list.
 * @param cl allowed climates
 */
const building_desc_t* hausbauer_t::get_city_building_from_list(const vector_tpl<const building_desc_t*>& building_list, koord pos_origin, koord size, uint16 time, climate cl, bool allow_earlier, uint32 clusters)
{
	weighted_vector_tpl<const building_desc_t *> selections(16);
	// calculate sum of level of replaced buildings
	uint16 sum_level = 0;
	bool checked[64][64];
	// initialize check flag.
	for(uint8 i=0; i<size.x; i++) {
		for(uint8 k=0; k<size.y; k++) {
			checked[i][k] = false;
		}
	}
	for(uint8 x = 0; x<size.x; x++) {
		for(uint8 y = 0; y<size.y; y++) {
			if(checked[x][y]) {
				// this position is already checked.
				continue;
			}
			const grund_t* gr = welt->lookup_kartenboden(pos_origin + koord(x,y));
			assert(gr);
			if(gr->ist_natur()) {
				continue;
			}
			const gebaeude_t* gb = gr->get_building();
			assert(gb  &&  gb->is_city_building());
			sum_level += gb->get_tile()->get_desc()->get_level();
			const uint8 gb_layout = gb->get_tile()->get_layout();
			koord gb_size = gb->get_tile()->get_desc()->get_size(gb_layout);
			for(uint8 gx=0; gx<gb_size.x; gx++) {
				for(uint8 gy=0; gy<gb_size.y; gy++) {
					// mark as checked tile.
					if(x+gx<size.x  &&  y+gy<size.y) checked[x+gx][y+gy] = true;
				}
			}
		}
	}
	sum_level += 1;
	const uint16 level_replaced = sum_level;
	const uint16 area_of_building = size.x*size.y;

	FOR(vector_tpl<building_desc_t const*>, const desc, building_list) {
		// only allow buildings of the designated size.
		if(!is_allowed_size(desc, size)) {
			continue;
		}

		const uint16 random = simrand(100, "static const building_desc_t* get_city_building_from_list");

		const int thislevel = desc->get_level();
		if(thislevel>sum_level) {
			if (selections.empty()  &&  thislevel-level_replaced<=2*area_of_building) {
				// Nothing of the correct level.  Continue with search of next level.
				sum_level = thislevel;
			}
			else {
				// We already found something of the correct level; stop.
				break;
			}
		}

		if(  thislevel == sum_level  &&  desc->get_distribution_weight() > 0  ) {
			if(  cl==MAX_CLIMATES  ||  desc->is_allowed_climate(cl)  ) {
				if(  time == 0  ||  (desc->get_intro_year_month() <= time  &&  ((allow_earlier && random > 65) || desc->get_retire_year_month() > time ))  ) {
					/* Level, time period, and climate are all OK.
					 * Now modify the distribution_weight rating by a factor based on the clusters.
					 */
					int distribution_weight = desc->get_distribution_weight();
					if(  clusters  ) {
						uint32 my_clusters = desc->get_clusters();
						if(  my_clusters & clusters  ) {
							distribution_weight *= stadt_t::get_cluster_factor();
						}
					}
					selections.append(desc, distribution_weight);
				}
			}
		}
	}

	if(selections.get_sum_weight()==0) {
		return NULL;
	}
	if(selections.get_count()==1) {
		return selections.front();
	}
	// now there is something to choose
	return pick_any_weighted(selections);
}


const building_desc_t* hausbauer_t::get_city_building_from_list(const vector_tpl<const building_desc_t*>& building_list, int level, koord size, uint16 time, climate cl, bool allow_earlier, uint32 clusters)
{
	weighted_vector_tpl<const building_desc_t *> selections(16);

//	DBG_MESSAGE("hausbauer_t::get_city_building_from_list()","target level %i", level );
	const building_desc_t *desc_at_least=NULL;
	FOR(vector_tpl<building_desc_t const*>, const desc, building_list)
	{
		const uint16 random = simrand(100, "static const building_desc_t* get_city_building_from_list");
		if(	desc->is_allowed_climate(cl)  &&  is_allowed_size(desc, size)  &&
			desc->get_distribution_weight()>0  &&
			(time==0  ||  (desc->get_intro_year_month()<=time  &&  ((allow_earlier && random > 65) || desc->get_retire_year_month()>time)))) {
			desc_at_least = desc;
		}

		const int thislevel = desc->get_level();
		if(thislevel>level) {
			if (selections.empty()) {
				// Nothing of the correct level. Continue with search on a higher level.
				level = thislevel;
			}
			else {
				// We already found something of the correct level; stop.
				break;
			}
		}

		if(  thislevel == level  &&  is_allowed_size(desc, size)  &&  desc->get_distribution_weight() > 0  ) {
			if(  cl==MAX_CLIMATES  ||  desc->is_allowed_climate(cl)  ) {
				if(  time == 0  ||  (desc->get_intro_year_month() <= time  &&  ((allow_earlier && random > 65) || desc->get_retire_year_month() > time ))  ) {
//					DBG_MESSAGE("hausbauer_t::get_city_building_from_list()","appended %s at %i", desc->get_name(), thislevel );
					/* Level, time period, and climate are all OK.
					 * Now modify the distribution_weight rating by a factor based on the clusters.
					 */
					int distribution_weight = desc->get_distribution_weight();
					if(  clusters  ) {
						uint32 my_clusters = desc->get_clusters();
						if(  my_clusters & clusters  ) {
							distribution_weight *= stadt_t::get_cluster_factor();
						}
					}
					selections.append(desc, distribution_weight);
				}
			}
		}
	}

	if(selections.get_sum_weight()==0) {
		// this is some level below, but at least it is something
		return desc_at_least;
	}
	if(selections.get_count()==1) {
		return selections.front();
	}
	// now there is something to choose
	return pick_any_weighted(selections);
}


const building_desc_t* hausbauer_t::get_commercial(koord pos_origin, koord size, uint16 time, climate cl, bool allow_earlier, uint32 clusters)
{
	return get_city_building_from_list(city_commercial, pos_origin, size, time, cl, allow_earlier, clusters);
}


const building_desc_t* hausbauer_t::get_commercial(int level, koord size, uint16 time, climate cl, bool allow_earlier, uint32 clusters)
{
	return get_city_building_from_list(city_commercial, level, size, time, cl, allow_earlier, clusters);
}


const building_desc_t* hausbauer_t::get_industrial(koord pos_origin, koord size, uint16 time, climate cl, bool allow_earlier, uint32 clusters)
{
	return get_city_building_from_list(city_industry, pos_origin, size, time, cl, allow_earlier, clusters);
}


const building_desc_t* hausbauer_t::get_industrial(int level, koord size, uint16 time, climate cl, bool allow_earlier, uint32 clusters)
{
	return get_city_building_from_list(city_industry, level, size, time, cl, allow_earlier, clusters);
}


const building_desc_t* hausbauer_t::get_residential(koord pos_origin, koord size, uint16 time, climate cl, bool allow_earlier, uint32 clusters)
{
	return get_city_building_from_list(city_residential, pos_origin, size, time, cl, allow_earlier, clusters);
}


const building_desc_t* hausbauer_t::get_residential(int level, koord size, uint16 time, climate cl, bool allow_earlier, uint32 clusters)
{
	return get_city_building_from_list(city_residential, level, size, time, cl, allow_earlier, clusters);
}


const building_desc_t* hausbauer_t::get_headquarter(int level, uint16 time)
{
	if(  level<0  ) {
		return NULL;
	}
	FOR(vector_tpl<building_desc_t const*>, const desc, hausbauer_t::headquarters) {
		if(  desc->get_extra()==(uint32)level  &&  desc->is_available(time)  ) {
			return desc;
		}
	}
	return NULL;
}


const building_desc_t *hausbauer_t::get_random_desc(vector_tpl<const building_desc_t *> &list, uint16 time, bool ignore_retire, climate cl)
{
	//"select from list" (Google)
	if (!list.empty()) {
		// previously just returned a random object; however, now we look at the distribution_weight entry
		weighted_vector_tpl<const building_desc_t *> auswahl(16);
		FOR(vector_tpl<building_desc_t const*>, const desc, list) {
			if((cl==MAX_CLIMATES  ||  desc->is_allowed_climate(cl))  &&  desc->get_distribution_weight()>0  &&  (time==0  ||  (desc->get_intro_year_month()<=time  &&  (ignore_retire  ||  desc->get_retire_year_month()>time)  )  )  ) {
//				DBG_MESSAGE("hausbauer_t::get_random_desc()","appended %s at %i", desc->get_name(), thislevel );
				auswahl.append(desc, desc->get_distribution_weight());
			}
		}
		// now look what we have got ...
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


const vector_tpl<const building_desc_t*>* hausbauer_t::get_list(const building_desc_t::btype typ)
{
	switch (typ) {
		case building_desc_t::monument:         return &unbuilt_monuments;
		case building_desc_t::attraction_land: return &attractions_land;
		case building_desc_t::headquarters:      return &headquarters;
		case building_desc_t::townhall:         return &townhalls;
		case building_desc_t::attraction_city: return &attractions_city;
		case building_desc_t::dock:
		case building_desc_t::flat_dock:
		case building_desc_t::depot:
		case building_desc_t::generic_stop:
		case building_desc_t::generic_extension:
		                                    return &station_building;
		default:                            return NULL;
	}
}


const vector_tpl<const building_desc_t*>* hausbauer_t::get_citybuilding_list(const building_desc_t::btype typ)
{
	switch (typ) {
		case building_desc_t::city_res:		return &city_residential;
		case building_desc_t::city_com:		return &city_commercial;
		case building_desc_t::city_ind:		return &city_industry;
		default:						return NULL;
	}
}

void hausbauer_t::new_month()
{
	FOR(vector_tpl<const building_desc_t*>, building, station_building)
	{
		const uint16 current_month = welt->get_timeline_year_month();
		const uint16 intro_month = building->get_intro_year_month();
		{
			if(intro_month == current_month)
			{
				cbuffer_t buf;
				buf.printf(translator::translate("New %s now available:\n%s\n"), "building", translator::translate(building->get_name()));
				welt->get_message()->add_message(buf, koord::invalid, message_t::new_vehicle, NEW_VEHICLE, IMG_EMPTY);
			}
		}
	}
}
