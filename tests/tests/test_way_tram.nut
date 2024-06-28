//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for tramways
//

function test_way_tram_build_flat()
{
	local pl = player_x(0)
	local tramway = way_desc_x.get_available_ways(wt_rail, st_tram)[0]

	// build straight
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 3, 0), coord3d(3, 5, 0), tramway, true), null)

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 3, 0), coord3d(3, 5, 0), "" + wt_rail), null)
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_way_tram_build_parallel()
{
	local pl   = player_x(0)
	local rail_desc = way_desc_x.get_available_ways(wt_rail, st_tram)[0]
	local remover = command_x(tool_remove_way)

	{
		for (local i = 0; i < 16; ++i) {
			ASSERT_EQUAL(command_x.build_way(pl, coord3d(0, i, 0), coord3d(15, i, 0), rail_desc, true), null)
		}

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"2AAAAAAAAAAAAAA8",
				"2AAAAAAAAAAAAAA8",
				"2AAAAAAAAAAAAAA8",
				"2AAAAAAAAAAAAAA8",
				"2AAAAAAAAAAAAAA8",
				"2AAAAAAAAAAAAAA8",
				"2AAAAAAAAAAAAAA8",
				"2AAAAAAAAAAAAAA8",
				"2AAAAAAAAAAAAAA8",
				"2AAAAAAAAAAAAAA8",
				"2AAAAAAAAAAAAAA8",
				"2AAAAAAAAAAAAAA8",
				"2AAAAAAAAAAAAAA8",
				"2AAAAAAAAAAAAAA8",
				"2AAAAAAAAAAAAAA8",
				"2AAAAAAAAAAAAAA8"
			])

		for (local i = 0; i < 16; ++i) {
			ASSERT_EQUAL(remover.work(pl, coord3d(0, i, 0), coord3d(15, i, 0), "" + wt_rail), null)
		}
	}

	RESET_ALL_PLAYER_FUNDS()

	{
		for (local i = 0; i < 16; ++i) {
			ASSERT_EQUAL(command_x.build_way(pl, coord3d(0, i, 0), coord3d(15, i, 0), rail_desc, false), null)
		}

		// FIXME this is different from the road pattern (which is all straight roads even without ctrl)
		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"6EEAAAAAAAAAAAE8",
				"7B9...........3C",
				"5..............5",
				"5..............5",
				"5..............5",
				"5..............5",
				"5..............5",
				"5..............5",
				"5..............5",
				"1..............1",
				"6EEAAAAAAAAAAAE8",
				"7B9...........3C",
				"5..............5",
				"5..............5",
				"5..............5",
				"1..............1"
			])

		ASSERT_EQUAL(remover.work(pl, coord3d(0, 9, 0), coord3d(15, 9, 0), "" + wt_rail), null)
		ASSERT_EQUAL(remover.work(pl, coord3d(0, 15, 0), coord3d(15, 15, 0), "" + wt_rail), null)
		for (local i = 0; i < 15; i = i + 10) {
			ASSERT_EQUAL(remover.work(pl, coord3d(1, i, 0), coord3d(0, 1+i, 0), "" + wt_rail), null)
			ASSERT_EQUAL(remover.work(pl, coord3d(1, i+1, 0), coord3d(2, i, 0), "" + wt_rail), null)
			ASSERT_EQUAL(remover.work(pl, coord3d(14, i, 0), coord3d(15, i, 0), "" + wt_rail), null)
		}
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_way_tram_build_on_road()
{
	local pl = player_x(0)
	local tramway = way_desc_x.get_available_ways(wt_rail, st_tram)[0]
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 3, 0), coord3d(3, 5, 0), road, true), null)

	// build fully on existing road
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 3, 0), coord3d(3, 5, 0), tramway, true), null)
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"...4....",
				"...5....",
				"...1....",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"...4....",
				"...5....",
				"...1....",
				"........",
				"........"
			])

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 3, 0), coord3d(3, 5, 0), "" + wt_rail), null)
	}

	// cross existing road
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 4, 0), coord3d(4, 4, 0), tramway, true), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"...4....",
				"...5....",
				"...1....",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"........",
				"..2A8...",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(2, 4, 0), coord3d(4, 4, 0), "" + wt_rail), null)
	}

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 3, 0), coord3d(3, 5, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_way_tram_build_across_road_bridge()
{
	local pl = player_x(0)
	local bridge = bridge_desc_x.get_available_bridges(wt_road)[0]
	local setslope = command_x.set_slope
	local tramway = way_desc_x.get_available_ways(wt_rail, st_tram)[0]

	// build bridge
	ASSERT_EQUAL(setslope(pl, coord3d(3, 3, 0), slope.south), null)
	ASSERT_EQUAL(setslope(pl, coord3d(3, 5, 0), slope.north), null)
	ASSERT_EQUAL(command_x.build_bridge_at(pl, coord3d(3, 3, 0), bridge), null)

	// and build tram track
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 3, 0), coord3d(3, 5, 0), tramway, true), null)
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 1),
			[
				"........",
				"........",
				"........",
				"...4....",
				"...5....",
				"...1....",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 1),
			[
				"........",
				"........",
				"........",
				"...4....",
				"...5....",
				"...1....",
				"........",
				"........"
			])

		// do not remove tram tracks here, this is done by tool_remover below
	}

	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 3, 0)), null)
	ASSERT_EQUAL(setslope(pl, coord3d(3, 3, 0), slope.flat), null)
	ASSERT_EQUAL(setslope(pl, coord3d(3, 5, 0), slope.flat), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_way_tram_build_across_crossing()
{
	local pl = player_x(0)

	local tramways = way_desc_x.get_available_ways(wt_rail, st_tram)
	tramways.sort(@(a, b) a.get_topspeed() <=> b.get_topspeed())
	local tramway = tramways[0]

	local roads = way_desc_x.get_available_ways(wt_road, st_flat)
	roads.sort(@(a, b) a.get_topspeed() <=> b.get_topspeed())
	local road = roads[0]

	local rails = way_desc_x.get_available_ways(wt_rail, st_flat)
	rails.sort(@(a, b) a.get_topspeed() <=> b.get_topspeed())
	rails.filter(@(idx, desc) desc.get_topspeed() >= tramway.get_topspeed())
	local rail = rails[0]

	// build crossing
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 3, 0), coord3d(3, 5, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 4, 0), coord3d(4, 4, 0), road, true), null)

	// build tram track
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 4, 0), coord3d(4, 4, 0), tramway, true), null)

		// crossing should have been replaced by tramway

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(2, 4, 0), coord3d(4, 4, 0), "" + wt_rail), null)
	}

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 3, 0), coord3d(3, 5, 0), "" + wt_rail), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(2, 4, 0), coord3d(4, 4, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_way_tram_build_in_tunel()
{
	local pl = player_x(0)
	local tramway = way_desc_x.get_available_ways(wt_rail, st_tram)[0]
	local road_tunnel = tunnel_desc_x.get_available_tunnels(wt_road)[0]
	local rail_tunnel = tunnel_desc_x.get_available_tunnels(wt_rail)[0]

	ASSERT_EQUAL(command_x.grid_raise(pl, coord3d(2, 2, 0)), null)
	ASSERT_EQUAL(command_x.grid_raise(pl, coord3d(2, 3, 0)), null)
	ASSERT_EQUAL(command_x.grid_raise(pl, coord3d(3, 2, 0)), null)
	ASSERT_EQUAL(command_x.grid_raise(pl, coord3d(3, 3, 0)), null)
	ASSERT_EQUAL(command_x.grid_raise(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x.grid_raise(pl, coord3d(4, 3, 0)), null)

	ASSERT_EQUAL(command_x(tool_build_tunnel).work(pl, coord3d(1, 2, 0), road_tunnel.get_name()), null)

	// build tramway through road tunnel
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 2, 0), coord3d(4, 2, 0), tramway, true), null)
		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(1, 2, 0), coord3d(4, 2, 0), "" + wt_rail), null)
	}

	// cross road tunnel with tramway
	{
		local tunnel_builder = command_x(tool_build_tunnel)
		tunnel_builder.set_flags(2) // ctrl
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 2, 0), coord3d(4, 2, 0), tramway, true), null)
		ASSERT_EQUAL(tunnel_builder.work(pl, coord3d(2, 1, 0), rail_tunnel.get_name()), null)
		ASSERT_EQUAL(tunnel_builder.work(pl, coord3d(2, 3, 0), rail_tunnel.get_name()), null)

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"........",
				"..0.....",
				".2AA8...",
				"..0.....",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 1, 0), coord3d(2, 3, 0), tramway, true), "")
		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"........",
				"..0.....",
				".2AA8...",
				"..0.....",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"........",
				".2AA8...",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(2, 1, 0), coord3d(2, 1, 0), "" + wt_rail), null)
		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(2, 3, 0), coord3d(2, 3, 0), "" + wt_rail), null)
		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(1, 2, 0), coord3d(4, 2, 0), "" + wt_rail), null)
	}

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(1, 2, 0), coord3d(4, 2, 0), "" + wt_road), null)

	ASSERT_EQUAL(command_x.grid_lower(pl, coord3d(2, 2, 1)), null)
	ASSERT_EQUAL(command_x.grid_lower(pl, coord3d(2, 3, 1)), null)
	ASSERT_EQUAL(command_x.grid_lower(pl, coord3d(3, 2, 1)), null)
	ASSERT_EQUAL(command_x.grid_lower(pl, coord3d(3, 3, 1)), null)
	ASSERT_EQUAL(command_x.grid_lower(pl, coord3d(4, 2, 1)), null)
	ASSERT_EQUAL(command_x.grid_lower(pl, coord3d(4, 3, 1)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_way_tram_has_double_slopes()
{
	local roads = way_desc_x.get_available_ways(wt_rail, st_tram)

	foreach (r in roads) {
		ASSERT_FALSE(r.has_double_slopes())
	}
}
