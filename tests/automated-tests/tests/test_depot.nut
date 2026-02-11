//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for depots
//


function test_depot_build_invalid_params()
{
	local pl = player_x(0)
	local public_pl = player_x(1)

	// invalid default_param
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_build_depot).work(pl, coord3d(0, 0, 0), null), "")
		}
		catch (e) {
			error_caught = true;
			ASSERT_EQUAL(e, "Error during initializing tool")
		}

		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_build_depot).work(pl, coord3d(0, 0, 0), ""), "")
		}
		catch (e) {
			error_caught = true;
			ASSERT_EQUAL(e, "Error during initializing tool")
		}

		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_build_depot).work(pl, coord3d(0, 0, 0), "nonexistent"), "")
		}
		catch (e) {
			error_caught = true;
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
	}

	// build as public player
	{
		local error_caught = false
		try {
			command_x.build_depot(public_pl, coord3d(0, 0, 0), get_depot_by_wt(wt_road))
		}
		catch (e) {
			error_caught = true;
			ASSERT_EQUAL(e, "Error during initializing tool")
		}

		ASSERT_TRUE(error_caught)
		ASSERT_EQUAL(depot_x.get_depot_list(pl, wt_road).len(), 0)
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_depot_build_invalid_pos()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local road_depot = get_depot_by_wt(wt_road)
	local hangar = get_depot_by_wt(wt_air)
	local shipyard = get_depot_by_wt(wt_water)
	local runway_desc = way_desc_x.get_available_ways(wt_air, st_elevated)[0]
	local station_desc = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x.passenger)[0]

	local road_descs = way_desc_x.get_available_ways(wt_road, st_flat)
	local rail_descs = way_desc_x.get_available_ways(wt_rail, st_flat)
	road_descs.sort(@(a, b) a.get_topspeed() <=> b.get_topspeed())
	rail_descs.sort(@(a, b) a.get_topspeed() <=> b.get_topspeed())

	local road_desc = road_descs[0]
	local rail_desc = rail_descs[0]

	ASSERT_TRUE(runway_desc != null)
	ASSERT_TRUE(road_desc != null)
	ASSERT_TRUE(rail_desc != null)

	// invalid pos
	{
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(-1, -1, 0), road_depot), "")
	}

	// no shipyards on land
	{
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 2, 0), shipyard), "Ship depots must be built on water!")
	}

	// no land depots on water
	{
		ASSERT_EQUAL(command_x(tool_set_climate).work(pl, coord3d(4, 2, 0), coord3d(4, 2, 0), "" + cl_water), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 2, 0), hangar), "Depots must be built on flat dead-end way tiles!")
		ASSERT_EQUAL(command_x(tool_set_climate).work(pl, coord3d(4, 2, 0), coord3d(4, 2, 0), "" + cl_mediterran), null)
	}

	// no hangars on runways
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 5, 0), coord3d(5, 7, 0), runway_desc, true), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(5, 5, 0), hangar), "Depots cannot be built on runways!")
		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(5, 5, 0), coord3d(5, 7, 0), "" + wt_air), null)
	}

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), road_desc, true), null)

	// no depot in the middle of a road
	{
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 3, 0), road_depot), "Depots must be built on flat dead-end way tiles!")
	}

	// no depot over a stop
	{
		ASSERT_EQUAL(command_x.build_station(pl, coord3d(4, 2, 0), station_desc, 0), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 2, 0), road_depot), "Tile not empty.")
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)
	}

	// do not replace existing depots
	{
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 2, 0), road_depot), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 2, 0), road_depot), "Tile not empty.")
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)
	}

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 3, 0), coord3d(5, 3, 0), road_desc, true), null)

	// no depot on road-road crossings
	{
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 3, 0), road_depot), "Depots must be built on flat dead-end way tiles!")
	}

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 3, 0), coord3d(5, 3, 0), "" + wt_road), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 3, 0), coord3d(5, 3, 0), rail_desc, true), null)

	// no depot on road-rail crossings
	{
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 3, 0), road_depot), "Tile not empty.")
	}

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 3, 0), coord3d(5, 3, 0), "" + wt_rail), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_depot_build_road()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local default_depot = get_depot_by_wt(wt_road)
	local way_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0]

	// preconditions
	ASSERT_TRUE(default_depot != null)
	ASSERT_EQUAL(depot_x.get_depot_list(pl, wt_road).len(), 0)
	ASSERT_TRUE(way_desc != null)

	ASSERT_EQUAL(command_x.build_way(public_pl, coord3d(4, 2, 0), coord3d(4, 4, 0), way_desc, true), null)
	ASSERT_EQUAL(command_x.build_way(public_pl, coord3d(3, 3, 0), coord3d(5, 3, 0), way_desc, true), null)

	// build depot as normal player, all 4 directions
	{
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 2, 0), default_depot), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(5, 3, 0), default_depot), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 4, 0), default_depot), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(3, 3, 0), default_depot), null)

		ASSERT_EQUAL(depot_x.get_depot_list(player_x(0), wt_road).len(), 4)

		ASSERT_TRUE(depot_x(4, 2, 0).is_valid())
		ASSERT_TRUE(depot_x(5, 3, 0).is_valid())
		ASSERT_TRUE(depot_x(4, 4, 0).is_valid())
		ASSERT_TRUE(depot_x(3, 3, 0).is_valid())
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(5, 3, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 4, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 3, 0)), null)

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_road), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 3, 0), coord3d(5, 3, 0), "" + wt_road), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_depot_build_road_on_tram_crossing()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local default_depot = get_depot_by_wt(wt_road)
	local way_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local tramtrack = way_desc_x.get_available_ways(wt_rail, st_tram)[0]

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), way_desc, true), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 2, 0), coord3d(4, 2, 0), tramtrack, true), null)

	{
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 2, 0), default_depot), "Tile not empty.")
	}

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_road), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 2, 0), coord3d(4, 2, 0), "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_depot_build_water()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local default_depot = get_depot_by_wt(wt_water)

	// preconditions
	ASSERT_TRUE(default_depot != null)
	ASSERT_EQUAL(depot_x.get_depot_list(pl, wt_water).len(), 0)

	ASSERT_EQUAL(command_x(tool_set_climate).work(pl, coord3d(4, 2, 0), coord3d(4, 2, 0), "" + cl_water), null)

	// build depot as normal player
	{
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 2, 0), default_depot), null)
		ASSERT_EQUAL(depot_x.get_depot_list(player_x(0), wt_water).len(), 1)
		ASSERT_TRUE(depot_x(4, 2, 0).is_valid())
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_set_climate).work(pl, coord3d(4, 2, 0), coord3d(4, 2, 0), "" + cl_mediterran), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_depot_build_monorail()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local default_depot = get_depot_by_wt(wt_monorail)
	local way_desc = way_desc_x.get_available_ways(wt_monorail, st_flat)[0]
	local elevated_monorail = way_desc_x.get_available_ways(wt_monorail, st_elevated)[0]

	// preconditions
	ASSERT_TRUE(default_depot != null)
	ASSERT_EQUAL(depot_x.get_depot_list(pl, wt_monorail).len(), 0)
	ASSERT_TRUE(way_desc != null)

	ASSERT_EQUAL(command_x.build_way(public_pl, coord3d(4, 2, 0), coord3d(4, 4, 0), way_desc, true), null)
	ASSERT_EQUAL(command_x.build_way(public_pl, coord3d(3, 3, 0), coord3d(5, 3, 0), way_desc, true), null)

	// build depot as normal player, all 4 directions
	{
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 2, 0), default_depot), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(5, 3, 0), default_depot), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 4, 0), default_depot), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(3, 3, 0), default_depot), null)

		ASSERT_EQUAL(depot_x.get_depot_list(player_x(0), wt_monorail).len(), 4)

		ASSERT_TRUE(depot_x(4, 2, 0).is_valid())
		ASSERT_TRUE(depot_x(5, 3, 0).is_valid())
		ASSERT_TRUE(depot_x(4, 4, 0).is_valid())
		ASSERT_TRUE(depot_x(3, 3, 0).is_valid())

		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(5, 3, 0)), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 4, 0)), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 3, 0)), null)
	}

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_monorail), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 3, 0), coord3d(5, 3, 0), "" + wt_monorail), null)

	ASSERT_EQUAL(command_x.build_way(public_pl, coord3d(4, 2, 0), coord3d(4, 4, 0), elevated_monorail, true), null)
	ASSERT_EQUAL(command_x.build_way(public_pl, coord3d(3, 3, 0), coord3d(5, 3, 0), elevated_monorail, true), null)

	// build elevated monorail depot, with foundation, all 4 directions
	{
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 2, 1), default_depot), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(5, 3, 1), default_depot), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 4, 1), default_depot), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(3, 3, 1), default_depot), null)

		ASSERT_EQUAL(depot_x.get_depot_list(player_x(0), wt_monorail).len(), 4)

		ASSERT_TRUE(depot_x(4, 2, 1).is_valid())
		ASSERT_TRUE(depot_x(5, 3, 1).is_valid())
		ASSERT_TRUE(depot_x(4, 4, 1).is_valid())
		ASSERT_TRUE(depot_x(3, 3, 1).is_valid())

		ASSERT_FALSE(square_x(4, 2).get_tile_at_height(0).is_empty())
		ASSERT_FALSE(square_x(5, 3).get_tile_at_height(0).is_empty())
		ASSERT_FALSE(square_x(4, 4).get_tile_at_height(0).is_empty())
		ASSERT_FALSE(square_x(3, 3).get_tile_at_height(0).is_empty())

		// This leaves behind the depot foundations
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 1)), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(5, 3, 1)), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 4, 1)), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 3, 1)), null)
	}

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 2, 1), coord3d(4, 4, 1), "" + wt_monorail), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 3, 1), coord3d(5, 3, 1), "" + wt_monorail), null)

	ASSERT_FALSE(square_x(4, 2).get_tile_at_height(0).is_empty())
	ASSERT_FALSE(square_x(5, 3).get_tile_at_height(0).is_empty())
	ASSERT_FALSE(square_x(4, 4).get_tile_at_height(0).is_empty())
	ASSERT_FALSE(square_x(3, 3).get_tile_at_height(0).is_empty())

	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(5, 3, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 4, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 3, 0)), null)

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_depot_build_tram()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local default_depot = get_depot_by_wt(wt_tram)
	local way_desc = way_desc_x.get_available_ways(wt_rail, st_tram)[0]

	// preconditions
	ASSERT_TRUE(default_depot != null)
	ASSERT_EQUAL(depot_x.get_depot_list(pl, wt_tram).len(), 0)
	ASSERT_TRUE(way_desc != null)

	ASSERT_EQUAL(command_x.build_way(public_pl, coord3d(4, 2, 0), coord3d(4, 4, 0), way_desc, true), null)
	ASSERT_EQUAL(command_x.build_way(public_pl, coord3d(3, 3, 0), coord3d(5, 3, 0), way_desc, true), null)

	// build depot as normal player, all 4 directions
	{
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 2, 0), default_depot), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(5, 3, 0), default_depot), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 4, 0), default_depot), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(3, 3, 0), default_depot), null)

		ASSERT_EQUAL(depot_x.get_depot_list(player_x(0), wt_tram).len(), 4)

		ASSERT_TRUE(depot_x(4, 2, 0).is_valid())
		ASSERT_TRUE(depot_x(5, 3, 0).is_valid())
		ASSERT_TRUE(depot_x(4, 4, 0).is_valid())
		ASSERT_TRUE(depot_x(3, 3, 0).is_valid())
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(5, 3, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 4, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 3, 0)), null)

	// note: wt_tram does not work
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_rail), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 3, 0), coord3d(5, 3, 0), "" + wt_rail), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_depot_build_sloped()
{
	local pl = player_x(0)
	local setslope = command_x.set_slope
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local wayremover = command_x(tool_remove_way)

	local pos = coord3d(4, 3, 0)

	{
		for (local sl = slope.flat+1; sl < slope.raised; ++sl) {
			ASSERT_EQUAL(setslope(pl, pos, sl), null)

			local d = slope.to_dir(sl)
			if (d != dir.none) { // only consider slopes we can build roads on
				RESET_ALL_PLAYER_FUNDS()

				local c = dir.to_coord(dir.backward(d))
				local adjacent = pos + coord3d(c.x, c.y, 0)
				ASSERT_EQUAL(command_x.build_way(pl, adjacent, pos, road, true), null)
				local old_maintenance = pl.get_current_maintenance()

				ASSERT_EQUAL(command_x.build_depot(pl, pos, get_depot_by_wt(wt_road)), "Depots must be built on flat dead-end way tiles!")
				ASSERT_EQUAL(pl.get_current_maintenance(), old_maintenance)
				ASSERT_EQUAL(tile_x(pos.x, pos.y, pos.z).find_object(mo_building), null)

				ASSERT_EQUAL(wayremover.work(pl, pos, adjacent, "" + wt_road), null)
			}
		}
	}

	// clean up
	ASSERT_EQUAL(setslope(pl, pos, slope.flat), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_depot_build_on_tunnel_entrance()
{
	local pl = player_x(0)
	local rail_tunnel = tunnel_desc_x.get_available_tunnels(wt_rail)[0]

	ASSERT_EQUAL(command_x.grid_raise(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x.grid_raise(pl, coord3d(4, 3, 0)), null)
	ASSERT_EQUAL(command_x.grid_raise(pl, coord3d(5, 2, 0)), null)
	ASSERT_EQUAL(command_x.grid_raise(pl, coord3d(5, 3, 0)), null)

	ASSERT_EQUAL(command_x(tool_build_tunnel).work(pl, coord3d(4, 1, 0), rail_tunnel.get_name()), null)
	ASSERT_EQUAL(command_x(tool_build_tunnel).work(pl, coord3d(3, 2, 0), rail_tunnel.get_name()), null)
	ASSERT_EQUAL(command_x(tool_build_tunnel).work(pl, coord3d(5, 2, 0), rail_tunnel.get_name()), null)

	// Building depots on tunnel entrance works (contrary to stations)
	{
		for (local d = dir.north; d < dir.all; d = d*2) {
			local p = coord3d(4, 2, 0) + dir.to_coord(d)

			ASSERT_EQUAL(command_x.build_depot(pl, p, get_depot_by_wt(wt_rail)), null)
		}
	}

	local remover = command_x(tool_remover)
	remover.set_flags(2)
	ASSERT_EQUAL(remover.work(pl, coord3d(4, 1, 0)), null)
	ASSERT_EQUAL(remover.work(pl, coord3d(4, 1, 0)), null)
	ASSERT_EQUAL(remover.work(pl, coord3d(5, 2, 0)), null)
	ASSERT_EQUAL(remover.work(pl, coord3d(4, 3, 0)), null)
	ASSERT_EQUAL(remover.work(pl, coord3d(3, 2, 0)), null)

	ASSERT_EQUAL(command_x.grid_lower(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x.grid_lower(pl, coord3d(4, 3, 0)), null)
	ASSERT_EQUAL(command_x.grid_lower(pl, coord3d(5, 2, 0)), null)
	ASSERT_EQUAL(command_x.grid_lower(pl, coord3d(5, 3, 0)), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_depot_build_on_bridge_end()
{
	local pl = player_x(0)
	local rail_bridge = bridge_desc_x.get_available_bridges(wt_rail)[0]
	local setslope = command_x.set_slope

	// north-south direction
	{
		ASSERT_EQUAL(setslope(pl, coord3d(4, 2, 0), slope.south), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 4, 0), slope.north), null)

		ASSERT_EQUAL(command_x(tool_build_bridge).work(pl, coord3d(4, 2, 0), rail_bridge.get_name()), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 2, 0), get_depot_by_wt(wt_rail)), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 4, 0), get_depot_by_wt(wt_rail)), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)

		ASSERT_EQUAL(setslope(pl, coord3d(4, 2, 0), slope.flat), null)
		ASSERT_EQUAL(setslope(pl, coord3d(4, 4, 0), slope.flat), null)
	}

	// east-west direction
	{
		ASSERT_EQUAL(setslope(pl, coord3d(3, 3, 0), slope.east), null)
		ASSERT_EQUAL(setslope(pl, coord3d(5, 3, 0), slope.west), null)

		ASSERT_EQUAL(command_x(tool_build_bridge).work(pl, coord3d(3, 3, 0), rail_bridge.get_name()), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(3, 3, 0), get_depot_by_wt(wt_rail)), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(5, 3, 0), get_depot_by_wt(wt_rail)), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 3, 0)), null)

		ASSERT_EQUAL(setslope(pl, coord3d(3, 3, 0), slope.flat), null)
		ASSERT_EQUAL(setslope(pl, coord3d(5, 3, 0), slope.flat), null)
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_depot_build_on_halt()
{
	local pl = player_x(0)
	local rail_desc = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local station_desc = building_desc_x.get_available_stations(building_desc_x.station, wt_rail, good_desc_x.passenger)[0]
	local depot = get_depot_by_wt(wt_rail)

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), rail_desc, true), null)
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(4, 2, 0), station_desc), null)

	{
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 2, 0), depot), "Tile not empty.")
	}

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_depot_convoy_add_normal()
{
	// build depot
	local pl = player_x(0)
	local public_pl = player_x(1)
	local way_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0]

	ASSERT_EQUAL(command_x.build_way(public_pl, coord3d(0, 0, 0), coord3d(0, 1, 0), way_desc, true), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(0, 0, 0), get_depot_by_wt(wt_road)), null)

	ASSERT_EQUAL(depot_x.get_depot_list(pl, wt_road).len(), 1)
	local the_depot = depot_x.get_depot_list(pl, wt_road)[0]
	ASSERT_EQUAL(the_depot.get_convoy_list().len(), 0)

	local vehicle_desc = vehicle_desc_x.get_available_vehicles(wt_road)[0]
	ASSERT_TRUE(vehicle_desc != null)

	{
		ASSERT_TRUE(the_depot.append_vehicle(pl, convoy_x(0), vehicle_desc))

		ASSERT_EQUAL(the_depot.get_convoy_list().len(), 1)
		local cnv = the_depot.get_convoy_list()[0]

		ASSERT_EQUAL(cnv.get_vehicles().len(), 1)
		ASSERT_EQUAL(cnv.get_pos().tostring(), coord3d(0, 0, 0).tostring())
		ASSERT_EQUAL(cnv.get_pos().tostring(), cnv.get_home_depot().tostring())
		ASSERT_EQUAL(cnv.get_speed(), 0)
		ASSERT_EQUAL(cnv.get_traveled_distance()[0], 0)
		ASSERT_EQUAL(cnv.get_distance_traveled_total(), 0)
		ASSERT_TRUE(cnv.is_in_depot())
		ASSERT_FALSE(cnv.is_waiting())
		ASSERT_EQUAL(cnv.get_line(), null)

		ASSERT_TRUE(cnv.destroy(player_x(0)))
		ASSERT_EQUAL(the_depot.get_convoy_list().len(), 0)
	}

	// clean up
	local remover = command_x(tool_remover)
	ASSERT_EQUAL(remover.work(player_x(1), tile_x(0, 0, 0)), null)
	ASSERT_EQUAL(remover.work(player_x(1), tile_x(0, 0, 0)), null)
	ASSERT_EQUAL(remover.work(player_x(1), tile_x(0, 1, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_depot_convoy_add_nonelectrified()
{
	local pl = player_x(0)
	local way_desc = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local electro_loco = vehicle_desc_x.get_available_vehicles(wt_rail).filter(@(idx, v) ( v.needs_electrification() && v.can_be_first() ))[0]

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), way_desc, true), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 2, 0), get_depot_by_wt(wt_rail)), null)

	local the_depot = depot_x(4, 2, 0)

	{
		ASSERT_TRUE(the_depot.append_vehicle(pl, convoy_x(0), electro_loco)) // can add loco via script even if depot is not electrified

		ASSERT_EQUAL(the_depot.get_convoy_list().len(), 1)
		local cnv = the_depot.get_convoy_list()[0]
		ASSERT_EQUAL(cnv.get_vehicles().len(), 1)
		ASSERT_EQUAL(cnv.get_pos().tostring(), coord3d(4, 2, 0).tostring())

		ASSERT_TRUE(cnv.destroy(pl))
	}

	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}
