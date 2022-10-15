//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for bridge building / removal
//


function test_way_bridge_build_ground()
{
	local pl          = player_x(0)
	local bridge_desc = bridge_desc_x.get_available_bridges(wt_road)[0]
	local remover     = command_x(tool_remove_way)
	ASSERT_TRUE(bridge_desc != null)

	// build bridge on flat ground
	ASSERT_EQUAL(command_x.build_bridge(pl, coord3d(3, 5, 0), coord3d(3, 2, 0), bridge_desc), null)

	ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
		[
			"........",
			"...4....",
			"...5....",
			"...0....",
			"...0....",
			"...5....",
			"...1....",
			"........"
		])

	ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
		[
			"........",
			"...4....",
			"...5....",
			"...5....",
			"...5....",
			"...5....",
			"...1....",
			"........"
		])

	// build bridge over bridgehead (should fail)
	ASSERT_EQUAL(command_x.build_bridge(pl, coord3d(2, 2, 0), coord3d(4, 2, 0), bridge_desc), "")
	ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
		[
			"........",
			"...4....",
			"...5....",
			"...5....",
			"...5....",
			"...5....",
			"...1....",
			"........"
		])

	// build bridgehead slope under bridge (should fail)
	ASSERT_EQUAL(command_x.build_bridge(pl, coord3d(3, 3, 0), coord3d(5, 3, 0), bridge_desc), "")
	ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
		[
			"........",
			"...4....",
			"...5....",
			"...5....",
			"...5....",
			"...5....",
			"...1....",
			"........"
		])

	// build bridge crossing (should fail)
	ASSERT_EQUAL(command_x.build_bridge(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), bridge_desc), "")
	ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
		[
			"........",
			"...4....",
			"...5....",
			"...5....",
			"...5....",
			"...5....",
			"...1....",
			"........"
		])

	ASSERT_EQUAL(remover.work(pl, tile_x(3, 1, 0), tile_x(3, 6, 0), "" + wt_road), null)
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


