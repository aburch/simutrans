//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for scenario rules/conditions
//

function test_scenario_rules_allow_forbid_tool()
{
	local pl = player_x(0)

	{
		ASSERT_EQUAL(command_x.grid_raise(pl, coord3d(4, 2, 0)), null) // FIXME this should fail
		ASSERT_EQUAL(command_x.grid_lower(pl, coord3d(4, 2, 1)), null)
	}

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_scenario_rules_allow_forbid_way_tool()
{
	local waybuilder = command_x(tool_build_way)
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local pl = player_x(0)

	rules.forbid_way_tool_rect(0, tool_build_way, wt_road, road.get_name(), coord(2, 2), coord(5, 5), "Foo Bar")

	// Fully in forbiden zone
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 2, 0), coord3d(5, 5, 0), road, true), "")
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])
	}

	// Ending in forbidden zone
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(0, 2, 0), coord3d(2, 2, 0), road, true), "")
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])
	}

	// Starting in forbidden zone
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 2, 0), coord3d(0, 2, 0), road, true), "")
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])
	}

	// make sure we can build other ways
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 2, 0), coord3d(0, 2, 0), rail, true), null)
		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"2A8.....",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(2, 2, 0), coord3d(0, 2, 0), "" + wt_rail), null)
	}

	// clean up
	rules.clear()
	RESET_ALL_PLAYER_FUNDS()
}


function test_scenario_rules_allow_forbid_way_tool_rect()
{
	local waybuilder = command_x(tool_build_way)
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local pl = player_x(0)

	rules.forbid_way_tool_rect(0, tool_build_way, wt_road, "", coord(2, 2), coord(5, 5), "Foo Bar")

	// Fully in forbiden zone
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 2, 0), coord3d(5, 5, 0), road, true), "")
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])
	}

	// Ending in forbidden zone
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(0, 2, 0), coord3d(2, 2, 0), road, true), "")
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])
	}

	// Starting in forbidden zone
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 2, 0), coord3d(0, 2, 0), road, true), "")
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])
	}

	// make sure we can build other ways
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 2, 0), coord3d(0, 2, 0), rail, true), null)
		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"2A8.....",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(2, 2, 0), coord3d(0, 2, 0), "" + wt_rail), null)
	}

	// clean up
	rules.clear()
	RESET_ALL_PLAYER_FUNDS()
}


function test_scenario_rules_allow_forbid_way_tool_cube()
{
	local waybuilder = command_x(tool_build_way)
	local setslope = command_x.set_slope
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local pl = player_x(0)

	rules.forbid_way_tool_cube(0, tool_build_way, wt_road, "", coord3d(2, 2, 1), coord3d(5, 5, 2), "Foo Bar")

	// build below
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 2, 0), coord3d(0, 2, 0), road, true), null)
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"2A8.....",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(2, 2, 0), coord3d(0, 2, 0), "" + wt_road), null)
	}

	// build into forbidden zone
	{
		ASSERT_EQUAL(setslope(pl, coord3d(3, 4, 0), slope.all_up_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(3, 3, 0), slope.north), null)

		ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 0, 0), coord3d(3, 4, 1), road, true), "")
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(setslope(pl, coord3d(3, 4, 1), slope.all_down_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(3, 3, 0), slope.flat), null)
	}

	rules.clear()
	rules.forbid_way_tool_cube(0, tool_build_way, wt_road, "", coord3d(0, 0, 1), coord3d(0, 0, 1), "Foo Bar")

	// build double height slope through forbidden cube
	{
		ASSERT_EQUAL(setslope(pl, coord3d(1, 1, 0), (2*slope.east)), null)
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 1, 0), coord3d(1, 1, 0), road, true), null)
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				".28.....",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(2, 1, 0), coord3d(1, 1, 0), "" + wt_road), null)
		ASSERT_EQUAL(setslope(pl, coord3d(1, 1, 0), slope.flat), null)
	}

	// clean up
	rules.clear()
	RESET_ALL_PLAYER_FUNDS()
}


