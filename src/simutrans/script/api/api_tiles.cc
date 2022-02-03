/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_tiles.cc exports tile related functions. */

#include "api_simple.h"
#include "get_next.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../tool/simmenu.h"
#include "../../world/simworld.h"
#include "../../ground/wasser.h"

#include "../../simconvoi.h"
#include "../../vehicle/vehicle.h"

namespace script_api {
	declare_enum_param(grund_t::flag_values, uint8, "flags");
	declare_specialized_param(depot_t*, "t|x|y", "depot_x");
};

using namespace script_api;


SQInteger get_next_object(HSQUIRRELVM vm)
{
	grund_t *gr = param<grund_t*>::get(vm, 1);
	return generic_get_next(vm, gr ? gr->get_top() : 0);
}


SQInteger get_object_index(HSQUIRRELVM vm)
{
	grund_t *gr = param<grund_t*>::get(vm, 1);
	uint8 index = param<uint8>::get(vm, 2);

	obj_t *obj = NULL;
	if (gr  &&  index < gr->get_top()) {
		obj = gr->obj_bei(index);
	}
	return param<obj_t*>::push(vm, obj);
}

SQInteger get_object_count(HSQUIRRELVM vm)
{
	grund_t *gr = param<grund_t*>::get(vm, 1);
	return param<uint8>::push(vm, gr ? gr->get_top() : 0);
}

call_tool_work tile_remove_object(grund_t* gr, player_t* player, obj_t::typ type)
{
	cbuffer_t buf;
	buf.printf("%d", (int)type);
	return call_tool_work(TOOL_REMOVER | GENERAL_TOOL, (const char*)buf, 0, player, gr->get_pos());
}

// return way ribis, have to implement a wrapper, to correctly rotate ribi
static SQInteger get_way_ribi(HSQUIRRELVM vm)
{
	grund_t *gr = param<grund_t*>::get(vm, 1);
	waytype_t wt = param<waytype_t>::get(vm, 2);
	bool masked = param<bool>::get(vm, 3);

	ribi_t::ribi ribi = gr ? (masked ? gr->get_weg_ribi(wt) : gr->get_weg_ribi_unmasked(wt) ) : 0;

	return param<my_ribi_t>::push(vm, ribi);
}

static SQInteger get_canal_ribi(HSQUIRRELVM vm)
{
	grund_t *gr = param<grund_t*>::get(vm, 1);
	ribi_t::ribi ribi = gr  &&  gr->is_water() ?  ((wasser_t*)gr)->get_canal_ribi() : (ribi_t::ribi)0;

	return param<my_ribi_t>::push(vm, ribi);
}


// we have to implement a wrapper, to correctly rotate ribi
grund_t* get_neighbour(grund_t *gr, waytype_t wt, my_ribi_t ribi)
{
	grund_t *to = NULL;
	if (gr  &&  ribi_t::is_single(ribi)) {
		gr->get_neighbour(to, wt, ribi);
	}
	return to;
}

my_slope_t get_slope(grund_t *gr)
{
	return gr->get_grund_hang();
}


halthandle_t get_first_halt_on_square(planquadrat_t* plan)
{
	return plan->get_halt(NULL);
}

vector_tpl<halthandle_t> const& square_get_halt_list(planquadrat_t *plan)
{
	static vector_tpl<halthandle_t> list;
	list.clear();
	if (plan) {
		const halthandle_t* haltlist = plan->get_haltlist();
		for(uint8 i=0, end = plan->get_haltlist_count(); i < end; i++) {
			list.append(haltlist[i]);
		}
	}
	return list;
}

vector_tpl<convoihandle_t> const get_convoy_list(grund_t* gr)
{
	static vector_tpl<convoihandle_t> list;
	list.clear();

	for( uint8 n = 0; n<gr->get_top(); n++) {
		obj_t* obj = gr->obj_bei( n );
		if( vehicle_t* veh = dynamic_cast<vehicle_t*>(obj) ) {
			convoihandle_t cnv;
			if( veh->get_convoi() ) {
				list.append_unique( veh->get_convoi()->self );
			}
		}
	}
	return list;
}