function test_way_bridge_build_at_slope()
{
	local start_pos = coord3d(2, 1, 0)
	local end_pos = coord3d(2, 6, 0)
	local remover = command_x(tool_remove_way)
	local setslope = command_x.set_slope
	local pl = player_x(0)
	local bridge_desc = bridge_desc_x.get_available_bridges(wt_road)[0]

	ASSERT_EQUAL(setslope(pl, coord3d(2, 1, 0), slope.south), null)

	{
		// down slope
		local err = command_x.build_bridge_at(pl, start_pos, bridge_desc)
		ASSERT_EQUAL(err, "Bridge is too long for this type!\n")

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
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

	{
		// wrong single-height slope
		ASSERT_EQUAL(setslope(pl, end_pos, slope.west), null)
		local err = command_x.build_bridge_at(pl, start_pos, bridge_desc)
		ASSERT_EQUAL(err, "A bridge must start on a way!")

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
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

	{
		// double height slope
		ASSERT_EQUAL(setslope(pl, end_pos, 2*slope.north), null)
		local err = command_x.build_bridge_at(pl, start_pos, bridge_desc)
		ASSERT_EQUAL(err, "Cannot connect to the\ncenter of a double slope!")

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
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

	{
		// wrong double height slope
		ASSERT_EQUAL(setslope(pl, end_pos, 2*slope.west), null)
		local err = command_x.build_bridge_at(pl, start_pos, bridge_desc)
		ASSERT_EQUAL(err, "Cannot connect to the\ncenter of a double slope!")

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
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

	{
		// flat double height slope (i.e. 2 stacked flat slopes)
		ASSERT_EQUAL(setslope(pl, end_pos, slope.all_up_slope), null)
		ASSERT_EQUAL(setslope(pl, end_pos + coord3d(0, 0, 1), slope.all_up_slope), null)
		local err = command_x.build_bridge_at(pl, start_pos, bridge_desc)
		ASSERT_EQUAL(err, "Cannot connect to the\ncenter of a double slope!")

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
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

		ASSERT_EQUAL(setslope(pl, end_pos + coord3d(0, 0, 2), slope.all_down_slope), null)
		ASSERT_EQUAL(setslope(pl, end_pos + coord3d(0, 0, 1), slope.all_down_slope), null)
	}

	{
		// flat single height slope
		ASSERT_EQUAL(setslope(pl, end_pos, slope.all_up_slope), null)
		local err = command_x.build_bridge_at(pl, start_pos, bridge_desc)
		ASSERT_EQUAL(err, null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
			[
				"........",
				"..4.....",
				"..5.....",
				"..5.....",
				"..5.....",
				"..5.....",
				"..1.....",
				"........"
			])

		// remove way
		ASSERT_EQUAL(remover.work(pl, start_pos, end_pos + coord3d(0, 0, 1), "" + wt_road), null)

		// remove slope
		ASSERT_EQUAL(setslope(pl, end_pos + coord3d(0, 0, 1), slope.all_down_slope), null)
	}

	{
		// correct slope
		setslope(pl, end_pos, slope.north)
		local err = command_x.build_bridge_at(pl, start_pos, bridge_desc)
		ASSERT_EQUAL(err, null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
			[
				"........",
				"..4.....",
				"..5.....",
				"..5.....",
				"..5.....",
				"..5.....",
				"..1.....",
				"........"
			])

		// remove way
		ASSERT_EQUAL(remover.work(pl, start_pos, end_pos, "" + wt_road), null)

		// remove slope
		ASSERT_EQUAL(setslope(pl, end_pos, slope.flat), null)
	}

	// clean up
	ASSERT_EQUAL(setslope(pl, start_pos, slope.flat), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_way_bridge_build_at_slope_stacked()
{
	local remover = command_x(tool_remove_way)
	local setslope = command_x.set_slope
	local pl = player_x(0)
	local bridge_desc = bridge_desc_x.get_available_bridges(wt_road)[0]

	{
		ASSERT_EQUAL(setslope(pl, coord3d(3, 2, 0), slope.south), null)
		ASSERT_EQUAL(setslope(pl, coord3d(3, 5, 0), slope.north), null)

		ASSERT_EQUAL(command_x.build_bridge_at(pl, coord3d(3, 2, 0), bridge_desc), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
			[
				"........",
				"........",
				"...4....",
				"...5....",
				"...5....",
				"...1....",
				"........",
				"........"
			])


		// second bridge layer
		ASSERT_EQUAL(setslope(pl, coord3d(3, 1, 0), slope.all_up_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(3, 1, 1), slope.south), null)
		ASSERT_EQUAL(setslope(pl, coord3d(3, 6, 0), slope.all_up_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(3, 6, 1), slope.north), null)

		ASSERT_EQUAL(command_x.build_bridge_at(pl, coord3d(3, 1, 1), bridge_desc), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
			[
				"........",
				"...4....",
				"...4....",
				"...5....",
				"...5....",
				"...1....",
				"...1....",
				"........"
			])

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 2),
			[
				"........",
				"...4....",
				"...5....",
				"...5....",
				"...5....",
				"...5....",
				"...1....",
				"........"
			])
	}

	{
		// remove lower bridge and rebuild it
		ASSERT_EQUAL(remover.work(pl, coord3d(3, 2, 0), coord3d(3, 5, 0), "" + wt_road), null)

		ASSERT_EQUAL(command_x.build_bridge_at(pl, coord3d(3, 2, 0), bridge_desc), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
			[
				"........",
				"...4....",
				"...4....",
				"...5....",
				"...5....",
				"...1....",
				"...1....",
				"........"
			])

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 2),
			[
				"........",
				"...4....",
				"...5....",
				"...5....",
				"...5....",
				"...5....",
				"...1....",
				"........"
			])

		ASSERT_EQUAL(remover.work(pl, coord3d(3, 1, 1), coord3d(3, 6, 1), "" + wt_road), null)
		ASSERT_EQUAL(setslope(pl, coord3d(3, 1, 1), slope.flat), null)
		ASSERT_EQUAL(setslope(pl, coord3d(3, 6, 1), slope.flat), null)
		ASSERT_EQUAL(setslope(pl, coord3d(3, 1, 1), slope.all_down_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(3, 6, 1), slope.all_down_slope), null)
	}

	{
		// second bridge layer
		ASSERT_EQUAL(setslope(pl, coord3d(1, 3, 0), slope.all_up_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(1, 3, 1), slope.east), null)
		ASSERT_EQUAL(setslope(pl, coord3d(6, 3, 0), slope.all_up_slope), null)
		ASSERT_EQUAL(setslope(pl, coord3d(6, 3, 1), slope.west), null)

		ASSERT_EQUAL(command_x.build_bridge_at(pl, coord3d(1, 3, 1), bridge_desc), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
			[
				"........",
				"........",
				"...4....",
				".2.5..8.",
				"...5....",
				"...1....",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 2),
			[
				"........",
				"........",
				"...4....",
				".2AAAA8.",
				"........",
				"...1....",
				"........",
				"........"
			])
	}

	{
		// remove lower bridge and rebuild it
		ASSERT_EQUAL(remover.work(pl, coord3d(3, 5, 0), coord3d(3, 2, 0), "" + wt_road), null)
		ASSERT_EQUAL(command_x.build_bridge_at(pl, coord3d(3, 5, 0), bridge_desc), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
			[
				"........",
				"........",
				"...4....",
				".2.5..8.",
				"...5....",
				"...1....",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 2),
			[
				"........",
				"........",
				"...4....",
				".2AAAA8.",
				"........",
				"...1....",
				"........",
				"........"
			])

	}

	// clean up
	ASSERT_EQUAL(remover.work(pl, coord3d(3, 2, 0), coord3d(3, 5, 0), "" + wt_road), null)
	ASSERT_EQUAL(remover.work(pl, coord3d(1, 3, 1), coord3d(6, 3, 1), "" + wt_road), null)
	ASSERT_EQUAL(setslope(pl, coord3d(1, 3, 1), slope.all_down_slope), null)
	ASSERT_EQUAL(setslope(pl, coord3d(1, 3, 1), slope.all_down_slope), null)
	ASSERT_EQUAL(setslope(pl, coord3d(6, 3, 1), slope.all_down_slope), null)
	ASSERT_EQUAL(setslope(pl, coord3d(6, 3, 1), slope.all_down_slope), null)
	ASSERT_EQUAL(setslope(pl, coord3d(3, 2, 0), slope.all_down_slope), null)
	ASSERT_EQUAL(setslope(pl, coord3d(3, 5, 0), slope.all_down_slope), null)

	RESET_ALL_PLAYER_FUNDS()
}


// TODO Try to build bridge when way_height_clearance == 2
function test_way_bridge_build_above_way()
{
	local remover = command_x(tool_remove_way)
	local setslope = command_x.set_slope
	local pl = player_x(0)
	local bridge_desc = bridge_desc_x.get_available_bridges(wt_road)[0]
	local way_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0]

	ASSERT_TRUE(bridge_desc != null)
	ASSERT_TRUE(way_desc != null)

	{
		ASSERT_EQUAL(setslope(pl, coord3d(3, 2, 0), slope.south), null)
		ASSERT_EQUAL(setslope(pl, coord3d(3, 5, 0), slope.north), null)

		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 3, 0), coord3d(4, 3, 0), way_desc, true), null)
		ASSERT_EQUAL(command_x.build_bridge_at(pl, coord3d(3, 2, 0), bridge_desc), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"...4....",
				"..2A8...",
				"........",
				"...1....",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
			[
				"........",
				"........",
				"...4....",
				"...5....",
				"...5....",
				"...1....",
				"........",
				"........"
			])
	}

	{
		// remove way, rebuild it under bridge
		ASSERT_EQUAL(remover.work(pl, coord3d(2, 3, 0), coord3d(4, 3, 0), "" + wt_road), null)
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 3, 0), coord3d(4, 3, 0), way_desc, true), null)
	}

	ASSERT_EQUAL(remover.work(pl, coord3d(2, 3, 0), coord3d(4, 3, 0), "" + wt_road), null)
	ASSERT_EQUAL(remover.work(pl, coord3d(3, 2, 0), coord3d(3, 5, 0), "" + wt_road), null)
	ASSERT_EQUAL(setslope(pl, coord3d(3, 2, 0), slope.all_down_slope), null)
	ASSERT_EQUAL(setslope(pl, coord3d(3, 5, 0), slope.all_down_slope), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_way_bridge_build_above_runway()
{
	local pl = player_x(0)
	local wayremover = command_x(tool_remove_way)
	local taxiway = way_desc_x.get_available_ways(wt_air, st_flat)[0]
	local runway = way_desc_x.get_available_ways(wt_air, st_elevated)[0]
	local bridge = bridge_desc_x.get_available_bridges(wt_road)[0]
	local setslope = command_x.set_slope

	// preconditions

	// build bridge across taxiway
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(7, 7, 0), coord3d(7, 9, 0), taxiway, true), null)
		ASSERT_EQUAL(command_x.build_bridge(pl, coord3d(6, 8, 0), coord3d(8, 8, 0), bridge), null)

		ASSERT_WAY_PATTERN(wt_air, coord3d(5, 5, 0),
			[
				"........",
				"........",
				"..4.....",
				"..5.....",
				"..1.....",
				"........",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN(wt_road, coord3d(5, 5, 0),
			[
				"........",
				"........",
				"........",
				"2A.A8...",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(wayremover.work(pl, coord3d(7, 7, 0), coord3d(7, 9, 0), "" + wt_air), null)
		ASSERT_EQUAL(wayremover.work(pl, coord3d(5, 8, 0), coord3d(9, 8, 0), "" + wt_road), null)
	}

	// build bridge across runway, should fail
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(7, 7, 0), coord3d(7, 9, 0), runway, true), null)
		ASSERT_EQUAL(command_x.build_bridge(pl, coord3d(6, 8, 0), coord3d(8, 8, 0), bridge), "")

		ASSERT_WAY_PATTERN(wt_air, coord3d(5, 5, 0),
			[
				"........",
				"........",
				"..4.....",
				"..5.....",
				"..1.....",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(wayremover.work(pl, coord3d(7, 7, 0), coord3d(7, 9, 0), "" + wt_air), null)
	}

	// clean up
	ASSERT_EQUAL(pl.get_current_maintenance(), 0)
	RESET_ALL_PLAYER_FUNDS()
}


function test_way_bridge_planner()
{
	local pl = player_x(0)
	local start_pos = coord3d(2, 3, 0)
	local end_pos = coord3d(2, 6, 0)
	local bridge_desc = bridge_desc_x.get_available_bridges(wt_road)[0]

	local working_slopes = [ slope.north ] // maybe also slope.south in the future

	ASSERT_EQUAL(command_x.set_slope(pl, start_pos, slope.south), null)

	{
		for (local sl = slope.flat; sl < slope.raised; ++sl) {
			command_x.set_slope(pl, end_pos, sl)

			local end = bridge_planner_x.find_end(pl, start_pos, dir.south, bridge_desc, 0)
			local expected_end = (working_slopes.find(sl) != null) ? end_pos : coord3d(-1, -1, -1)

			ASSERT_EQUAL(end.tostring(), expected_end.tostring())
		}

		ASSERT_EQUAL(command_x.set_slope(pl, end_pos, slope.all_up_slope), null)
		local end = bridge_planner_x.find_end(pl, start_pos, dir.south, bridge_desc, 0)
		ASSERT_EQUAL(end.tostring(), (end_pos + coord3d(0, 0, 1)).tostring())

		// clean up
		ASSERT_EQUAL(command_x.set_slope(pl, start_pos, slope.flat), null)
		ASSERT_EQUAL(command_x.set_slope(pl, end_pos + coord3d(0, 0, 1), slope.all_down_slope), null)
	}

	// min length
	{
		ASSERT_EQUAL(command_x.set_slope(pl, coord3d(2, 1, 0), slope.south), null)
		ASSERT_EQUAL(command_x.set_slope(pl, coord3d(3, 1, 0), slope.south), null)

		ASSERT_EQUAL(command_x.set_slope(pl, coord3d(2, 2, 0), slope.north), null)
		ASSERT_EQUAL(command_x.set_slope(pl, coord3d(3, 3, 0), slope.north), null)

		ASSERT_EQUAL(bridge_planner_x.find_end(pl, coord3d(2, 1, 0), dir.south, bridge_desc, 0).tostring(), coord3d(2, 2, 0).tostring())
		ASSERT_EQUAL(bridge_planner_x.find_end(pl, coord3d(2, 1, 0), dir.south, bridge_desc, 1).tostring(), coord3d(2, 2, 0).tostring())

		ASSERT_EQUAL(bridge_planner_x.find_end(pl, coord3d(3, 1, 0), dir.south, bridge_desc, 0).tostring(), coord3d(3, 3, 0).tostring())
		ASSERT_EQUAL(bridge_planner_x.find_end(pl, coord3d(3, 1, 0), dir.south, bridge_desc, 1).tostring(), coord3d(-1, -1, -1).tostring())
		ASSERT_EQUAL(bridge_planner_x.find_end(pl, coord3d(3, 1, 0), dir.south, bridge_desc, 2).tostring(), coord3d(3, 3, 0).tostring())
		ASSERT_EQUAL(bridge_planner_x.find_end(pl, coord3d(3, 1, 0), dir.south, bridge_desc, 3).tostring(), coord3d(-1, -1, -1).tostring())

		ASSERT_EQUAL(command_x.set_slope(pl, coord3d(2, 1, 0), slope.flat), null)
		ASSERT_EQUAL(command_x.set_slope(pl, coord3d(3, 1, 0), slope.flat), null)
		ASSERT_EQUAL(command_x.set_slope(pl, coord3d(2, 2, 0), slope.flat), null)
		ASSERT_EQUAL(command_x.set_slope(pl, coord3d(3, 3, 0), slope.flat), null)
	}

	ASSERT_EQUAL(pl.get_current_maintenance(), 0)
	RESET_ALL_PLAYER_FUNDS()
}
