//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for powerline stuff and transformers
//


function test_powerline_connect()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local powerline = way_desc_x.get_available_ways(wt_power, st_flat)[0]
	local wayremover = command_x(tool_remove_way)

	ASSERT_TRUE(powerline != null)

	local center = coord3d(3, 4, 0)
	local north = center + dir.to_coord(dir.north)
	local east  = center + dir.to_coord(dir.east)
	local south = center + dir.to_coord(dir.south)
	local west  = center + dir.to_coord(dir.west)

	// preconditions
	ASSERT_EQUAL(pl.get_current_maintenance(), 0)

	{
		ASSERT_EQUAL(command_x.build_way(pl, center, south, powerline, true), null)
		ASSERT_EQUAL(wayremover.work(pl, south, south, "" + wt_power), null)

		ASSERT_TRUE(powerline_x(center.x, center.y, center.z).is_connected(powerline_x(center.x, center.y, center.z)))

		ASSERT_WAY_PATTERN(wt_power, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"........",
				"...0....",
				"........",
				"........",
				"........"
			])
	}

	local maint_per_tile = pl.get_current_maintenance()
	ASSERT_TRUE(maint_per_tile > 0)

	{
		ASSERT_EQUAL(command_x.build_way(pl, center, north, powerline, true), null)

		ASSERT_TRUE(powerline_x(center.x, center.y, center.z).is_connected(powerline_x(north.x, north.y, north.z)))

		ASSERT_WAY_PATTERN(wt_power, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"...4....",
				"...1....",
				"........",
				"........",
				"........"
			])
		ASSERT_EQUAL(pl.get_current_maintenance(), 2*maint_per_tile)
	}


	{
		ASSERT_EQUAL(command_x.build_way(pl, center, east, powerline, true), null)

		ASSERT_TRUE(powerline_x(center.x, center.y, center.z).is_connected(powerline_x(east.x, east.y, east.z)))

		ASSERT_WAY_PATTERN(wt_power, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"...4....",
				"...38...",
				"........",
				"........",
				"........"
			])
		ASSERT_EQUAL(pl.get_current_maintenance(), 3*maint_per_tile)
	}

	{
		ASSERT_EQUAL(command_x.build_way(pl, center, south, powerline, true), null)

		ASSERT_TRUE(powerline_x(center.x, center.y, center.z).is_connected(powerline_x(south.x, south.y, south.z)))

		ASSERT_WAY_PATTERN(wt_power, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"...4....",
				"...78...",
				"...1....",
				"........",
				"........"
			])
		ASSERT_EQUAL(pl.get_current_maintenance(), 4*maint_per_tile)
	}

	{
		ASSERT_EQUAL(command_x.build_way(pl, center, west, powerline, true), null)

		ASSERT_TRUE(powerline_x(center.x, center.y, center.z).is_connected(powerline_x(west.x, west.y, west.z)))

		ASSERT_WAY_PATTERN(wt_power, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"...4....",
				"..2F8...",
				"...1....",
				"........",
				"........"
			])
		ASSERT_EQUAL(pl.get_current_maintenance(), 5*maint_per_tile)
	}

	// clean up
	ASSERT_EQUAL(wayremover.work(pl, coord3d(2, 4, 0), coord3d(4, 4, 0), "" + wt_power), null)
	ASSERT_EQUAL(wayremover.work(pl, coord3d(3, 3, 0), coord3d(3, 5, 0), "" + wt_power), null)
	ASSERT_EQUAL(pl.get_current_maintenance(), 0)

	RESET_ALL_PLAYER_FUNDS()
}


