#include "api.h"

/** @file api_tiles.cc exports tile related functions. */

#include "../api_class.h"
#include "../api_function.h"
#include "../../simworld.h"


#define begin_class(c,p) push_class(vm, c);
#define end_class() sq_pop(vm,1);

using namespace script_api;

void export_tiles(HSQUIRRELVM vm)
{
	/**
	 * Class to access tiles on the map.
	 */
	begin_class("tile_x", "extend_get,coord3d");

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
	 * Access halt at this tile.
	 * @returns halt_x instance or null/false if no halt is present
	 */
	register_method(vm, &grund_t::get_halt, "get_halt");
	end_class();

	/**
	 * Class to map squares, which holds all the tiles on one particular coordinate.
	 */
	begin_class("square_x", "extend_get,coord");

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
	end_class();
}
