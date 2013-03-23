#include "api.h"

/** @file api_tiles.cc exports tile related functions. */

#include "get_next.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../simworld.h"

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

	ding_t *ding = NULL;
	if (gr  &&  index < gr->get_top()) {
		ding = gr->obj_bei(index);
	}
	return param<ding_t*>::push(vm, ding);
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
	begin_class(vm, "tile_x", "extend_get,coord3d");

	/**
	 * Constructor. Returns tile at particular 3d coordinate.
	 * If not tile is found, it returns the ground tile.
	 * Raises error, if (@p x, @p y) coordinate are out-of-range.
	 * @param x x-coordinate
	 * @param y z-coordinate
	 * @param y z-coordinate
	 * @typemask (integer,integer,integer)
	 */
	// actually defined simutrans/script/scenario_base.nut
	// register_function(..., "constructor", ...);


	/**
	 * Search for a given object type on the tile.
	 * @return some instance or null if not found
	 */
	register_method(vm, &grund_t::suche_obj, "find_object");

	/**
	 * Meta-method to be used in foreach loops to loop over all objects on the tile. Do not call it directly.
	 */
	register_function(vm, get_next_object,  "_nexti",  2, "x o|i");
	/**
	 * Meta-method to be used in foreach loops to loop over all objects on the tile. Do not call it directly.
	 */
	register_function(vm, get_object_index, "_get",    2, "xi");

	/**
	 * Access halt at this tile.
	 * @returns halt_x instance or null/false if no halt is present
	 */
	register_method(vm, &grund_t::get_halt, "get_halt");

	/**
	 * Queries tile type.
	 * @returns true if tile is an ocean tile
	 */
	register_method(vm, &grund_t::ist_wasser, "is_water");

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
	end_class(vm);

	/**
	 * Class to map squares, which holds all the tiles on one particular coordinate.
	 */
	begin_class(vm, "square_x", "extend_get,coord");

	/**
	 * Constructor. Returns map square at particular 2d coordinate.
	 * Raises error, if (@p x, @p y) coordinate are out-of-range.
	 * @param x x-coordinate
	 * @param y z-coordinate
	 * @typemask (integer,integer)
	 */
	// actually defined simutrans/script/scenario_base.nut
	// register_function(..., "constructor", ...);
	/**
	 * Access halt at this tile.
	 * @returns halt_x instance or null/false if no halt is present
	 */
	register_method(vm, &planquadrat_t::get_halt, "get_halt");

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
	end_class(vm);
}