function test_powerline_build_below_powerbridge()
{
	local pl = player_x(0)
	local power_bridge = bridge_desc_x.get_available_bridges(wt_power)[0]
	local powerline = way_desc_x.get_available_ways(wt_power, st_flat)[0]

	ASSERT_EQUAL(command_x.build_bridge(pl, coord3d(3, 2, 0), coord3d(3, 4, 0), power_bridge), null)

	// build normal powerline below power bridge, should fail
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 3, 0), coord3d(4, 3, 0), powerline, true), "")
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 2, 0), coord3d(3, 4, 0), "" + wt_power), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_powerline_build_powerbridge_above_powerline()
{
	local pl = player_x(0)
	local power_bridge = bridge_desc_x.get_available_bridges(wt_power)[0]
	local powerline = way_desc_x.get_available_ways(wt_power, st_flat)[0]

	// Note: Build way first, then bridge (see above for why)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 3, 0), coord3d(4, 3, 0), powerline, true), null)

	// build flat powerline bridge
	{
		ASSERT_EQUAL(command_x.build_bridge(pl, coord3d(3, 2, 0), coord3d(3, 4, 0), power_bridge), null)

		local bridge_part = powerline_x(3, 3, 1)
		local ground_part = powerline_x(3, 3, 0)
		ASSERT_TRUE(bridge_part != null)
		ASSERT_TRUE(ground_part != null)

		ASSERT_FALSE(bridge_part.is_connected(ground_part))
		ASSERT_FALSE(ground_part.is_connected(bridge_part))
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(2, 3, 0), coord3d(4, 3, 0), "" + wt_power), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 2, 0), coord3d(3, 4, 0), "" + wt_power), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_powerline_build_transformer()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local build_trafo = command_x(tool_build_transformer)
	local setclimate = command_x(tool_set_climate)
	local setslope = command_x(tool_setslope)
	local power_tunnel = tunnel_desc_x.get_available_tunnels(wt_power)[0]

	// preconditions
	ASSERT_TRUE(power_tunnel != null)
	ASSERT_EQUAL(pl.get_current_maintenance(), 0)

	// build coal mine + coal power plant, then link them
	ASSERT_EQUAL(build_factory(public_pl, coord3d(0, 0, 0), 1, 1, 1024, "Kohlegrube"), null)
	ASSERT_EQUAL(build_factory(public_pl, coord3d(6, 6, 0), 1, 1, 1024, "Kohlekraftwerk"), null)
	command_x(tool_link_factory).work(public_pl, coord3d(0, 0, 0), coord3d(7, 7, 0), "")

	ASSERT_EQUAL(pl.get_current_maintenance(), 0)


	{
		// build transformer not adjacent to a factory
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(0, 7, 0)), "Transformer only next to factory!")
		ASSERT_EQUAL(pl.get_current_maintenance(), 0)

		// diagonally adjacent to factory, not allowed
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(3, 3, 0)), "Transformer only next to factory!")
		ASSERT_EQUAL(pl.get_current_maintenance(), 0)
	}

	{
		// cannot build transformer on water even if adjacent to factory
		ASSERT_EQUAL(setclimate.work(pl, coord3d(3, 2, 0), coord3d(3, 2, 0), "" + cl_water), null)

		ASSERT_EQUAL(build_trafo.work(pl, coord3d(3, 2, 0)), "Transformer only on flat bare land!")
		ASSERT_EQUAL(pl.get_current_maintenance(), 0)

		ASSERT_EQUAL(setclimate.work(pl, coord3d(3, 2, 0), coord3d(3, 2, 0), "" + cl_mediterran), null)
	}

	{
		// build transformer in the air, same as building on map ground
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(3, 2, 1)), null)
		ASSERT_TRUE(pl.get_current_maintenance() > 0)

		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 2, 0)), null)
		ASSERT_EQUAL(pl.get_current_maintenance(), 0)
	}

	{
		// cannot build transformer on sloped tile
		ASSERT_EQUAL(setslope.work(pl, coord3d(3, 2, 0), "" + slope.north), null)

		ASSERT_EQUAL(build_trafo.work(pl, coord3d(3, 2, 0)), "Transformer only on flat bare land!")
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(3, 2, 1)), "Transformer only on flat bare land!")
		ASSERT_EQUAL(pl.get_current_maintenance(), 0)
	}

	{
		// Cannot build transformer underground adjacent to factory
		// Depends also on allow_underground_transformers setting
		ASSERT_EQUAL(setslope.work(pl, coord3d(3, 2, 0), "" + slope.all_up_slope), null)
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(3, 2, 0)), "Transformer only next to factory!")
		// 2 height levels
		ASSERT_EQUAL(setslope.work(pl, coord3d(3, 2, 1), "" + slope.all_up_slope), null)
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(3, 2, 0)), "Transformer only next to factory!")

		// However, building on flat ground on a different height level works
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(3, 2, 2)), null)
		ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(3, 2, 2)), null)

		ASSERT_EQUAL(setslope.work(pl, coord3d(3, 2, 2), "" + slope.all_down_slope), null)
		ASSERT_EQUAL(setslope.work(pl, coord3d(3, 2, 1), "" + slope.all_down_slope), null)
		ASSERT_EQUAL(pl.get_current_maintenance(), 0)
	}

	{
		// Build transformer underground below mine (senke_t)
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(2, 2, -1)), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(2, 2, -1)), null)
		ASSERT_EQUAL(pl.get_current_maintenance(), 0)

		// And below power plant (pumpe_t)
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(6, 6, -1)), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(6, 6, -1)), null)
		ASSERT_EQUAL(pl.get_current_maintenance(), 0)
	}

	{
		// build underground transformer on underground powerline, should fail
		local lower = command_x(tool_lower_land)
		local raise = command_x(tool_raise_land)
		local digger = command_x(tool_build_tunnel)
		digger.set_flags(2) # Ctrl

		ASSERT_EQUAL(lower.work(pl, coord3d(2, 4, 0)), null)
		ASSERT_EQUAL(lower.work(pl, coord3d(3, 4, 0)), null)
		ASSERT_EQUAL(pl.get_current_maintenance(), 0)

		ASSERT_EQUAL(digger.work(pl, coord3d(2, 3, -1), power_tunnel.get_name()), null)
		ASSERT_EQUAL(digger.work(pl, coord3d(2, 3, -1), coord3d(2, 2, -1), power_tunnel.get_name()), null)

		// test
		local old_maint = pl.get_current_maintenance()
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(2, 2, -1)), "Tile not empty.")
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(2, 2, -1)), null)

		// and clean up
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(2, 2, -1)), "")   // underground  ???
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(2, 3, -1)), null) // above ground ???
		ASSERT_EQUAL(pl.get_current_maintenance(), 0)

		ASSERT_EQUAL(raise.work(pl, coord3d(2, 4, -1)), null)
		ASSERT_EQUAL(raise.work(pl, coord3d(3, 4, -1)), null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(0, 0, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(7, 7, 0)), null)
	ASSERT_EQUAL(pl.get_current_maintenance(), 0)

	RESET_ALL_PLAYER_FUNDS()
}


