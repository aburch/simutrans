//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// test tools for setting climate
//


function test_climate_invalid()
{
	local pl = player_x(0)
	local setclimate = command_x(tool_set_climate)
	local pos = coord3d(3, 3, 0)

	ASSERT_EQUAL(setclimate.work(pl, pos, pos, null), "")
	ASSERT_EQUAL(setclimate.work(pl, pos, pos, ""), "")
	ASSERT_EQUAL(setclimate.work(pl, pos, pos, ","), "")
	ASSERT_EQUAL(setclimate.work(pl, pos, pos, "42"), "")
	ASSERT_EQUAL(setclimate.work(pl, coord3d(-1, -1, 0), coord3d(-1, -1, 0), "" + cl_water), null)

	local error_caught = false
	try {
		ASSERT_TRUE(command_x.change_climate_at(pl, pos, -1) != null)
	}
	catch (e) {
		error_caught = true
		ASSERT_EQUAL(e, "Invalid climate number provided")
	}
	ASSERT_TRUE(error_caught)
}


function test_climate_flat()
{
	local pl = player_x(0)
	local pos = coord3d(3, 2, 0)
	local setclimate = command_x(tool_set_climate)
	local climates = [ cl_water, cl_desert, cl_tropic, cl_mediterran, cl_temperate, cl_tundra, cl_rocky, cl_arctic ]

	// single tile
	{
		foreach (cl in climates) {
			ASSERT_EQUAL(command_x.change_climate_at(pl, pos, cl), null)
			ASSERT_EQUAL(square_x(pos.x, pos.y).get_climate(), cl)
		}
	}

	// multi-tile
	{
		// not implemented in change_climate_at
		// as multi-tile climate change is forbidden in multiplayer games
		foreach (cl in climates) {
			ASSERT_EQUAL(setclimate.work(pl, pos, pos + coord3d(2, 2, 0), "" + cl), null)
			ASSERT_EQUAL(square_x(pos.x, pos.y).get_climate(), cl)
		}
	}

	// multi-tile inverted
	{
		foreach (cl in climates) {
			ASSERT_EQUAL(setclimate.work(pl, pos, pos - coord3d(2, 2, 0), "" + cl), null)
			ASSERT_EQUAL(square_x(pos.x, pos.y).get_climate(), cl)
		}
	}

	// invalid second pos, same as single-tile setclimate
	{
		foreach (cl in climates) {
			ASSERT_EQUAL(setclimate.work(pl, pos, coord3d(-1, -1, -1), "" + cl), null)
			ASSERT_EQUAL(square_x(pos.x, pos.y).get_climate(), cl)
		}
	}

	ASSERT_EQUAL(command_x.change_climate_at(pl, pos, cl_mediterran), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_climate_cliff()
{
	local pl = player_x(0)
	local setslope = command_x(tool_setslope)
	local pos = coord3d(3, 2, 0)
	local setclimate = command_x(tool_set_climate)
	local climates = [ cl_water, cl_desert, cl_tropic, cl_mediterran, cl_temperate, cl_tundra, cl_rocky, cl_arctic ]

	for (local h = 0; h < 4; ++h) {
		for (local y = 0; y < 3; ++y) {
			for (local x = 0; x < 3; ++x) {
				ASSERT_EQUAL(setslope.work(pl, pos + coord3d(x, y, h), "" + slope.all_up_slope), null)
			}
		}
	}

	RESET_ALL_PLAYER_FUNDS()

	// set climate not next to cliff
	{
		foreach (cl in climates) {
			ASSERT_EQUAL(setclimate.work(pl, pos + coord3d(1, 1, 4), pos + coord3d(1, 1, 4), "" + cl), null)
		}

		ASSERT_EQUAL(setclimate.work(pl, pos + coord3d(1, 1, 4), pos + coord3d(1, 1, 4), "" + cl_mediterran), null)
	}

	// set climate next to cliff
	{
		foreach (cl in climates) {
			ASSERT_EQUAL(setclimate.work(pl, pos + coord3d(0, 0, 4), pos + coord3d(2, 2, 4), "" + cl), null)

			if (cl == cl_water) {
				// cannot build water next to cliff edge
				for (local y = 0; y < 3; ++y) {
					for (local x = 0; x < 3; ++x) {
						ASSERT_EQUAL(tile_x(pos.x + x, pos.y + y, 4).is_water(), x==1 && y ==1)
					}
				}
			}
		}

		ASSERT_EQUAL(setclimate.work(pl, pos + coord3d(0, 0, 4), pos + coord3d(2, 2, 4), "" + cl_mediterran), null)
	}

	// clean up
	RESET_ALL_PLAYER_FUNDS()

	for (local h = 4; h > 0; --h) {
		for (local y = 0; y < 3; ++y) {
			for (local x = 0; x < 3; ++x) {
				ASSERT_EQUAL(setslope.work(pl, pos + coord3d(x, y, h), "" + slope.all_down_slope), null)
			}
		}
	}

	ASSERT_EQUAL(setclimate.work(pl, coord3d(0, 0, 0), coord3d(15, 15, 0), "" + cl_mediterran), null)
	RESET_ALL_PLAYER_FUNDS()
}

