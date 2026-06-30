//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for tool_remove_house_t: removes city buildings only, never roads.
// Uses CITY_RES_2X2 (explicitly city_res type, 2x2 tiles).
//
// Note on the two-click / area script API:
//   The script API's two-click mode (work(pl, start, end, "")) requires
//   is_work_here_network_safe() to return true for the first click, which
//   contradicts the one_click = true mode used for interactive area dragging.
//   Area behaviour is therefore tested by calling the tool once per tile.
//


// One-click removes a city building; the road on a nearby tile is not affected.
function test_remove_house_one_click()
{
	local public_pl = player_x(1)
	local remover   = command_x(tool_remove_house)

	// Setup: city at (8,8), road from (2,5) to (6,5), CITY_RES_2X2 at (2,2).
	// CITY_RES_2X2 is city_res type (2x2), occupying (2,2),(3,2),(2,3),(3,3).
	ASSERT_EQUAL(command_x(tool_add_city).work(public_pl, coord3d(8, 8, 0), "0"), null)
	ASSERT_EQUAL(command_x(tool_build_way).work(public_pl, coord3d(2, 5, 0), coord3d(6, 5, 0), "cobblestone_road"), null)
	ASSERT_EQUAL(command_x(tool_build_house).work(public_pl, coord3d(2, 2, 0), "11CITY_RES_2X2"), null)

	// Building is present before removal.
	ASSERT_TRUE(tile_x(2, 2, 0).find_object(mo_building) != null)

	// One-click removal at the origin tile of the building.
	ASSERT_EQUAL(remover.work(public_pl, coord3d(2, 2, 0)), null)

	// All tiles of the 2x2 building must be cleared.
	ASSERT_TRUE(tile_x(2, 2, 0).find_object(mo_building) == null)
	ASSERT_TRUE(tile_x(3, 2, 0).find_object(mo_building) == null)
	ASSERT_TRUE(tile_x(2, 3, 0).find_object(mo_building) == null)
	ASSERT_TRUE(tile_x(3, 3, 0).find_object(mo_building) == null)

	// Road at y=5 must still be present.
	ASSERT_FALSE(tile_x(2, 5, 0).is_empty())
	ASSERT_FALSE(tile_x(6, 5, 0).is_empty())

	// Applying the tool directly on a road tile must leave the road intact.
	remover.work(public_pl, coord3d(4, 5, 0))
	ASSERT_FALSE(tile_x(4, 5, 0).is_empty())

	// Cleanup.
	ASSERT_EQUAL(command_x(tool_remove_way).work(public_pl, coord3d(2, 5, 0), coord3d(6, 5, 0), "" + wt_road), null)
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(8, 8, 0)), null)
	command_x(tool_remover).work(public_pl, coord3d(7, 9, 0))
	RESET_ALL_PLAYER_FUNDS()
}


// Area removal: apply the tool to every tile in a rectangle, removing all city
// buildings inside it while leaving the road outside untouched.
//
// Note: the script API drives area removal by individual one-click calls per
// tile (see file header note). The C++ do_work(start, end) area path is
// exercised through the interactive GUI drag.
function test_remove_house_area()
{
	local public_pl = player_x(1)
	local remover   = command_x(tool_remove_house)

	// Setup: city at (8,8), road from (2,6) to (8,6) (outside the sweep area),
	// two CITY_RES_2X2 buildings at (2,2) and (5,2).
	// Building A occupies (2,2)-(3,3), building B occupies (5,2)-(6,3).
	ASSERT_EQUAL(command_x(tool_add_city).work(public_pl, coord3d(8, 8, 0), "0"), null)
	ASSERT_EQUAL(command_x(tool_build_way).work(public_pl, coord3d(2, 6, 0), coord3d(8, 6, 0), "cobblestone_road"), null)
	ASSERT_EQUAL(command_x(tool_build_house).work(public_pl, coord3d(2, 2, 0), "11CITY_RES_2X2"), null)
	ASSERT_EQUAL(command_x(tool_build_house).work(public_pl, coord3d(5, 2, 0), "11CITY_RES_2X2"), null)

	// Both buildings present before removal.
	ASSERT_TRUE(tile_x(2, 2, 0).find_object(mo_building) != null)
	ASSERT_TRUE(tile_x(5, 2, 0).find_object(mo_building) != null)

	// Sweep every tile in (0,0)-(7,4): covers both buildings, stops before y=6 road.
	for (local y = 0; y <= 4; y++) {
		for (local x = 0; x <= 7; x++) {
			remover.work(public_pl, coord3d(x, y, 0))
		}
	}

	// Both buildings must be gone.
	ASSERT_TRUE(tile_x(2, 2, 0).find_object(mo_building) == null)
	ASSERT_TRUE(tile_x(3, 2, 0).find_object(mo_building) == null)
	ASSERT_TRUE(tile_x(2, 3, 0).find_object(mo_building) == null)
	ASSERT_TRUE(tile_x(3, 3, 0).find_object(mo_building) == null)
	ASSERT_TRUE(tile_x(5, 2, 0).find_object(mo_building) == null)
	ASSERT_TRUE(tile_x(6, 2, 0).find_object(mo_building) == null)
	ASSERT_TRUE(tile_x(5, 3, 0).find_object(mo_building) == null)
	ASSERT_TRUE(tile_x(6, 3, 0).find_object(mo_building) == null)

	// Road at y=6 must still be present.
	ASSERT_FALSE(tile_x(2, 6, 0).is_empty())
	ASSERT_FALSE(tile_x(8, 6, 0).is_empty())

	// Cleanup.
	ASSERT_EQUAL(command_x(tool_remove_way).work(public_pl, coord3d(2, 6, 0), coord3d(8, 6, 0), "" + wt_road), null)
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(8, 8, 0)), null)
	command_x(tool_remover).work(public_pl, coord3d(7, 9, 0))
	RESET_ALL_PLAYER_FUNDS()
}