void export_tiles(HSQUIRRELVM vm)
{
	/**
	 * Class to access tiles on the map.
	 *
	 * There is the possibility to iterate through all objects on the tile:
	 * @code
	 * local tile = tile_x( ... )
	 * foreach(thing in tile) {
	 *     ... // thing is an instance of the map_object_x (or a derived) class
	 * }
	 * @endcode
	 */
	begin_class(vm, "tile_x", "extend_get,coord3d,ingame_object");

	/**
	 * Constructor. Returns tile at particular 3d coordinate.
	 * If not tile is found, it returns the ground tile.
	 * Raises error, if (@p x, @p y) coordinates are out-of-range.
	 * @param x x-coordinate
	 * @param y y-coordinate
	 * @param z z-coordinate
	 * @typemask (integer,integer,integer)
	 */
	// actually defined in simutrans/script/script_base.nut
	// register_function(..., "constructor", ...);

	/**
	 * @returns if object is still valid.
	 */
	export_is_valid<grund_t*>(vm); //register_function("is_valid")
	/**
	 * Search for a given object type on the tile.
	 * @return some instance or null if not found
	 */
	register_method(vm, &grund_t::suche_obj, "find_object");
	/**
	 * Remove object of given type from the tile.
	 * Type @p type must be one of the following: ::mo_label, ::mo_pedestrian, ::mo_city_car, ::mo_powerline, ::mo_transformer_s,
	 *  ::mo_transformer_c, ::mo_signal, ::mo_roadsign, ::mo_wayobj, ::mo_tunnel, ::mo_bridge, ::mo_field, ::mo_building, ::mo_depot_rail.
	 * Setting @p type to ::mo_depot_rail deletes depots of every type.
	 * @param pl player that pays for removal
	 * @param type object type
	 * @returns null upon success, an error message otherwise
	 * @warning Does not work with all object types.
	 * @ingroup game_cmd
	 */
	register_method(vm, &tile_remove_object, "remove_object", true);
	/**
	 * Access halt at this tile.
	 * @returns halt_x instance or null/false if no halt is present
	 */
	register_method(vm, &grund_t::get_halt, "get_halt");

	/**
	 * Queries tile type.
	 * @returns true if tile is an ocean tile
	 */
	register_method(vm, &grund_t::is_water, "is_water");

	/**
	 * Queries tile type.
	 * @returns true if tile is an bridge tile (including bridge starts)
	 */
	register_method(vm, &grund_t::ist_bruecke, "is_bridge");

	/**
	 * Queries tile type.
	 * @returns true if tile is an tunnel tile (including tunnel mouths)
	 */
	register_method(vm, &grund_t::ist_tunnel, "is_tunnel");

	/**
	 * Queries tile type.
	 * @returns true if tile is empty (no ways, buildings, powerlines, halts, not water tile)
	 */
	register_method(vm, &grund_t::ist_natur, "is_empty");

	/**
	 * Queries tile type.
	 * @returns true if tile on ground (not bridge/elevated, not tunnel)
	 */
	register_method(vm, &grund_t::ist_karten_boden, "is_ground");

	/**
	 * Returns encoded slope of tile, zero means flat tile.
	 * @returns slope
	 */
	register_method(vm, &get_slope, "get_slope", true);

	/**
	 * Returns text of a sign on this tile (station sign, city name, label).
	 * @returns text
	 */
	register_method(vm, &grund_t::get_text, "get_text");

	/**
	 * Queries ways on the tile.
	 * @param wt waytype
	 * @returns true if there is a way with the given waytype on the tile.
	 */
	register_method(vm, &grund_t::hat_weg, "has_way");

	/**
	 * Queries ways on the tile.
	 * @returns true if there is at least one way on the tile
	 */
	register_method(vm, &grund_t::hat_wege, "has_ways");

	/**
	 * Queries ways on the tile.
	 * @returns true if there is are two ways on the tile
	 */
	register_method<bool (grund_t::*)() const>(vm, &grund_t::has_two_ways, "has_two_ways");
	/**
	 * Returns way_x object on this tile of way type @p wt if present
	 * @param wt waytype
	 * @returns way object or null
	 */
	register_method(vm, &grund_t::get_weg, "get_way");
	/**
	 * Return directions in which ways on this tile go. One-way signs are ignored here.
	 * @param wt waytype
	 * @returns direction
	 * @typemask dir(waytypes)
	 */
	register_function_fv(vm, &get_way_ribi, "get_way_dirs", 2, "xi", freevariable<bool>(false) );
	/**
	 * Return directions in which ways on this tile go. Some signs restrict available directions.
	 * @param wt waytype
	 * @returns direction
	 * @typemask dir(waytypes)
	 */
	register_function_fv(vm, &get_way_ribi, "get_way_dirs_masked", 2, "xi", freevariable<bool>(true) );
	/**
	 * Return directions in which canals branch off from water tiles.
	 * Used for jps pathfinding on water tiles.
	 * @returns direction
	 * @typemask dir()
	 */
	register_function(vm, &get_canal_ribi, "get_canal_ribi", 1, "x");
	/**
	 * Returns neighbour if one follows way in the given direction.
	 * @param wt waytype, if equal to @ref wt_all then ways are ignored.
	 * @param d direction
	 * @return neighbour tile or null
	 */
	register_method(vm, &get_neighbour, "get_neighbour", true);
	/**
	 * Returns depot_x object on this tile if any depot is present.
	 * @returns depot object or null
	 */
	register_method(vm, &grund_t::get_depot, "get_depot");
	/**
	 * Checks whether player can delete all objects on the tile.
	 * @param pl player
	 * @return error message or null if player can delete everything
	 */
	register_method(vm, &grund_t::kann_alle_obj_entfernen, "can_remove_all_objects");

	/** @name Functions to mark tiles.
	 * Methods to mark, unmark, and check mark-status of tiles.
	 * Mark flag can be reset by cursor movement.
	 * @warning In network games, they only work on server.
	 */
	//@{
	/// Check if tile is marked.
	register_method_fv(vm, &grund_t::get_flag, "is_marked", freevariable<uint8>(grund_t::marked));
	/// Unmark tile.
	register_method_fv(vm, &grund_t::clear_flag, "unmark", freevariable<uint8>(grund_t::marked));
	/// Mark tile.
	register_method_fv(vm, &grund_t::set_flag, "mark", freevariable<uint8>(grund_t::marked));
	//@}

	/**
	 * @return convoy list by tile
	 */
	register_method(vm, &get_convoy_list, "get_convoys", true);

#ifdef SQAPI_DOC // document members
	/**
	 * List to iterate through all objects on this tile.
	 * @code
	 * t= tile_x(47,11)
	 * foreach(obj in t.get_objects()) {
	 *    ...
	 * }
	 * @endcode
	 */
	tile_object_list_x get_objects();
#endif

	end_class(vm);

	/**
	 * Class that holds an iterator through the list of objects on a particular tile.
	 *
	 * For an example see tile_x.
	 */
	begin_class(vm, "tile_object_list_x", "coord3d");
	/**
	 * Meta-method to be used in foreach loops to loop over all objects on the tile. Do not call it directly.
	 */
	register_function(vm, get_next_object,  "_nexti",  2, "x o|i");
	/**
	 * Meta-method to be used in foreach loops to loop over all objects on the tile. Do not call it directly.
	 */
	register_function(vm, get_object_index, "_get",    2, "x i|s");
	/**
	 * Returns number of objects on the tile.
	 * @typemask integer()
	 */
	register_function(vm, get_object_count, "get_count",  1, "x");

	end_class(vm);

	/**
	 * Class to map squares, which holds all the tiles on one particular coordinate.
	 */
	begin_class(vm, "square_x", "extend_get,coord,ingame_object");

	/**
	 * Constructor. Returns map square at particular 2d coordinate.
	 * Raises error, if (@p x, @p y) coordinate are out-of-range.
	 * @param x x-coordinate
	 * @param y z-coordinate
	 * @typemask (integer,integer)
	 */
	// actually defined in simutrans/script/script_base.nut
	// register_function(..., "constructor", ...);

	/**
	 * @returns if object is still valid.
	 */
	export_is_valid<planquadrat_t*>(vm); //register_function("is_valid")
	/**
	 * Access some halt at this square.
	 * @deprecated Use square_x::get_player_halt or tile_x::get_halt instead!
	 * @returns halt_x instance or null/false if no halt is present
	 */
	register_method(vm, &get_first_halt_on_square, "get_halt", true);

	/**
	 * Access halt of this player at this map position.
	 * @param pl potential owner of halt
	 * @returns halt_x instance or null/false if no halt is present
	 */
	register_method(vm, &planquadrat_t::get_halt, "get_player_halt");

	/**
	 * Access tile at specified height.
	 * @param z height
	 * @returns tile_x or null/false if nothing found
	 */
	register_method(vm, &planquadrat_t::get_boden_in_hoehe, "get_tile_at_height");

	/**
	 * Access ground tile.
	 * @returns tile_x instance
	 */
	register_method(vm, &planquadrat_t::get_kartenboden, "get_ground_tile");

	/**
	 * Returns list of stations that cover this tile.
	 * @typemask array<halt_x>
	 */
	register_method(vm, &square_get_halt_list, "get_halt_list", true);

	/**
	 * Returns climate of ground tile.
	 */
	register_method(vm, &planquadrat_t::get_climate, "get_climate");

	end_class(vm);
}
