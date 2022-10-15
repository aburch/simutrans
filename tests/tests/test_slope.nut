//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//

//
// Test set_/can_set_slope
//

function test_slope_to_dir()
{
	local slope_to_dir = {}

	slope_to_dir[slope.north] <- dir.south
	slope_to_dir[slope.east ] <- dir.west
	slope_to_dir[slope.south] <- dir.north
	slope_to_dir[slope.west ] <- dir.east

	slope_to_dir[2*slope.north] <- dir.south
	slope_to_dir[2*slope.east ] <- dir.west
	slope_to_dir[2*slope.south] <- dir.north
	slope_to_dir[2*slope.west ] <- dir.east

	for (local sl = 0; sl < 81; ++sl) {
		if (sl in slope_to_dir) {
			ASSERT_EQUAL(slope.to_dir(sl), slope_to_dir[sl])
		}
		else {
			ASSERT_EQUAL(slope.to_dir(sl), dir.none)
		}
	}
}


function test_slope_can_set()
{
	local pl = player_x(0)
	local pos = coord3d(2, 3, 0)

	for (local sl = 0; sl < slope.raised; ++sl) {
		for (local target_sl = 0; target_sl < slope.raised; ++target_sl) {
			command_x.set_slope(pl, pos, sl)
			ASSERT_EQUAL(tile_x(2, 3, 0).get_slope(), sl)
			RESET_ALL_PLAYER_FUNDS()

			local expected = ""
			ASSERT_EQUAL(command_x.can_set_slope(pl, pos, sl), expected)

			local sq = square_x(2, 3)
			ASSERT_TRUE(sq != null && sq.is_valid())
			ASSERT_EQUAL(sq.get_climate(), cl_mediterran)

			local tile = sq.get_tile_at_height(0)
			ASSERT_TRUE(tile != null && tile.is_valid())
			ASSERT_EQUAL(tile.get_slope(), sl)

			ASSERT_EQUAL(pl.get_current_cash(),        200000)     // get_current_cash is in credits (returns float)
			ASSERT_EQUAL(pl.get_current_net_wealth(),  200000*100) // get_current_net_wealth is in 1/100 credits
		}
	}

	// reset to normal slope
	command_x.set_slope(pl, pos + coord3d(0, 0, 1), slope.all_down_slope)
	command_x.set_slope(pl, pos,                    slope.all_down_slope)
	RESET_ALL_PLAYER_FUNDS()

	ASSERT_EQUAL(command_x.can_set_slope(pl, pos + coord3d(0, 0, 1), slope.all_up_slope), "")
	ASSERT_EQUAL(command_x.can_set_slope(pl, pos - coord3d(0, 0, 1), slope.all_down_slope), "")

	ASSERT_EQUAL(pl.get_current_cash(),        200000)     // get_current_cash is in credits (returns float)
	ASSERT_EQUAL(pl.get_current_net_wealth(),  200000*100) // get_current_net_wealth is in 1/100 credits

	RESET_ALL_PLAYER_FUNDS()
}


