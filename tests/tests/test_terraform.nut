//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for terraforming terrain
//


function test_terraform_raise_lower_land()
{
	local raise = command_x(tool_raise_land)
	local lower = command_x(tool_lower_land)

	// TODO check terrain height

	local err = raise.work(player_x(0), coord(3, 2))
	ASSERT_EQUAL(err, "Zu nah am Kartenrand") // TODO Fix error message?

	ASSERT_EQUAL(raise.work(player_x(0), coord3d(3, 2, 100)), null)
	ASSERT_EQUAL(raise.work(player_x(0), coord3d(3, 2, 0)), null)
	ASSERT_EQUAL(raise.work(player_x(0), coord3d(3, 2, 0)), null)

	ASSERT_EQUAL(lower.work(player_x(0), coord3d(3, 2, 0)), null)
	ASSERT_EQUAL(lower.work(player_x(0), coord3d(3, 2, 1)), null)
	ASSERT_EQUAL(lower.work(player_x(0), coord3d(3, 2, 1)), null)
	ASSERT_EQUAL(lower.work(player_x(0), coord3d(3, 2, 1)), null)
	ASSERT_EQUAL(lower.work(player_x(0), coord3d(3, 2, 1)), null)

	// clean up
	ASSERT_EQUAL(raise.work(player_x(0), coord3d(3, 2, 0)), null)
	ASSERT_EQUAL(raise.work(player_x(0), coord3d(3, 2, 0)), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_terraform_raise_lower_land_at_map_border()
{
	local raise = command_x(tool_raise_land)
	local lower = command_x(tool_lower_land)

	// TODO Check terrain height

	ASSERT_EQUAL(raise.work(player_x(0), coord(0, 0)), "Zu nah am Kartenrand")
	ASSERT_EQUAL(raise.work(player_x(0), coord3d(0, 0, 100)), null)
	ASSERT_EQUAL(raise.work(player_x(0), coord3d(0, 0, 0)), null)
	ASSERT_EQUAL(raise.work(player_x(0), coord3d(0, 0, 0)), null)

	ASSERT_EQUAL(lower.work(player_x(0), coord3d(0, 0, 0)), null)
	ASSERT_EQUAL(lower.work(player_x(0), coord3d(0, 0, 1)), null)
	ASSERT_EQUAL(lower.work(player_x(0), coord3d(0, 0, 1)), null)
	ASSERT_EQUAL(lower.work(player_x(0), coord3d(0, 0, 1)), null)
	ASSERT_EQUAL(lower.work(player_x(0), coord3d(0, 0, 1)), null)

	// clean up
	ASSERT_EQUAL(raise.work(player_x(0), coord3d(0, 0, 0)), null)
	ASSERT_EQUAL(raise.work(player_x(0), coord3d(0, 0, 0)), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_terraform_raise_lower_land_at_water_center()
{
	local raise = command_x(tool_raise_land)
	local lower = command_x(tool_lower_land)
	local clim  = command_x(tool_set_climate)

	{
		clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water)

		// raise center piece
		local err = lower.work(player_x(0), coord3d(3, 3, 0))
		ASSERT_EQUAL(err, "")

		ASSERT_TRUE(tile_x(2, 2, 0).is_water())
		ASSERT_TRUE(tile_x(3, 2, 0).is_water())
		ASSERT_TRUE(tile_x(2, 3, 0).is_water())
		ASSERT_TRUE(tile_x(3, 3, 0).is_water())
	}

	{
		clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water)

		// lower center piece
		local err = lower.work(player_x(0), coord3d(3, 3, 0))
		ASSERT_EQUAL(err, "")

		ASSERT_TRUE(tile_x(2, 2, 0).is_water())
		ASSERT_TRUE(tile_x(3, 2, 0).is_water())
		ASSERT_TRUE(tile_x(2, 3, 0).is_water())
		ASSERT_TRUE(tile_x(3, 3, 0).is_water())
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_terraform_raise_lower_land_at_water_corner()
{
	local raise = command_x(tool_raise_land)
	local lower = command_x(tool_lower_land)
	local clim  = command_x(tool_set_climate)

	// FIXME inconsistencies with raising/lowering depending on direction

	// raise north-west corner
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(raise.work(player_x(0), coord3d(2, 2, 0)), "")

		ASSERT_FALSE(tile_x(2, 2, 0).is_water())
		ASSERT_TRUE(tile_x(3, 2, 0).is_water())
		ASSERT_TRUE(tile_x(2, 3, 0).is_water())
		ASSERT_TRUE(tile_x(3, 3, 0).is_water())
	}

	// lower north-west corner
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(lower.work(player_x(0), coord3d(2, 2, 0)), "")

		ASSERT_TRUE(tile_x(2, 2, 0).is_water())
		ASSERT_TRUE(tile_x(3, 2, 0).is_water())
		ASSERT_TRUE(tile_x(2, 3, 0).is_water())
		ASSERT_TRUE(tile_x(3, 3, 0).is_water())
	}


	// raise south-east corner
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(raise.work(player_x(0), coord3d(2, 4, 0)), null)

		ASSERT_TRUE(tile_x(2, 2, 0).is_water())
		ASSERT_FALSE(tile_x(2, 3, 0).is_water())
		ASSERT_TRUE(tile_x(3, 2, 0).is_water())
		ASSERT_TRUE(tile_x(3, 3, 0).is_water())

		ASSERT_EQUAL(lower.work(player_x(0), coord3d(2, 4, 0)), null)
	}

	// lower south-east corner
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(lower.work(player_x(0), coord3d(2, 4, 0)), null)

		ASSERT_FALSE(tile_x(2, 2, 0).is_water())
		ASSERT_FALSE(tile_x(3, 2, 0).is_water())
		ASSERT_FALSE(tile_x(2, 3, 0).is_water())
		ASSERT_FALSE(tile_x(3, 3, 0).is_water())

		ASSERT_EQUAL(raise.work(player_x(0), coord3d(2, 4, 0)), null)
	}

	// raise south-west corner
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(raise.work(player_x(0), coord3d(4, 4, 0)), null)

		ASSERT_TRUE(tile_x(2, 2, 0).is_water())
		ASSERT_TRUE(tile_x(2, 3, 0).is_water())
		ASSERT_TRUE(tile_x(3, 2, 0).is_water())
		ASSERT_FALSE(tile_x(3, 3, 0).is_water())

		ASSERT_EQUAL(lower.work(player_x(0), coord3d(4, 4, 0)), null)
	}

	// lower south-west corner
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(lower.work(player_x(0), coord3d(4, 4, 0)), null)

		ASSERT_TRUE(tile_x(2, 2, 0).is_water())
		ASSERT_FALSE(tile_x(3, 2, 0).is_water())
		ASSERT_FALSE(tile_x(2, 3, 0).is_water())
		ASSERT_FALSE(tile_x(3, 3, 0).is_water())

		ASSERT_EQUAL(raise.work(player_x(0), coord3d(4, 4, 0)), null)
	}

	// raise north-west corner
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(raise.work(player_x(0), coord3d(4, 2, 0)), null)

		ASSERT_TRUE(tile_x(2, 2, 0).is_water())
		ASSERT_FALSE(tile_x(3, 2, 0).is_water())
		ASSERT_TRUE(tile_x(2, 3, 0).is_water())
		ASSERT_TRUE(tile_x(3, 3, 0).is_water())

		ASSERT_EQUAL(lower.work(player_x(0), coord3d(4, 2, 0)), null)
	}

	// lower north-west corner
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(lower.work(player_x(0), coord3d(4, 2, 0)), null)

		ASSERT_FALSE(tile_x(2, 2, 0).is_water())
		ASSERT_FALSE(tile_x(3, 2, 0).is_water())
		ASSERT_FALSE(tile_x(2, 3, 0).is_water())
		ASSERT_FALSE(tile_x(3, 3, 0).is_water())

		ASSERT_EQUAL(raise.work(player_x(0), coord3d(4, 2, 0)), null)
	}

	// clean up
	ASSERT_EQUAL(clim.work(player_x(0), coord3d(3, 2, 0), coord3d(2, 3, 0), "" + cl_mediterran), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_terraform_raise_lower_land_at_water_edge()
{
	local raise = command_x(tool_raise_land)
	local lower = command_x(tool_lower_land)
	local clim  = command_x(tool_set_climate)

	// FIXME inconsistencies with raising/lowering depending on direction

	// raise north edge
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(raise.work(player_x(0), coord3d(3, 2, 0)), "")

		ASSERT_TRUE(tile_x(2, 2, 0).is_water())
		ASSERT_FALSE(tile_x(3, 2, 0).is_water())
		ASSERT_TRUE(tile_x(2, 3, 0).is_water())
		ASSERT_TRUE(tile_x(3, 3, 0).is_water())
	}

	// lower north edge
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(lower.work(player_x(0), coord3d(3, 2, 0)), "")

		ASSERT_TRUE(tile_x(2, 2, 0).is_water())
		ASSERT_TRUE(tile_x(3, 2, 0).is_water())
		ASSERT_TRUE(tile_x(2, 3, 0).is_water())
		ASSERT_TRUE(tile_x(3, 3, 0).is_water())
	}

	// raise east edge
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(raise.work(player_x(0), coord3d(4, 3, 0)), null)

		ASSERT_TRUE(tile_x(2, 2, 0).is_water())
		ASSERT_TRUE(tile_x(2, 3, 0).is_water())
		ASSERT_FALSE(tile_x(3, 2, 0).is_water())
		ASSERT_FALSE(tile_x(3, 3, 0).is_water())

		ASSERT_EQUAL(lower.work(player_x(0), coord3d(4, 3, 0)), null)
	}

	// lower east edge
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(lower.work(player_x(0), coord3d(4, 3, 0)), null)

		ASSERT_TRUE(tile_x(2, 2, 0).is_water())
		ASSERT_TRUE(tile_x(2, 3, 0).is_water())
		ASSERT_FALSE(tile_x(3, 2, 0).is_water())
		ASSERT_FALSE(tile_x(3, 3, 0).is_water())

		ASSERT_EQUAL(raise.work(player_x(0), coord3d(4, 3, 0)), null)
	}

	// raise south edge
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(raise.work(player_x(0), coord3d(3, 4, 0)), null)

		ASSERT_TRUE(tile_x(2, 2, 0).is_water())
		ASSERT_TRUE(tile_x(3, 2, 0).is_water())
		ASSERT_FALSE(tile_x(2, 3, 0).is_water())
		ASSERT_FALSE(tile_x(3, 3, 0).is_water())

		ASSERT_EQUAL(lower.work(player_x(0), coord3d(3, 4, 0)), null)
	}

	// lower south edge
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(lower.work(player_x(0), coord3d(3, 4, 0)), null)

		ASSERT_FALSE(tile_x(2, 2, 0).is_water())
		ASSERT_FALSE(tile_x(3, 2, 0).is_water())
		ASSERT_FALSE(tile_x(2, 3, 0).is_water())
		ASSERT_FALSE(tile_x(3, 3, 0).is_water())

		ASSERT_EQUAL(raise.work(player_x(0), coord3d(3, 4, 0)), null)
	}

	// raise west edge
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(raise.work(player_x(0), coord3d(2, 3, 0)), "")

		ASSERT_TRUE(tile_x(2, 2, 0).is_water())
		ASSERT_TRUE(tile_x(3, 2, 0).is_water())
		ASSERT_FALSE(tile_x(2, 3, 0).is_water())
		ASSERT_TRUE(tile_x(3, 3, 0).is_water())
	}

	// lower west edge
	{
		ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(lower.work(player_x(0), coord3d(2, 3, 0)), "")

		ASSERT_TRUE(tile_x(2, 2, 0).is_water())
		ASSERT_TRUE(tile_x(3, 2, 0).is_water())
		ASSERT_TRUE(tile_x(2, 3, 0).is_water())
		ASSERT_TRUE(tile_x(3, 3, 0).is_water())
	}

	// clean up
	ASSERT_EQUAL(clim.work(player_x(0), coord3d(2, 3, 0), coord3d(3, 2, 0), "" + cl_mediterran), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_terraform_raise_lower_land_below_way()
{
	local pl = player_x(0)
	local road_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local raise = command_x(tool_raise_land)
	local lower = command_x(tool_lower_land)
	local setslope = command_x.set_slope

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), road_desc, true), null)

	// raise below way
	{
		ASSERT_EQUAL(raise.work(pl, coord3d(4, 2, 0)), "Tile not empty.")
		ASSERT_EQUAL(raise.work(pl, coord3d(4, 3, 0)), "Tile not empty.")
		ASSERT_EQUAL(raise.work(pl, coord3d(4, 4, 0)), "Tile not empty.")
		ASSERT_EQUAL(raise.work(pl, coord3d(4, 5, 0)), "Tile not empty.")
		ASSERT_EQUAL(raise.work(pl, coord3d(5, 2, 0)), "Tile not empty.")
		ASSERT_EQUAL(raise.work(pl, coord3d(5, 3, 0)), "Tile not empty.")
		ASSERT_EQUAL(raise.work(pl, coord3d(5, 4, 0)), "Tile not empty.")
		ASSERT_EQUAL(raise.work(pl, coord3d(5, 5, 0)), "Tile not empty.")
	}

	// and lower
	{
		ASSERT_EQUAL(lower.work(pl, coord3d(4, 2, 0)), "Tile not empty.")
		ASSERT_EQUAL(lower.work(pl, coord3d(4, 3, 0)), "Tile not empty.")
		ASSERT_EQUAL(lower.work(pl, coord3d(4, 4, 0)), "Tile not empty.")
		ASSERT_EQUAL(lower.work(pl, coord3d(4, 5, 0)), "Tile not empty.")
		ASSERT_EQUAL(lower.work(pl, coord3d(5, 2, 0)), "Tile not empty.")
		ASSERT_EQUAL(lower.work(pl, coord3d(5, 3, 0)), "Tile not empty.")
		ASSERT_EQUAL(lower.work(pl, coord3d(5, 4, 0)), "Tile not empty.")
		ASSERT_EQUAL(lower.work(pl, coord3d(5, 5, 0)), "Tile not empty.")
	}

	// set slope up
	{
		ASSERT_EQUAL(setslope(pl, coord3d(4, 2, 0),   slope.south), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 2, 0),   slope.south), "")
		ASSERT_EQUAL(setslope(pl, coord3d(4, 2, 0), 2*slope.south), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 2, 0), slope.all_down_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 2, 0), slope.all_down_slope), null)

		ASSERT_EQUAL(setslope(pl, coord3d(4, 4, 0),   slope.north), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 4, 0),   slope.north), "")
		ASSERT_EQUAL(setslope(pl, coord3d(4, 4, 0), 2*slope.north), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 4, 0), slope.all_down_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 4, 0), slope.all_down_slope), null)

		ASSERT_EQUAL(setslope(pl, coord3d(4, 2, 0), slope.all_up_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 2, 0), slope.all_up_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 2, 0), slope.all_down_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 2, 0), slope.all_down_slope), null)

		ASSERT_EQUAL(setslope(pl, coord3d(4, 4, 0), slope.all_up_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 4, 0), slope.all_up_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 4, 0), slope.all_down_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 4, 0), slope.all_down_slope), null)
	}

	// set slope down
	{
		ASSERT_EQUAL(setslope(pl, coord3d(4, 2,  0), slope.all_down_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 2,  0), slope.all_down_slope), "")
		ASSERT_EQUAL(setslope(pl, coord3d(4, 2, -1), slope.all_up_slope), null)

		ASSERT_EQUAL(setslope(pl, coord3d(4, 4,  0), slope.all_down_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 4,  0), slope.all_down_slope), "")
		ASSERT_EQUAL(setslope(pl, coord3d(4, 4, -1), slope.all_up_slope), null)
	}

	// non-dead-end, should fail for all slopes
	{
		for (local sl = slope.flat+1; sl <= slope.all_down_slope; ++sl) {
			if (sl != slope.raised && sl != 81) {
				ASSERT_EQUAL(setslope(pl, coord3d(4, 3, 0), sl), "Tile not empty.")
			}
		}
	}

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