function test_powerline_build_over_transformer()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local build_trafo = command_x(tool_build_transformer)
	local production = 1024 // 1/s
	local powerline = way_desc_x.get_available_ways(wt_power, st_flat)[0]

	ASSERT_EQUAL(build_factory(public_pl, coord3d(0, 0, 0), 1, 1, production, "Kohlegrube"), null)

	{
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(3, 1, 0)), null)
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 0, 0), coord3d(3, 2, 0), powerline, true), null)

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 0, 0), coord3d(3, 2, 0), "" + wt_power), null)

		ASSERT_EQUAL(tile_x(3, 1, 0).find_object(mo_transformer_s), null)
		ASSERT_EQUAL(tile_x(3, 1, 0).find_object(mo_transformer_c), null)
	}

	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(0, 0, 0)), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_powerline_build_transformer_multiple()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local build_trafo = command_x(tool_build_transformer)
	local production = 1024 // 1/s
	local powerline = way_desc_x.get_available_ways(wt_power, st_flat)[0]

	// build coal mine + coal power plant
	ASSERT_EQUAL(build_factory(public_pl, coord3d(0, 0, 0), 1, 1, production, "Kohlegrube"), null)
	ASSERT_EQUAL(build_factory(public_pl, coord3d(3, 0, 0), 1, 1, production, "Kohlekraftwerk"), null)

	// single transformer adjacent to multiple factories
	{
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(3, 2, 0)), null)
		ASSERT_TRUE(tile_x(3, 2, 0).find_object(mo_transformer_s) != null)
		ASSERT_TRUE(tile_x(3, 2, 0).find_object(mo_transformer_c) == null)

		local trafo = transformer_x(3, 2, 0)
		local pp = factory_x(3, 0)
		local mine = factory_x(0, 0)
		ASSERT_EQUAL(trafo.get_factory().get_name(), pp.get_name())
		ASSERT_TRUE(pp.is_transformer_connected())
		ASSERT_FALSE(mine.is_transformer_connected())

		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 2, 0)), null)
	}

	// multiple transformers next to single factory
	{
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(0, 3, 0)), null)
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(1, 3, 0)), "Only one transformer per factory!")

		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(0, 3, 0)), null)
	}

	// same as before, but for electricity producers
	{
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(5, 1, 0)), null)
		ASSERT_EQUAL(build_trafo.work(pl, coord3d(5, 0, 0)), "Only one transformer per factory!")
	}

	// remove factory, should remove transformer and disconnect power nets
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 0, 0), coord3d(6, 0, 0), powerline, true), null)
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 2, 0), coord3d(6, 2, 0), powerline, true), null)

		ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(3, 0, 0)), null)

		ASSERT_EQUAL(tile_x(5, 1, 0).find_object(mo_transformer_s), null)
		ASSERT_EQUAL(tile_x(5, 1, 0).find_object(mo_transformer_c), null)

		ASSERT_FALSE(powerline_x(5, 0, 0).is_connected(powerline_x(5, 2, 0)))

		ASSERT_EQUAL(command_x(tool_remove_way).work(public_pl, coord3d(5, 0, 0), coord3d(6, 0, 0), "" + wt_power), null)
		ASSERT_EQUAL(command_x(tool_remove_way).work(public_pl, coord3d(5, 2, 0), coord3d(6, 2, 0), "" + wt_power), null)
	}

	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(0, 0, 0)), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_powerline_ways()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local wayremover = command_x(tool_remove_way)

	local powerline = way_desc_x.get_available_ways(wt_power, st_flat)[0]
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local tramway = way_desc_x.get_available_ways(wt_rail, st_tram)[0]

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 1, 0), coord3d(2, 3, 0), road, true), null)

	{
		// road-powerline crossing
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 2, 0), coord3d(3, 2, 0), powerline, true), null)
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"..4.....",
				"..5.....",
				"..1.....",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN(wt_power, coord3d(0, 0, 0),
			[
				"........",
				"........",
				".2A8....",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(wayremover.work(pl, coord3d(1, 2, 0), coord3d(3, 2, 0), "" + wt_power), null)
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"..4.....",
				"..5.....",
				"..1.....",
				"........",
				"........",
				"........",
				"........"
			])
	}

	{
		// build powerline ending on road
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 2, 0), coord3d(2, 2, 0), powerline, true), "")
		ASSERT_WAY_PATTERN(wt_power, coord3d(0, 0, 0),
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
		// build powerline over end of road
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 3, 0), coord3d(2, 3, 0), powerline, true), "")
		ASSERT_WAY_PATTERN(wt_power, coord3d(0, 0, 0),
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

		ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 3, 0), coord3d(3, 3, 0), powerline, true), "")
		ASSERT_WAY_PATTERN(wt_power, coord3d(0, 0, 0),
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
		// build powerline in the same direction as way below
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 0, 0), coord3d(2, 4, 0), powerline, true), "")
		ASSERT_WAY_PATTERN(wt_power, coord3d(0, 0, 0),
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

	// remove straight way, add diagonal way
	ASSERT_EQUAL(wayremover.work(pl, coord3d(2, 1, 0), coord3d(2, 3, 0), "" + wt_road), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(7, 0, 0), coord3d(0, 7, 0), road, true), null)

	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 4, 0), coord3d(6, 4, 0), powerline, true), "")
		ASSERT_WAY_PATTERN(wt_power, coord3d(0, 0, 0),
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

		// and the other direction
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 1, 0), coord3d(4, 6, 0), powerline, true), "")
		ASSERT_WAY_PATTERN(wt_power, coord3d(0, 0, 0),
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

	ASSERT_EQUAL(wayremover.work(pl, coord3d(0, 7, 0), coord3d(7, 0, 0), "" + wt_road), null)

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_powerline_remove_powerbridge()
{
	local pl = player_x(0)
	local power_bridge = bridge_desc_x.get_available_bridges(wt_power)[0]

	ASSERT_EQUAL(command_x(tool_setslope).work(pl, coord3d(2, 3, 0), "" + slope.east), null)
	ASSERT_EQUAL(command_x(tool_setslope).work(pl, coord3d(4, 3, 0), "" + slope.west), null)
	ASSERT_EQUAL(command_x.build_bridge(pl, coord3d(2, 3, 0), coord3d(4, 3, 0), power_bridge), null)

	// try removing the bridge from its center
	{
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 3, 1)), "Cannot delete a bridge from its centre")
		ASSERT_TRUE(tile_x(2, 3, 0).find_object(mo_bridge) != null)
	}

	// try removing the bridge from one of its ends
	{
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(2, 3, 0)), null)

		ASSERT_TRUE(tile_x(2, 3, 0).find_object(mo_bridge) == null)
		ASSERT_TRUE(tile_x(3, 3, 1).find_object(mo_bridge) == null)
		ASSERT_TRUE(tile_x(4, 3, 0).find_object(mo_bridge) == null)

		ASSERT_TRUE(tile_x(2, 3, 0).find_object(mo_powerline) != null)
		ASSERT_TRUE(tile_x(4, 3, 0).find_object(mo_powerline) != null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(2, 3, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 3, 0)), null)
	ASSERT_EQUAL(command_x(tool_setslope).work(pl, coord3d(2, 3, 0), "" + slope.flat), null)
	ASSERT_EQUAL(command_x(tool_setslope).work(pl, coord3d(4, 3, 0), "" + slope.flat), null)
	RESET_ALL_PLAYER_FUNDS()
}
