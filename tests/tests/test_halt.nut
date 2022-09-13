//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// test for halts/stops
//


function test_halt_build_rail_single_tile()
{
	local pl = player_x(0)
	local setslope = command_x(tool_setslope)
	local stationbuilder = command_x(tool_build_station)
	local wayremover = command_x(tool_remove_way)
	local road_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0] // road because it has double slopes available
	local station_desc = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x.passenger)[0] // FIXME: null instead of pax fails
	local bridge_desc = bridge_desc_x.get_available_bridges(wt_road)[0]

	// preconditions
	ASSERT_TRUE(road_desc != null)
	ASSERT_TRUE(station_desc != null)
	ASSERT_TRUE(bridge_desc != null)

	ASSERT_EQUAL(pl.get_current_maintenance(), 0)

	local pos = coord3d(4, 2, 0)

	{
		for (local sl = slope.flat; sl < slope.raised; ++sl) {
			ASSERT_EQUAL(setslope.work(pl, pos, "" + sl), sl != slope.flat ? null : "")
			ASSERT_EQUAL(stationbuilder.work(pl, pos, station_desc.get_name()), "No suitable way on the ground!")

			ASSERT_EQUAL(pl.get_current_maintenance(), 0)
			ASSERT_EQUAL(tile_x(pos.x, pos.y, pos.z).find_object(mo_building), null)
		}
	}

	ASSERT_EQUAL(setslope.work(pl, pos, "" + slope.flat), null)

	// cannot build on non-flat tile
	{
		for (local sl = slope.flat+1; sl < slope.raised; ++sl) {
			ASSERT_EQUAL(setslope.work(pl, pos, "" + sl), null)

			local d = slope.to_dir(sl)
			if (d != dir.none) { // only consider slopes we can build roads on
				RESET_ALL_PLAYER_FUNDS()

				local c = dir.to_coord(dir.backward(d))
				local adjacent = pos + coord3d(c.x, c.y, 0)
				ASSERT_EQUAL(command_x.build_way(pl, adjacent, pos, road_desc, true), null)
				local old_maintenance = pl.get_current_maintenance()

				ASSERT_EQUAL(stationbuilder.work(pl, pos, station_desc.get_name()), "No suitable way on the ground!")
				ASSERT_EQUAL(pl.get_current_maintenance(), old_maintenance)
				ASSERT_EQUAL(tile_x(pos.x, pos.y, pos.z).find_object(mo_building), null)

				ASSERT_EQUAL(wayremover.work(pl, pos, adjacent, "" + wt_road), null)
			}
		}
	}

	ASSERT_EQUAL(setslope.work(pl, pos, "" + slope.flat), null)

	// build on bridge
	{
		ASSERT_EQUAL(setslope.work(pl, coord3d(3, 2, 0), "" + slope.south), null)
		ASSERT_EQUAL(setslope.work(pl, coord3d(3, 4, 0), "" + slope.north), null)
		ASSERT_EQUAL(command_x(tool_build_bridge).work(pl, coord3d(3, 2, 0), bridge_desc.get_name()), null)

		local old_maintenance = pl.get_current_maintenance()

		ASSERT_EQUAL(stationbuilder.work(pl, coord3d(3, 3, 1), station_desc.get_name()), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 3, 1)), null)
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maintenance)

		// note z = 0 instead of 1
		ASSERT_EQUAL(stationbuilder.work(pl, coord3d(3, 2, 0), station_desc.get_name()), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 2, 0)), null)
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maintenance)

		// note z = 0 instead of 1
		ASSERT_EQUAL(stationbuilder.work(pl, coord3d(3, 4, 0), station_desc.get_name()), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 4, 0)), null)
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maintenance)

		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 4, 0)), null)
		ASSERT_EQUAL(setslope.work(pl, coord3d(3, 2, 0), "" + slope.flat), null)
		ASSERT_EQUAL(setslope.work(pl, coord3d(3, 4, 0), "" + slope.flat), null)

		ASSERT_EQUAL(pl.get_current_maintenance(), 0)
	}

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_halt_build_harbour()
{
	local pl = player_x(0)
	local lower = command_x(tool_lower_land)
	local raise = command_x(tool_raise_land)
	local stationbuilder = command_x(tool_build_station)
	local station_desc = building_desc_x.get_available_stations(building_desc_x.harbour, wt_water, good_desc_x.passenger)[0] // FIXME: null instead of pax fails
	local setclimate = command_x(tool_set_climate)

	// build harbour on flat land: should fail
	{
		ASSERT_EQUAL(stationbuilder.work(pl, coord3d(4, 3, 0), station_desc.get_name()), "Dock must be built on single slope!")
		ASSERT_EQUAL(tile_x(4, 3, 0).find_object(mo_building), null)
	}

	// build harbour on sloped land: should fail
	{
		ASSERT_EQUAL(lower.work(pl, coord3d(4, 3, 0)), null)
		ASSERT_EQUAL(lower.work(pl, coord3d(5, 3, 0)), null)

		ASSERT_EQUAL(stationbuilder.work(pl, coord3d(4, 3, 0), station_desc.get_name()), "")
		ASSERT_EQUAL(tile_x(4, 3, 0).find_object(mo_building), null)

		ASSERT_EQUAL(lower.work(pl, coord3d(4, 4, 0)), null)
		ASSERT_EQUAL(lower.work(pl, coord3d(5, 4, 0)), null)

		ASSERT_EQUAL(stationbuilder.work(pl, coord3d(4, 3, 0), station_desc.get_name()), "")
		ASSERT_EQUAL(tile_x(4, 3, 0).find_object(mo_building), null)
	}

	// build harbour on sloped land adjacent to water: Should succeed
	{
		ASSERT_EQUAL(setclimate.work(pl, coord3d(4, 3, -1), coord3d(4, 3, -1), "" + cl_water), null)
		ASSERT_EQUAL(stationbuilder.work(pl, coord3d(4, 2, -1), station_desc.get_name()), null)

		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 3, -1)), null)

		ASSERT_EQUAL(pl.get_current_maintenance(), 0)
		ASSERT_EQUAL(tile_x(4, 3, 0).find_object(mo_building), null)

		ASSERT_EQUAL(setclimate.work(pl, coord3d(4, 3, -1), coord3d(4, 3, -1), "" + cl_mediterran), null)
	}

	// clean up
	ASSERT_EQUAL(raise.work(pl, coord3d(4, 3, 0)), null)
	ASSERT_EQUAL(raise.work(pl, coord3d(5, 3, 0)), null)
	ASSERT_EQUAL(raise.work(pl, coord3d(4, 4, 0)), null)
	ASSERT_EQUAL(raise.work(pl, coord3d(5, 4, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_halt_build_air()
{
	local pl = player_x(0)
	local runway = way_desc_x.get_available_ways(wt_air, st_runway)[0]
	local taxiway = way_desc_x.get_available_ways(wt_air, st_flat)[0]
	local airhalt = building_desc_x("AirStop")

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 5, 0), coord3d(5, 7, 0), runway, true), null)

	// air halts must be on taxiways (contrary to the untranslated error message)
	{
		ASSERT_EQUAL(command_x.build_station(pl, coord3d(5, 5, 0), airhalt), "Flugzeughalt muss auf\nRunway liegen!\n")
		ASSERT_EQUAL(command_x.build_station(pl, coord3d(5, 6, 0), airhalt), "Flugzeughalt muss auf\nRunway liegen!\n")
		ASSERT_EQUAL(command_x.build_station(pl, coord3d(5, 7, 0), airhalt), "Flugzeughalt muss auf\nRunway liegen!\n")
	}

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 6, 0), coord3d(3, 6, 0), taxiway, true), null)

	// not in the middle of a taxiway
	{
		ASSERT_EQUAL(command_x.build_station(pl, coord3d(4, 6, 0), airhalt), "No terminal station here!")
	}

	// end of taxiway -> success
	{
		ASSERT_EQUAL(command_x.build_station(pl, coord3d(3, 6, 0), airhalt), null)

		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 6, 0)), null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 6, 0), coord3d(5, 6, 0), "" + wt_air), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(5, 5, 0), coord3d(5, 7, 0), "" + wt_air), null)
	RESET_ALL_PLAYER_FUNDS()
}