function test_slope_set_and_restore()
{
	local pl = player_x(0)
	local setslope = command_x.set_slope
	local restoreslope = command_x(tool_restoreslope)

	{
		// FIXME Crash when the "" are omitted (default_param is null)
		ASSERT_EQUAL(setslope(pl, coord3d(2, 3, 0), slope.north), null)
		ASSERT_EQUAL(tile_x(2, 3, 0).get_slope(), slope.north)

		ASSERT_EQUAL(setslope(pl, coord3d(2, 3, 0), slope.south), null)
		ASSERT_EQUAL(tile_x(2, 3, 0).get_slope(), slope.south)

		ASSERT_EQUAL(restoreslope.work(pl, coord3d(2, 3, 0)), null)
		ASSERT_EQUAL(tile_x(2, 3, 0).get_slope(), slope.flat)
	}

	{
		ASSERT_EQUAL(setslope(pl, coord3d(2, 3, 0), slope.all_up_slope), null)
		ASSERT_TRUE(tile_x(2, 3, 0).is_valid())
		ASSERT_TRUE(tile_x(2, 3, 1).is_valid())
		ASSERT_EQUAL(tile_x(2, 3, 1).get_slope(), slope.flat)

		// fails as expected because ground is 1 unit higher
		ASSERT_EQUAL(setslope(pl, coord3d(2, 3, 0), slope.all_up_slope), "")

		// TODO check tile height
		ASSERT_EQUAL(restoreslope.work(pl, coord3d(2, 3, 0)), "")
		ASSERT_TRUE(tile_x(2, 3, 0).is_valid())
		ASSERT_EQUAL(tile_x(2, 3, 0).get_slope(), slope.flat)

		ASSERT_EQUAL(setslope(pl, coord3d(2, 3, 1), slope.all_down_slope), null)
		ASSERT_EQUAL(tile_x(2, 3, 1).get_slope(), slope.flat)
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_slope_set_near_map_border()
{
	local pl = player_x(0)
	local setslope = command_x.set_slope

	// map edge
	{
		for (local sl = 0; sl < slope.raised; ++sl) {
			ASSERT_EQUAL(setslope(pl, coord3d(0, 3, 0), sl), "Zu nah am Kartenrand")
		}
	}

	// map corner
	{
		for (local sl = 0; sl < slope.raised; ++sl) {
			ASSERT_EQUAL(setslope(pl, coord3d(0, 0, 0), sl), "Zu nah am Kartenrand")
		}
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_slope_max_height_diff()
{
	local pl = player_x(0)
	local setslope = command_x.set_slope

	// build upwards, height difference = 4
	{
		ASSERT_EQUAL(setslope(pl, coord3d(3, 2, 0), slope.all_up_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(3, 2, 1), slope.all_up_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(3, 2, 2), slope.all_up_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(3, 2, 3), slope.all_up_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(3, 2, 4), slope.all_up_slope), "Maximum tile height difference reached.")
	}

	// diagonally, the height difference is unlimited (technically limited to 2*max_diff)
	{
		ASSERT_EQUAL(setslope(pl, coord3d(2, 3,  0), slope.all_down_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(2, 3, -1), slope.all_down_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(2, 3, -2), slope.all_down_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(2, 3, -3), slope.all_down_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(2, 3, -4), slope.all_down_slope), "Maximum tile height difference reached.")
	}

	// and clean up
	ASSERT_EQUAL(setslope(pl, coord3d(3, 2, 4), slope.all_down_slope), null)
	ASSERT_EQUAL(setslope(pl, coord3d(3, 2, 3), slope.all_down_slope), null)
	ASSERT_EQUAL(setslope(pl, coord3d(3, 2, 2), slope.all_down_slope), null)
	ASSERT_EQUAL(setslope(pl, coord3d(3, 2, 1), slope.all_down_slope), null)

	ASSERT_EQUAL(setslope(pl, coord3d(2, 3, -4), slope.all_up_slope), null)
	ASSERT_EQUAL(setslope(pl, coord3d(2, 3, -3), slope.all_up_slope), null)
	ASSERT_EQUAL(setslope(pl, coord3d(2, 3, -2), slope.all_up_slope), null)
	ASSERT_EQUAL(setslope(pl, coord3d(2, 3, -1), slope.all_up_slope), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_slope_get_price()
{
	local pl = player_x(0)
	for (local sl = 0; sl < slope.raised; ++sl) {
		ASSERT_EQUAL(command_x.slope_get_price(slope.flat), 2000 * 100)
	}

	local restore_slope = 84
	ASSERT_EQUAL(command_x.slope_get_price(restore_slope), 1500 * 100)

	ASSERT_EQUAL(pl.get_current_cash(),        200000)     // get_current_cash is in credits (returns float)
	ASSERT_EQUAL(pl.get_current_net_wealth(),  200000*100) // get_current_net_wealth is in 1/100 credits
	RESET_ALL_PLAYER_FUNDS()
}


function test_slope_restore_on_foundation()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_house).work(pl, coord3d(4, 2, 0), "11RES_01_23"), null)

	{
		ASSERT_EQUAL(command_x(tool_restoreslope).work(pl, coord3d(4, 2, 0)), "No suitable ground!")
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(4, 2, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_slope_restore_on_bridge()
{
	local pl = player_x(0)
	local rail_bridge = bridge_desc_x.get_available_bridges(wt_rail)[0]

	ASSERT_TRUE(rail_bridge != null)

	ASSERT_EQUAL(command_x.set_slope(pl, coord3d(4, 2, 0), slope.south), null)
	ASSERT_EQUAL(command_x.set_slope(pl, coord3d(4, 4, 0), slope.north), null)
	ASSERT_EQUAL(command_x(tool_build_bridge).work(pl, coord3d(4, 2, 0), rail_bridge.get_name()), null)

	{
		ASSERT_EQUAL(command_x(tool_restoreslope).work(pl, coord3d(4, 2, 0)), "No suitable ground!")
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x.set_slope(pl, coord3d(4, 2, 0), slope.flat), null)
	ASSERT_EQUAL(command_x.set_slope(pl, coord3d(4, 4, 0), slope.flat), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_slope_restore_on_label()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x.set_slope(pl, coord3d(4, 2, 0), slope.south), null)
	ASSERT_EQUAL(label_x.create(coord(4, 2), pl, "foo"), null)

	{
		ASSERT_EQUAL(command_x(tool_restoreslope).work(pl, coord3d(4, 2, 0)), "Tile not empty.")
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x.set_slope(pl, coord3d(4, 2, 0), slope.flat), null)
	RESET_ALL_PLAYER_FUNDS()
}