function test_scenario_rules_allow_forbid_tool_stacked_rect()
{
	local pl = player_x(0)
	local waybuilder = command_x(tool_build_way)
	local setslope = command_x.set_slope
	local road_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0]

	rules.forbid_way_tool_rect(0, tool_build_way, wt_road, "", coord(1, 1), coord(14, 14), "Foo Bar 1")

	// build in outer allowed ring, near map border
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 0, 0), coord3d(0, 5, 0), road_desc, false), null)
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"6AAAA8..",
				"5.......",
				"5.......",
				"5.......",
				"5.......",
				"1.......",
				"........",
				"........"
			])
	}

	// build in outer forbidden ring
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 1, 0), coord3d(1, 5, 0), road_desc, false), "")
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"6AAAA8..",
				"5.......",
				"5.......",
				"5.......",
				"5.......",
				"1.......",
				"........",
				"........"
			])
	}

	rules.allow_way_tool_rect(0, tool_build_way, wt_road, "", coord(2, 2), coord(13, 13))

	// try building in allowed ring, does work since introdcued allowed rules
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 2, 0), coord3d(2, 5, 0), road_desc, false), null)
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"6AAAA8..",
				"5.......",
				"5..6A8..",
				"5.69....",
				"5.5.....",
				"1.1.....",
				"........",
				"........"
			])
	}

	// try building from inside allowed ring to forbidden rect, must fail
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 1, 0), coord3d(2, 5, 0), road_desc, false), "")
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"6AAAA8..",
				"5.......",
				"5..6A8..",
				"5.69....",
				"5.5.....",
				"1.1.....",
				"........",
				"........"
			])
	}

	// try building from inside allowed ring to outside, must fail => we cannot cross border
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(0, 0, 0), coord3d(2, 5, 0), road_desc, false), "")
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"6AAAA8..",
				"5.......",
				"5..6A8..",
				"5.69....",
				"5.5.....",
				"1.1.....",
				"........",
				"........"
			])
	}

	rules.clear()

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(5, 0, 0), coord3d(0, 5, 0), "" + wt_road), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(5, 2, 0), coord3d(2, 5, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_scenario_rules_allow_forbid_tool_stacked_cube()
{
	local pl = player_x(0)
	local waybuilder = command_x(tool_build_way)
	local setslope = command_x.set_slope
	local road_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0]

	rules.forbid_way_tool_cube(0, tool_build_way, wt_road, "", coord3d(1, 1, 0), coord3d(14, 14, 0), "Foo Bar 1")

	// build in outer allowed ring, near map border
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 0, 0), coord3d(0, 5, 0), road_desc, false), null)
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"6AAAA8..",
				"5.......",
				"5.......",
				"5.......",
				"5.......",
				"1.......",
				"........",
				"........"
			])
	}

	// build in outer forbidden ring
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 1, 0), coord3d(1, 5, 0), road_desc, false), "")
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"6AAAA8..",
				"5.......",
				"5.......",
				"5.......",
				"5.......",
				"1.......",
				"........",
				"........"
			])
	}

	rules.allow_way_tool_rect(0, tool_build_way, wt_road, "", coord(2, 2), coord(13, 13))

	// try building in allowed ring, does work since introdcued allowed rules
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 2, 0), coord3d(2, 5, 0), road_desc, false), null)
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"6AAAA8..",
				"5.......",
				"5..6A8..",
				"5.69....",
				"5.5.....",
				"1.1.....",
				"........",
				"........"
			])
	}

	// try building from inside allowed ring to forbidden rect, must fail
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 1, 0), coord3d(2, 5, 0), road_desc, false), "")
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"6AAAA8..",
				"5.......",
				"5..6A8..",
				"5.69....",
				"5.5.....",
				"1.1.....",
				"........",
				"........"
			])
	}

	// try building from inside allowed ring to outside, must fail => we cannot cross border
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(0, 0, 0), coord3d(2, 5, 0), road_desc, false), "")
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"6AAAA8..",
				"5.......",
				"5..6A8..",
				"5.69....",
				"5.5.....",
				"1.1.....",
				"........",
				"........"
			])
	}

	rules.clear()

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(5, 0, 0), coord3d(0, 5, 0), "" + wt_road), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(5, 2, 0), coord3d(2, 5, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}