function test_halt_build_multi_tile()
{
	local pl = player_x(0)
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local station_desc = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x.passenger)[0] // FIXME: null instead of pax fails
	local bridge_desc = bridge_desc_x.get_available_bridges(wt_road)[0]

	// 2 adjacent tiles
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 1, 0), coord3d(2, 3, 0), road, true), null)
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 1, 0), coord3d(3, 3, 0), road, true), null)

		ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(2, 2, 0), station_desc.get_name()), null)
		ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(3, 2, 0), station_desc.get_name()), null)

		local halt = halt_x.get_halt(coord3d(2, 2, 0), pl)
		ASSERT_TRUE(halt != null)
		ASSERT_EQUAL(tile_x(2, 2, 0).get_halt().get_name(), tile_x(3, 2, 0).get_halt().get_name()) // check that this is really the same halt
		ASSERT_EQUAL(halt.get_owner().get_name(), pl.get_name())
		ASSERT_EQUAL(halt.is_connected(halt, good_desc_x.passenger), 0) // FIXME this should be 1
		ASSERT_TRUE(halt.accepts_good(good_desc_x.passenger))
		ASSERT_FALSE(halt.accepts_good(good_desc_x.mail))

		ASSERT_EQUAL(halt.get_arrived()[0], 0)
		ASSERT_EQUAL(halt.get_departed()[0], 0)
		ASSERT_EQUAL(halt.get_waiting()[0], 0)
		ASSERT_EQUAL(halt.get_happy()[0], 0)
		ASSERT_EQUAL(halt.get_unhappy()[0], 0)
		ASSERT_EQUAL(halt.get_noroute()[0], 0)
		ASSERT_EQUAL(halt.get_convoys()[0], 0)
		ASSERT_EQUAL(halt.get_walked()[0], 0)
		ASSERT_EQUAL(halt.get_convoy_list().get_count(), 0)
		ASSERT_EQUAL(halt.get_line_list().get_count(), 0)
		ASSERT_EQUAL(halt.get_factory_list().len(), 0)

		local tile_list = halt.get_tile_list()
		local expected_tiles = [ tile_x(2, 2, 0), tile_x(3, 2, 0) ]

		foreach (t in tile_list) {
			ASSERT_TRUE(t.find_object(mo_building) != null)

			// expected_tiles.find(t) != null won't work
			ASSERT_TRUE(expected_tiles.filter(@(idx, val) t.x == val.x && t.y == val.y && t.z == val.z).len() == 1)
		}

		ASSERT_EQUAL(halt.get_freight_to_dest(good_desc_x.passenger, coord(2, 2)), 0)
		ASSERT_EQUAL(halt.get_freight_to_halt(good_desc_x.passenger, halt), 0)

		// this also depends on the separate_halt_capacities setting
		ASSERT_EQUAL(halt.get_capacity(good_desc_x.passenger), 64)
		ASSERT_EQUAL(halt.get_capacity(good_desc_x.mail), 0)
		ASSERT_EQUAL(halt.get_capacity(good_desc_x("Kohle")), 0)
		ASSERT_EQUAL(halt.get_capacity(good_desc_x("nonexistent")), 0)

		ASSERT_EQUAL(halt.get_connections(good_desc_x.passenger).len(), 0)
		ASSERT_EQUAL(halt.get_connections(good_desc_x.mail).len(), 0)

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(2, 1, 0), coord3d(2, 3, 0), "" + wt_road), null)
		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 1, 0), coord3d(3, 3, 0), "" + wt_road), null)
		RESET_ALL_PLAYER_FUNDS()
	}

	// 2 tiles on top of each other
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 4, 0), coord3d(4, 4, 0), road, true), null)
		ASSERT_EQUAL(command_x(tool_build_bridge).work(pl, coord3d(3, 3, 0), coord3d(3, 5, 0), bridge_desc.get_name()), null)

		ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(3, 4, 0), station_desc.get_name()), null)
		ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(3, 4, 1), station_desc.get_name()), null)

		local lower_halt = halt_x.get_halt(coord3d(3, 4, 0), pl)
		local upper_halt = halt_x.get_halt(coord3d(3, 4, 1), pl)

		ASSERT_EQUAL(lower_halt.get_name(), upper_halt.get_name())
		ASSERT_EQUAL(lower_halt.get_tile_list().len(), 2)

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(2, 4, 0), coord3d(4, 4, 0), "" + wt_road), null)
		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 2, 0), coord3d(3, 6, 0), "" + wt_road), null)
	}

	RESET_ALL_PLAYER_FUNDS()

	local old_cash = pl.get_cash()[0]
	ASSERT_EQUAL(old_cash, 200*1000)

	// 16x16 station
	{
		for (local x = 0; x < 16; ++x) {
			ASSERT_EQUAL(command_x.build_way(pl, coord3d(x, 0, 0), coord3d(x, 15, 0), road, true), null)
			for (local y = 0; y < 16; ++y) {
				ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(x, y, 0), station_desc.get_name()), null)
			}
		}

		ASSERT_EQUAL(pl.get_current_maintenance(), 16*16*(road.get_maintenance() + station_desc.get_maintenance()))
		ASSERT_EQUAL(pl.get_cash()[0]*100, old_cash*100 - 16*16*(road.get_cost() + station_desc.get_cost()))

		local halt = halt_x.get_halt(coord3d(0, 0, 0), pl)
		ASSERT_TRUE(halt != null)
		ASSERT_TRUE(halt.accepts_good(good_desc_x.passenger))
		ASSERT_FALSE(halt.accepts_good(good_desc_x.mail))
		ASSERT_EQUAL(halt.is_connected(halt, good_desc_x.passenger), 0) // FIXME this should be 1
		ASSERT_EQUAL(halt.is_connected(halt, good_desc_x.mail), 0)

		for (local x = 0; x < 16; ++x) {
			ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(x, 0, 0), coord3d(x, 15, 0), "" + wt_road), null)
		}

		ASSERT_EQUAL(pl.get_current_maintenance(), 0)
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_halt_build_multi_mode()
{
	local pl = player_x(0)
	local road_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local rail_desc = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local pax_stop    = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x.passenger)[0]
	local pax_station = building_desc_x.get_available_stations(building_desc_x.station, wt_rail, good_desc_x.passenger)[0]

	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 3, 0), coord3d(4, 4, 0), road_desc, true), null)
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 3, 0), coord3d(5, 4, 0), rail_desc, true), null)

		ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 4, 0), pax_stop.get_name()), null)
		ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(5, 4, 0), pax_station.get_name()), null)

		ASSERT_EQUAL(halt_x.get_halt(coord3d(4, 4, 0), pl).get_name(), halt_x.get_halt(coord3d(5, 4, 0), pl).get_name())

		local halt = halt_x.get_halt(coord3d(4, 4, 0), pl)
		ASSERT_EQUAL(halt.get_capacity(good_desc_x.passenger), 64)
		ASSERT_TRUE(halt.accepts_good(good_desc_x.passenger))

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 4, 0), coord3d(4, 3, 0), "" + wt_road), null)
		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(5, 4, 0), coord3d(5, 3, 0), "" + wt_rail), null)
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_halt_build_multi_player()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local road_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local pax_halt  = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x.passenger)[0]

	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 4, 0), coord3d(3, 4, 0), road_desc, true), null)
		ASSERT_EQUAL(command_x(tool_build_station).work(pl,        coord3d(4, 4, 0), pax_halt.get_name()), null)
		ASSERT_EQUAL(command_x(tool_build_station).work(public_pl, coord3d(3, 4, 0), pax_halt.get_name()), null)

		ASSERT_TRUE(halt_x.get_halt(coord3d(3, 4, 0), pl) != null)
		ASSERT_EQUAL(halt_x.get_halt(coord3d(4, 4, 0), public_pl), null)

		local my_halt = halt_x.get_halt(coord3d(4, 4, 0), pl)
		local pub_halt = halt_x.get_halt(coord3d(3, 4, 0), public_pl)
		ASSERT_TRUE(my_halt != null)
		ASSERT_TRUE(pub_halt != null)

		ASSERT_TRUE(my_halt.get_name() != pub_halt.get_name())
		ASSERT_EQUAL(my_halt.get_tile_list().len(), 1)
		ASSERT_EQUAL(pub_halt.get_tile_list().len(), 1)

		ASSERT_EQUAL(command_x(tool_remove_way).work(public_pl, coord3d(4, 4, 0), coord3d(3, 4, 0), "" + wt_road), null)
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_halt_build_separate()
{
	local pl = player_x(0)
	local road_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local pax_halt  = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x.passenger)[0]

	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 4, 0), coord3d(3, 4, 0), road_desc, true), null)
		ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(5, 4, 0), pax_halt.get_name()), null)
		ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(3, 4, 0), pax_halt.get_name()), null)
		ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 4, 0), pax_halt.get_name()), null)

		local halt3 = halt_x.get_halt(coord3d(3, 4, 0), pl)
		local halt4 = halt_x.get_halt(coord3d(4, 4, 0), pl)
		local halt5 = halt_x.get_halt(coord3d(5, 4, 0), pl)

		ASSERT_TRUE(halt3 != null)
		ASSERT_TRUE(halt4 != null)
		ASSERT_TRUE(halt5 != null)

		ASSERT_TRUE(halt3.get_name() != halt5.get_name())
		ASSERT_TRUE(halt4.get_name() == halt5.get_name())
		ASSERT_TRUE(halt4.get_name() != halt3.get_name())

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(5, 4, 0), coord3d(3, 4, 0), "" + wt_road), null)
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_halt_build_near_factory()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local road_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local pax_halt     = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x.passenger)[0]
	local freight_halt = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x("Kohle"))[0]
	local remover = command_x(tool_remover)

	// build coal mine + coal power plant, then link them
	ASSERT_EQUAL(build_factory(pl, coord3d(0, 0, 0), 1, 1, 1024, "Kohlegrube"), null)

	{
		// also depends on station catchment area size
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 4, 0), coord3d(4, 3, 0), road_desc, true), null)
		ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 4, 0), pax_halt.get_name()), null)

		local halt = halt_x.get_halt(coord3d(4, 4, 0), pl)
		ASSERT_TRUE(halt != null)
		ASSERT_EQUAL(halt.get_factory_list().len(), 1)
		ASSERT_EQUAL(halt.get_factory_list()[0].get_name(), "Coal mine")

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 4, 0), coord3d(4, 3, 0), "" + wt_road), null)
	}

	ASSERT_EQUAL(remover.work(public_pl, coord3d(0, 0, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_halt_build_near_factories()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local road_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local pax_halt     = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x.passenger)[0]
	local freight_halt = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x("Kohle"))[0]
	local remover = command_x(tool_remover)

	// build coal mine + coal power plant, then link them
	ASSERT_EQUAL(build_factory(pl, coord3d(0, 0, 0), 1, 1, 1024, "Kohlegrube"), null)
	ASSERT_EQUAL(build_factory(pl, coord3d(6, 6, 0), 1, 1, 1024, "Kohlekraftwerk"), null)
	ASSERT_EQUAL(command_x(tool_link_factory).work(pl, coord3d(0, 0, 0), coord3d(6, 6, 0), ""), null)

	{
		// also depends on station catchment area size
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 4, 0), coord3d(4, 3, 0), road_desc, true), null)
		ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 4, 0), pax_halt.get_name()), null)

		local halt = halt_x.get_halt(coord3d(4, 4, 0), pl)
		ASSERT_TRUE(halt != null)
		ASSERT_EQUAL(halt.get_factory_list().len(), 2)
		ASSERT_EQUAL(halt.get_factory_list()[0].get_name(), "Coal power station")
		ASSERT_EQUAL(halt.get_factory_list()[1].get_name(), "Coal mine")

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 4, 0), coord3d(4, 3, 0), "" + wt_road), null)
	}

	// this only works with catchment area size of 4
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(0, 7, 0), coord3d(1, 7, 0), road_desc, true), null)
		ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(1, 7, 0), pax_halt.get_name()), null)

		local halt = halt_x.get_halt(coord3d(1, 7, 0), pl)
		ASSERT_TRUE(halt != null)
		ASSERT_EQUAL(halt.get_factory_list().len(), 0)

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(0, 7, 0), coord3d(1, 7, 0), "" + wt_road), null)
	}


	ASSERT_EQUAL(remover.work(public_pl, coord3d(0, 0, 0)), null)
	ASSERT_EQUAL(remover.work(public_pl, coord3d(6, 6, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_halt_build_on_tunnel_entrance()
{
	local pl = player_x(0)
	local rail_tunnel = tunnel_desc_x.get_available_tunnels(wt_rail)[0]
	local raise = command_x(tool_raise_land)
	local lower = command_x(tool_lower_land)
	local station_desc = building_desc_x.get_available_stations(building_desc_x.station, wt_rail, good_desc_x.passenger)[0]

	ASSERT_EQUAL(raise.work(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(raise.work(pl, coord3d(4, 3, 0)), null)
	ASSERT_EQUAL(raise.work(pl, coord3d(5, 2, 0)), null)
	ASSERT_EQUAL(raise.work(pl, coord3d(5, 3, 0)), null)

	ASSERT_EQUAL(command_x(tool_build_tunnel).work(pl, coord3d(4, 1, 0), rail_tunnel.get_name()), null)
	ASSERT_EQUAL(command_x(tool_build_tunnel).work(pl, coord3d(3, 2, 0), rail_tunnel.get_name()), null)
	ASSERT_EQUAL(command_x(tool_build_tunnel).work(pl, coord3d(5, 2, 0), rail_tunnel.get_name()), null)

	// Building station on tunnel entrance fails (contrary to depots)
	{
		for (local d = dir.north; d < dir.all; d = d*2) {
			local p = coord3d(4, 2, 0) + dir.to_coord(d)

			ASSERT_EQUAL(command_x.build_station(pl, p, station_desc), "No suitable way on the ground!")
		}
	}

	local remover = command_x(tool_remover)
	remover.set_flags(2)
	ASSERT_EQUAL(remover.work(pl, coord3d(4, 1, 0)), null)

	ASSERT_EQUAL(lower.work(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(lower.work(pl, coord3d(4, 3, 0)), null)
	ASSERT_EQUAL(lower.work(pl, coord3d(5, 2, 0)), null)
	ASSERT_EQUAL(lower.work(pl, coord3d(5, 3, 0)), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_halt_build_on_bridge_end()
{
	local pl = player_x(0)
	local rail_bridge = bridge_desc_x.get_available_bridges(wt_rail)[0]
	local setslope = command_x(tool_setslope)
	local station_desc = building_desc_x.get_available_stations(building_desc_x.station, wt_rail, good_desc_x.passenger)[0]

	// north-south direction
	{
		ASSERT_EQUAL(setslope.work(pl, coord3d(4, 2, 0), "" + slope.south), null)
		ASSERT_EQUAL(setslope.work(pl, coord3d(4, 4, 0), "" + slope.north), null)

		ASSERT_EQUAL(command_x(tool_build_bridge).work(pl, coord3d(4, 2, 0), rail_bridge.get_name()), null)
		ASSERT_EQUAL(command_x.build_station(pl, coord3d(4, 2, 0), station_desc), null)
		ASSERT_EQUAL(command_x.build_station(pl, coord3d(4, 4, 0), station_desc), null)

		// tool_remover removes halt first, then bridge, then depot (depot is automatically removed when destroying bridge)
		// So here we have to remove things on the tile twice (contrary to test_depot_build_on_bridge_end)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)

		ASSERT_EQUAL(setslope.work(pl, coord3d(4, 2, 0), "" + slope.flat), null)
		ASSERT_EQUAL(setslope.work(pl, coord3d(4, 4, 0), "" + slope.flat), null)
	}

	// east-west direction
	{
		ASSERT_EQUAL(setslope.work(pl, coord3d(3, 3, 0), "" + slope.east), null)
		ASSERT_EQUAL(setslope.work(pl, coord3d(5, 3, 0), "" + slope.west), null)

		ASSERT_EQUAL(command_x(tool_build_bridge).work(pl, coord3d(3, 3, 0), rail_bridge.get_name()), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(3, 3, 0), get_depot_by_wt(wt_rail)), null)
		ASSERT_EQUAL(command_x.build_depot(pl, coord3d(5, 3, 0), get_depot_by_wt(wt_rail)), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 3, 0)), null)

		ASSERT_EQUAL(setslope.work(pl, coord3d(3, 3, 0), "" + slope.flat), null)
		ASSERT_EQUAL(setslope.work(pl, coord3d(5, 3, 0), "" + slope.flat), null)
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_halt_build_on_depot()
{
	local pl = player_x(0)
	local rail_desc = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local station_desc = building_desc_x.get_available_stations(building_desc_x.station, wt_rail, good_desc_x.passenger)[0]
	local depot = get_depot_by_wt(wt_rail)

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), rail_desc, true), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 2, 0), depot), null)

	{
		ASSERT_EQUAL(command_x.build_station(pl, coord3d(4, 2, 0), station_desc), "No suitable ground!")
	}

	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_halt_build_station_extension()
{
	local pl = player_x(0)
	local setslope = command_x(tool_setslope)
	local stationbuilder = command_x(tool_build_station)
	local wayremover = command_x(tool_remove_way)
	local road_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0] // road because it has double slopes available
	local station_desc = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x.passenger)[0] // FIXME: null instead of pax fails
	local stext_desc = building_desc_x.get_available_stations(building_desc_x.station_extension, wt_rail, good_desc_x.passenger)[0]
	local bridge_desc = bridge_desc_x.get_available_bridges(wt_road)[0]

	ASSERT_TRUE(station_desc != null)
	ASSERT_TRUE(stext_desc != null)
	ASSERT_TRUE(bridge_desc != null)
	ASSERT_TRUE(road_desc != null)

	// build station extension without station: should fail
	{
		ASSERT_EQUAL(stationbuilder.work(pl, coord3d(4, 3, 0), stext_desc.get_name()), "Post muss neben\nHaltestelle\nliegen!\n")

		ASSERT_EQUAL(pl.get_current_maintenance(), 0)
		ASSERT_EQUAL(tile_x(4, 3, 0).find_object(mo_building), null)
	}

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), road_desc, true), null)
	ASSERT_EQUAL(stationbuilder.work(pl, coord3d(4, 3, 0), station_desc.get_name()), null)
	local old_maintenance = pl.get_current_maintenance()

	// build station extension next to station: should succeed
	{
		ASSERT_EQUAL(stationbuilder.work(pl, coord3d(5, 3, 0), stext_desc.get_name()), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(5, 3, 0)), null)
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maintenance)

		// and diagonal
		ASSERT_EQUAL(stationbuilder.work(pl, coord3d(5, 2, 0), stext_desc.get_name()), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(5, 2, 0)), null)
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maintenance)
	}

	// build station extension on raised tile: should succeed
	{
		ASSERT_EQUAL(setslope.work(pl, coord3d(5, 3, 0), "" + slope.all_up_slope), null)
		ASSERT_EQUAL(setslope.work(pl, coord3d(5, 3, 1), "" + slope.all_up_slope), null)
		ASSERT_EQUAL(setslope.work(pl, coord3d(5, 3, 2), "" + slope.all_up_slope), null)

		ASSERT_EQUAL(stationbuilder.work(pl, coord3d(5, 3, 3), stext_desc.get_name()), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(5, 3, 3)), null)
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maintenance)

		// up slope is automatically removed
		ASSERT_EQUAL(square_x(5, 3).get_tile_at_height(1), null)
		ASSERT_EQUAL(square_x(5, 3).get_tile_at_height(2), null)
	}

	{
		// diagonal too
		ASSERT_EQUAL(setslope.work(pl, coord3d(5, 2, 0), "" + slope.all_up_slope), null)
		ASSERT_EQUAL(setslope.work(pl, coord3d(5, 2, 1), "" + slope.all_up_slope), null)

		ASSERT_EQUAL(stationbuilder.work(pl, coord3d(5, 2, 2), stext_desc.get_name()), null)
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(5, 2, 2)), null)
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maintenance)

		// up slope is automatically removed
		ASSERT_EQUAL(square_x(5, 3).get_tile_at_height(1), null)
		ASSERT_EQUAL(square_x(5, 3).get_tile_at_height(2), null)
	}

	ASSERT_EQUAL(wayremover.work(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_road), null)
	ASSERT_EQUAL(pl.get_current_maintenance(), 0)

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_halt_upgrade_downgrade()
{
	local pl = player_x(0)
	local stationbuilder = command_x(tool_build_station)
	local wayremover = command_x(tool_remove_way)
	local road_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local pax_halts  = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x.passenger)
	pax_halts.sort(@(a, b) a.get_capacity() <=> b.get_capacity())

	ASSERT_TRUE(pax_halts.len() >= 2)
	local small_halt = pax_halts[0]
	local big_halt = pax_halts[1]
	local pos = coord3d(3, 3, 0)

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 2, 0), coord3d(3, 4, 0), road_desc, true), null)

	// upgrade station
	{
		ASSERT_EQUAL(stationbuilder.work(pl, pos, small_halt.get_name()), null)
		local halt = halt_x.get_halt(pos, pl)
		ASSERT_TRUE(halt != null)
		local old_capacity = halt.get_capacity(good_desc_x.passenger)
		local old_maint = pl.get_current_maintenance()
		ASSERT_EQUAL(stationbuilder.work(pl, pos, big_halt.get_name()), null)

		ASSERT_TRUE(halt.is_valid())
		ASSERT_TRUE(halt.accepts_good(good_desc_x.passenger))
		ASSERT_TRUE(halt.get_capacity(good_desc_x.passenger) > old_capacity)
		ASSERT_EQUAL(halt.get_tile_list().len(), 1)
		ASSERT_TRUE(pl.get_current_maintenance() > old_maint)
		ASSERT_EQUAL(pl.get_current_maintenance() - old_maint, big_halt.get_maintenance() - small_halt.get_maintenance())
	}

	// upgrade station to same level
	{
		local halt = halt_x.get_halt(pos, pl)
		local old_capacity = halt.get_capacity(good_desc_x.passenger)
		local old_maint = pl.get_current_maintenance()
		local old_cash = pl.get_current_cash()
		ASSERT_EQUAL(stationbuilder.work(pl, coord3d(3, 3, 0), big_halt.get_name()), null)

		ASSERT_TRUE(halt.is_valid())
		ASSERT_TRUE(halt.accepts_good(good_desc_x.passenger))
		ASSERT_EQUAL(halt.get_capacity(good_desc_x.passenger), old_capacity)
		ASSERT_EQUAL(halt.get_tile_list().len(), 1)
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint)
	}

	// downgrade without ctrl, should fail
	{
		local halt = halt_x.get_halt(pos, pl)
		local old_capacity = halt.get_capacity(good_desc_x.passenger)
		local old_maint = pl.get_current_maintenance()
		local old_cash = pl.get_current_cash()
		ASSERT_EQUAL(stationbuilder.work(pl, coord3d(3, 3, 0), small_halt.get_name()), "Upgrade must have\na higher level")

		ASSERT_TRUE(halt.is_valid())
		ASSERT_TRUE(halt.accepts_good(good_desc_x.passenger))
		ASSERT_EQUAL(halt.get_capacity(good_desc_x.passenger), old_capacity)
		ASSERT_EQUAL(halt.get_tile_list().len(), 1)
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint)
	}

	// downgrade with ctrl
	{
		local halt = halt_x.get_halt(pos, pl)
		local old_capacity = halt.get_capacity(good_desc_x.passenger)
		local old_maint = pl.get_current_maintenance()
		local old_cash = pl.get_current_cash()

		stationbuilder.set_flags(2) // ctrl
		ASSERT_EQUAL(stationbuilder.work(pl, coord3d(3, 3, 0), small_halt.get_name()), null)

		ASSERT_TRUE(halt.is_valid())
		ASSERT_TRUE(halt.accepts_good(good_desc_x.passenger))
		ASSERT_TRUE(halt.get_capacity(good_desc_x.passenger) < old_capacity)
		ASSERT_EQUAL(halt.get_tile_list().len(), 1)
		ASSERT_TRUE(pl.get_current_maintenance() < old_maint)
		ASSERT_EQUAL(pl.get_current_maintenance() - old_maint, small_halt.get_maintenance() - big_halt.get_maintenance())
	}

	// clean up
	ASSERT_EQUAL(wayremover.work(pl, coord3d(3, 2, 0), coord3d(3, 4, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_halt_make_public_single()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local stationbuilder = command_x(tool_build_station)
	local wayremover = command_x(tool_remove_way)
	local pax_halt   = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x.passenger)[0]
	local road_desc  = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local makepublic = command_x(tool_make_stop_public)

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), road_desc, true), null)
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(4, 3, 0), pax_halt), null)

	// invalid pos
	{
		local old_cash = pl.get_current_cash()
		local old_maint = pl.get_current_maintenance()

		ASSERT_EQUAL(makepublic.work(pl, coord3d(-1, -1, -1)), "No suitable ground!")

		ASSERT_EQUAL(pl.get_current_cash(), old_cash)
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint)
	}

	// no stop under cursor
	{
		local old_cash = pl.get_current_cash()
		local old_maint = pl.get_current_maintenance()

		ASSERT_EQUAL(makepublic.work(pl, coord3d(3, 2, 0)), null)

		ASSERT_EQUAL(pl.get_current_cash(), old_cash)
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint)
	}

	// valid stop - Making the stop public also makes the road under it public
	{
		local old_cash = pl.get_current_cash()
		local old_maint = pl.get_current_maintenance()

		ASSERT_EQUAL(makepublic.work(pl, coord3d(4, 3, 0)), null)
		ASSERT_TRUE(halt_x.get_halt(coord3d(4, 3, 0), public_pl) != null)
		ASSERT_EQUAL(halt_x.get_halt(coord3d(4, 3, 0), public_pl).get_owner().get_name(), public_pl.get_name())

		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint - 1*pax_halt.get_maintenance() - 1*road_desc.get_maintenance())
		ASSERT_EQUAL(pl.get_current_cash()*100, old_cash*100 - 60 * (1*pax_halt.get_maintenance() + 1*road_desc.get_maintenance()) )
		ASSERT_EQUAL(public_pl.get_current_maintenance(), 1*pax_halt.get_maintenance() + 1*road_desc.get_maintenance())
	}

	// already public - should have no effect
	{
		local old_cash  = pl.get_current_cash()
		local old_maint = pl.get_current_maintenance()

		ASSERT_EQUAL(makepublic.work(pl, coord3d(4, 3, 0)), null)
		ASSERT_TRUE(halt_x.get_halt(coord3d(4, 3, 0), public_pl) != null)
		ASSERT_EQUAL(halt_x.get_halt(coord3d(4, 3, 0), public_pl).get_owner().get_name(), public_pl.get_name())

		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint)
		ASSERT_EQUAL(pl.get_current_cash(), old_cash)
	}

	// same for public player
	{
		local old_cash  = public_pl.get_current_cash()
		local old_maint = public_pl.get_current_maintenance()

		ASSERT_EQUAL(makepublic.work(public_pl, coord3d(4, 3, 0)), null)
		ASSERT_TRUE(halt_x.get_halt(coord3d(4, 3, 0), public_pl) != null)

		ASSERT_EQUAL(public_pl.get_current_maintenance(), old_maint)
		ASSERT_EQUAL(public_pl.get_current_cash(), old_cash)
	}

	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(4, 3, 0)), null)
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(4, 3, 0), pax_halt), null)

	// If public player uses this tool, he pays for it
	{
		local old_cash  = public_pl.get_current_cash()
		local old_maint = public_pl.get_current_maintenance()

		ASSERT_EQUAL(makepublic.work(pl, coord3d(4, 3, 0)), null)

		ASSERT_TRUE(halt_x.get_halt(coord3d(4, 3, 0), public_pl) != null)
		ASSERT_EQUAL(halt_x.get_halt(coord3d(4, 3, 0), public_pl).get_owner().get_name(), public_pl.get_name())

		// only halt maintenance because the road is already public
		ASSERT_EQUAL(public_pl.get_current_cash()*100, 100*old_cash + 60 * pax_halt.get_maintenance())
		ASSERT_EQUAL(public_pl.get_current_maintenance(), old_maint + pax_halt.get_maintenance())
	}

	ASSERT_EQUAL(wayremover.work(public_pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_road), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), road_desc, true), null)
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(4, 3, 0), pax_halt), null)

	// Public player can make halts of other players public even if road underneath is not public
	{
		local old_cash  = public_pl.get_current_cash()
		local old_maint = public_pl.get_current_maintenance()

		ASSERT_EQUAL(makepublic.work(pl, coord3d(4, 3, 0)), null)

		ASSERT_TRUE(halt_x.get_halt(coord3d(4, 3, 0), public_pl) != null)
		ASSERT_EQUAL(halt_x.get_halt(coord3d(4, 3, 0), public_pl).get_owner().get_name(), public_pl.get_name())

		// halt + road maintenance
		ASSERT_EQUAL(public_pl.get_current_cash()*100, 100*old_cash + 60 * pax_halt.get_maintenance())
		ASSERT_EQUAL(public_pl.get_current_maintenance(), old_maint + pax_halt.get_maintenance() + road_desc.get_maintenance())
	}

	ASSERT_EQUAL(wayremover.work(public_pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_halt_make_public_multi_tile()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local stationbuilder = command_x(tool_build_station)
	local wayremover = command_x(tool_remove_way)
	local pax_halt   = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x.passenger)[0]
	local road_desc  = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local makepublic = command_x(tool_make_stop_public)

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 2, 0), coord3d(4, 5, 0), road_desc, true), null)
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(4, 3, 0), pax_halt), null)
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(4, 4, 0), pax_halt), null)

	// valid stop - Making the stop public also makes the road under it public
	{
		local old_cash = pl.get_current_cash()
		local old_maint = pl.get_current_maintenance()

		ASSERT_EQUAL(makepublic.work(pl, coord3d(4, 3, 0)), null)
		ASSERT_TRUE(halt_x.get_halt(coord3d(4, 3, 0), public_pl) != null)
		ASSERT_EQUAL(halt_x.get_halt(coord3d(4, 3, 0), public_pl).get_owner().get_name(), public_pl.get_name())

		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint - 2*pax_halt.get_maintenance() - 2*road_desc.get_maintenance())
		ASSERT_EQUAL(pl.get_current_cash()*100, old_cash*100 - 60 * (2*pax_halt.get_maintenance() + 2*road_desc.get_maintenance()) )
		ASSERT_EQUAL(public_pl.get_current_maintenance(), 2*pax_halt.get_maintenance() + 2*road_desc.get_maintenance())
	}

	// already public - should have no effect
	{
		local old_cash  = pl.get_current_cash()
		local old_maint = pl.get_current_maintenance()

		ASSERT_EQUAL(makepublic.work(pl, coord3d(4, 3, 0)), null)
		ASSERT_TRUE(halt_x.get_halt(coord3d(4, 3, 0), public_pl) != null)
		ASSERT_EQUAL(halt_x.get_halt(coord3d(4, 3, 0), public_pl).get_owner().get_name(), public_pl.get_name())

		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint)
		ASSERT_EQUAL(pl.get_current_cash(), old_cash)
	}

	// same for public player
	{
		local old_cash  = public_pl.get_current_cash()
		local old_maint = public_pl.get_current_maintenance()

		ASSERT_EQUAL(makepublic.work(public_pl, coord3d(4, 3, 0)), null)
		ASSERT_TRUE(halt_x.get_halt(coord3d(4, 3, 0), public_pl) != null)

		ASSERT_EQUAL(public_pl.get_current_maintenance(), old_maint)
		ASSERT_EQUAL(public_pl.get_current_cash(), old_cash)
	}

	ASSERT_EQUAL(wayremover.work(public_pl, coord3d(4, 2, 0), coord3d(4, 5, 0), "" + wt_road), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 2, 0), coord3d(4, 5, 0), road_desc, true), null)
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(4, 3, 0), pax_halt), null)
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(4, 4, 0), pax_halt), null)

	// If public player uses this tool, he pays for it
	{
		local old_cash  = public_pl.get_current_cash()
		local old_maint = public_pl.get_current_maintenance()

		ASSERT_EQUAL(makepublic.work(pl, coord3d(4, 3, 0)), null)

		ASSERT_TRUE(halt_x.get_halt(coord3d(4, 3, 0), public_pl) != null)
		ASSERT_EQUAL(halt_x.get_halt(coord3d(4, 3, 0), public_pl).get_owner().get_name(), public_pl.get_name())

		ASSERT_EQUAL(public_pl.get_current_cash()*100, 100*old_cash + 60 * 2*pax_halt.get_maintenance())
		ASSERT_EQUAL(public_pl.get_current_maintenance(), old_maint + (2*pax_halt.get_maintenance() + 2*road_desc.get_maintenance()))
	}

	ASSERT_EQUAL(wayremover.work(public_pl, coord3d(4, 2, 0), coord3d(4, 5, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_halt_make_public_underground()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local tunnel_desc = tunnel_desc_x.get_available_tunnels(wt_road)[0]
	local pax_halt    = building_desc_x("LongBusStop")
	local makepublic  = command_x(tool_make_stop_public)
	local raise       = command_x(tool_raise_land)
	local lower       = command_x(tool_lower_land)
	local stationbuilder = command_x(tool_build_station)

	ASSERT_EQUAL(raise.work(public_pl, coord3d(4, 3, 0)), null)
	ASSERT_EQUAL(raise.work(public_pl, coord3d(5, 3, 0)), null)
	ASSERT_EQUAL(raise.work(public_pl, coord3d(4, 4, 0)), null)
	ASSERT_EQUAL(raise.work(public_pl, coord3d(5, 4, 0)), null)

	ASSERT_EQUAL(command_x(tool_build_tunnel).work(pl, coord3d(4, 2, 0), tunnel_desc.get_name()), null)
	ASSERT_EQUAL(stationbuilder.work(pl, coord3d(4, 3, 0), pax_halt.get_name()), null)

	// valid underground stop
	{
		local old_cash = pl.get_current_cash()
		local old_maint = pl.get_current_maintenance()

		ASSERT_EQUAL(makepublic.work(pl, coord3d(4, 3, 0)), null)
		ASSERT_TRUE(halt_x.get_halt(coord3d(4, 3, 0), public_pl) != null)
		ASSERT_EQUAL(halt_x.get_halt(coord3d(4, 3, 0), public_pl).get_owner().get_name(), public_pl.get_name())

		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint - 1*pax_halt.get_maintenance() - 1*tunnel_desc.get_maintenance())
		ASSERT_EQUAL(pl.get_current_cash()*100, old_cash*100 - 60 * (1*pax_halt.get_maintenance() + 1*tunnel_desc.get_maintenance()) )
		ASSERT_EQUAL(public_pl.get_current_maintenance(), 1*pax_halt.get_maintenance() + 1*tunnel_desc.get_maintenance())
	}

	ASSERT_EQUAL(command_x(tool_remove_way).work(public_pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_road), null)

	ASSERT_EQUAL(lower.work(public_pl, coord3d(4, 3, 0)), null)
	ASSERT_EQUAL(lower.work(public_pl, coord3d(5, 3, 0)), null)
	ASSERT_EQUAL(lower.work(public_pl, coord3d(4, 4, 0)), null)
	ASSERT_EQUAL(lower.work(public_pl, coord3d(5, 4, 0)), null)
}


function test_halt_move_stop_invalid_param()
{
	// out of map limits
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_stop_mover).work(player_x(0), coord3d(-1, -1, 0), coord3d(-1, -1, 0)), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Tool has no effects")
		}
		ASSERT_TRUE(error_caught)
	}

	// no way
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_stop_mover).work(player_x(0), coord3d(1, 1, 0), coord3d(3, 1, 0)), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Tool has no effects")
		}
		ASSERT_TRUE(error_caught)
	}

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}