/// Helper function: Raise / lower land along the edge of a rectangular region
function terraform_volcano(pl, pos, size, h)
{
	local raise = h > 0
	local tool = raise ? command_x(tool_raise_land) : command_x(tool_lower_land)

	for (local dz = 0; raise ? dz<h : dz>h; dz += raise?1:-1) {
		for (local i = 0; i<size; ++i) {
			tool.work(pl, pos + coord3d(i,      0,      dz))
			tool.work(pl, pos + coord3d(size,   i,      dz))
			tool.work(pl, pos + coord3d(size-i, size,   dz))
			tool.work(pl, pos + coord3d(0,      size-i, dz))
		}
	}
}

function test_terraform_raise_lower_water_level()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local raise = command_x(tool_raise_land)
	local lower = command_x(tool_lower_land)

	// invalid default_param
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_change_water_height).work(pl, coord3d(0, 0, 0)), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
		ASSERT_TRUE(error_caught)

		error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_change_water_height).work(pl, coord3d(0, 0, 0), ""), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
		ASSERT_TRUE(error_caught)

		error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_change_water_height).work(pl, coord3d(0, 0, 0), "foo"), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
		ASSERT_TRUE(error_caught)

		error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_change_water_height).work(pl, coord3d(0, 0, 0), ".5"), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
		ASSERT_TRUE(error_caught)
	}

	// invalid pos
	{
		ASSERT_EQUAL(command_x(tool_change_water_height).work(pl, coord3d(-1, -1, -1), "0"), "Cannot alter water")
	}

	// near map border
	{
		ASSERT_EQUAL(command_x(tool_change_water_height).work(pl, coord3d(5, 5, 0), "0"), "Cannot alter water")
		ASSERT_EQUAL(command_x(tool_change_water_height).work(pl, coord3d(5, 5, 0), "0"), "Cannot alter water")
	}

	terraform_volcano(public_pl, coord3d(5, 5, 0), 3, 1)

	// lower water level on ground, should fail
	{
		local chg_water = command_x(tool_change_water_height)
		ASSERT_EQUAL(chg_water.work(pl, coord3d(6, 6, 0), "0"), "Cannot alter water")
		chg_water.set_flags(2)

		ASSERT_EQUAL(chg_water.work(pl, coord3d(6, 6, 0), "0"), "Cannot alter water")
		chg_water.set_flags(2)
		ASSERT_EQUAL(chg_water.work(pl, coord3d(6, 6, 0), "0"), "Cannot alter water")
	}

	// raise water level on ground, should make shallow water first (i.e. tile with cl_water)
	{
		local old_cash = pl.get_current_cash()
		local chg_water = command_x(tool_change_water_height)

		ASSERT_EQUAL(chg_water.work(pl, coord3d(6, 6, 0), "1"), null)
		ASSERT_EQUAL(square_x(6, 6).get_climate(), cl_water)
		ASSERT_EQUAL(pl.get_current_cash(), old_cash)

		ASSERT_EQUAL(chg_water.work(pl, coord3d(6, 6, 0), "1"), "Cannot alter water")
		ASSERT_EQUAL(pl.get_current_cash(), old_cash)

		ASSERT_EQUAL(chg_water.work(pl, coord3d(6, 6, 0), "0"), null)
		ASSERT_EQUAL(square_x(6, 6).get_climate(), cl_mediterran)
	}

	// make double height volcano
	terraform_volcano(public_pl, coord3d(5, 5, 0), 3, 1)

	{
		local chg_water = command_x(tool_change_water_height)
		ASSERT_EQUAL(chg_water.work(pl, coord3d(6, 6, 0), "1"), null)
		ASSERT_EQUAL(chg_water.work(pl, coord3d(6, 6, 0), "1"), null)
		ASSERT_EQUAL(tile_x(5, 6, 1).get_slope(), slope.east)

		ASSERT_EQUAL(square_x(6, 6).get_climate(), cl_water)
		ASSERT_TRUE(square_x(6, 6).get_tile_at_height(1) != null)

		ASSERT_EQUAL(chg_water.work(pl, coord3d(6, 6, 1), "0"), null)
		ASSERT_EQUAL(tile_x(5, 6, 0).get_slope(), 2*slope.east) // make sure single height slopes are changed to double again

		ASSERT_EQUAL(chg_water.work(pl, coord3d(6, 6, 0), "0"), null)
	}

	terraform_volcano(public_pl, coord3d(5, 5, 0), 3, -2)

	// make 1-high barrier
	terraform_volcano(public_pl, coord3d(2, 2, 0), 7, 2)
	terraform_volcano(public_pl, coord3d(5, 5, 0), 1, 1)
	terraform_volcano(public_pl, coord3d(3, 5, 0), 1, 1)
	terraform_volcano(public_pl, coord3d(7, 5, 0), 1, 1)

	//
	{
		local chg_water = command_x(tool_change_water_height)
		chg_water.set_flags(2) // ctrl

		ASSERT_EQUAL(chg_water.work(pl, coord3d(5, 3, 0), "1"), null)
		ASSERT_EQUAL(square_x(4, 3).get_climate(), cl_mediterran)
		ASSERT_EQUAL(square_x(5, 3).get_climate(), cl_water)
		ASSERT_EQUAL(square_x(6, 3).get_climate(), cl_mediterran)

		// FIXME this gives different water on the center tile vs other tiles?
		ASSERT_EQUAL(chg_water.work(pl, coord3d(5, 3, 0), "1"), null)
		ASSERT_EQUAL(square_x(3, 4).get_climate(), cl_water)
		ASSERT_EQUAL(square_x(3, 5).get_climate(), cl_mediterran)

		// cannot drain with ctrl active
		ASSERT_EQUAL(chg_water.work(pl, coord3d(5, 4, 1), "0"), "Cannot alter water")
		chg_water.set_flags(0)
		ASSERT_EQUAL(chg_water.work(pl, coord3d(5, 4, 1), "0"), null)

		ASSERT_EQUAL(chg_water.work(pl, coord3d(5, 3, 0), "1"), null)
		ASSERT_EQUAL(square_x(3, 5).get_climate(), cl_water)
		ASSERT_EQUAL(square_x(7, 5).get_climate(), cl_water)
		ASSERT_EQUAL(square_x(3, 7).get_climate(), cl_water)
		ASSERT_EQUAL(square_x(7, 7).get_climate(), cl_water)

		// Using tool on very shallow water should give 1-tile islands
		chg_water.set_flags(2)
		ASSERT_EQUAL(chg_water.work(pl, coord3d(5, 5, 1), "0"), null)
		ASSERT_EQUAL(square_x(5, 5).get_climate(), cl_mediterran)
		ASSERT_EQUAL(square_x(4, 5).get_climate(), cl_mediterran)
		ASSERT_EQUAL(square_x(6, 5).get_climate(), cl_mediterran)
		ASSERT_EQUAL(square_x(5, 4).get_climate(), cl_water)
		ASSERT_EQUAL(square_x(5, 6).get_climate(), cl_water)

		ASSERT_EQUAL(chg_water.work(pl, coord3d(5, 5, 1), "1"), null)

		// draining without ctrl should leave 2 small basins
		chg_water.set_flags(0)
		ASSERT_EQUAL(chg_water.work(pl, coord3d(5, 5, 1), "0"), null)
		ASSERT_EQUAL(square_x(5, 3).get_climate(), cl_water)
		ASSERT_EQUAL(square_x(5, 4).get_climate(), cl_mediterran)
		ASSERT_EQUAL(square_x(5, 6).get_climate(), cl_mediterran)
		ASSERT_EQUAL(square_x(5, 7).get_climate(), cl_water)

		ASSERT_EQUAL(chg_water.work(pl, coord3d(5, 7, 0), "0"), null)
		ASSERT_EQUAL(chg_water.work(pl, coord3d(5, 3, 0), "0"), null)
	}

	terraform_volcano(public_pl, coord3d(5, 5, 1), 1, 1)
	terraform_volcano(public_pl, coord3d(3, 5, 1), 1, 1)
	terraform_volcano(public_pl, coord3d(7, 5, 1), 1, 1)


	// water also spills across tile edges and diagonals if low enough
	{
		local chg_water = command_x(tool_change_water_height)

		command_x(tool_lower_land).work(pl, coord3d(5, 5, 2))
		command_x(tool_lower_land).work(pl, coord3d(6, 6, 2))

		ASSERT_EQUAL(chg_water.work(pl, coord3d(5, 7, 0), "1"), null)
		ASSERT_EQUAL(chg_water.work(pl, coord3d(5, 7, 1), "1"), null)

		ASSERT_EQUAL(square_x(6, 3).get_climate(), cl_water)

		// note: This also drains the other basin 1 level
		ASSERT_EQUAL(chg_water.work(pl, coord3d(5, 7, 1), "0"), null)
		ASSERT_EQUAL(square_x(5, 3).get_tile_at_height(1), null)
		ASSERT_EQUAL(chg_water.work(pl, coord3d(5, 7, 0), "0"), null)
		ASSERT_EQUAL(chg_water.work(pl, coord3d(5, 3, 0), "0"), null)

		command_x(tool_raise_land).work(pl, coord3d(5, 5, 1))
		command_x(tool_raise_land).work(pl, coord3d(6, 6, 1))
	}

	// clean up

	terraform_volcano(public_pl, coord3d(2, 2, 2), 7, -2)
	terraform_volcano(public_pl, coord3d(5, 5, 2), 1, -2)
	terraform_volcano(public_pl, coord3d(3, 5, 2), 1, -2)
	terraform_volcano(public_pl, coord3d(7, 5, 2), 1, -2)

	RESET_ALL_PLAYER_FUNDS()
}
