//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for building, rotating and demolishing houses
//

function test_building_build_house_invalid_param()
{
	local pl = player_x(0)

	// Invalid pos: error
	{
		ASSERT_EQUAL(command_x(tool_build_house).work(pl, coord3d(-1, 1, 0)), "")
	}

	// empty default_param: error
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_build_house).work(pl, coord3d(0, 0, 0), ""), null)
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
		ASSERT_TRUE(error_caught)
	}

	// Invalid default_param: error
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_build_house).work(pl, coord3d(0, 0, 0), "0"), null)
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
		ASSERT_TRUE(error_caught)
	}

	// Invalid default_param: error
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_build_house).work(pl, coord3d(0, 0, 0), "1"), null)
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
		ASSERT_TRUE(error_caught)
	}

	// Invalid default_param: error
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_build_house).work(pl, coord3d(0, 0, 0), "42"), null)
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
		ASSERT_TRUE(error_caught)
	}
}


function test_building_build_house_random()
{
	local public_pl = player_x(1)

	// no default_param: random building
	// built by player, owned by pubic player
	{
		ASSERT_EQUAL(command_x(tool_build_house).work(player_x(0), coord3d(0, 0, 0)), null)

		local b = building_x(0, 0, 0)
		ASSERT_EQUAL(b.get_owner().get_name(), public_pl.get_name())

		ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(0, 0, 0)), null)

		// check that foundations are removed
		ASSERT_TRUE(tile_x(0, 0, 0).is_empty())
		ASSERT_TRUE(tile_x(0, 1, 0).is_empty())
		ASSERT_TRUE(tile_x(1, 0, 0).is_empty())
		ASSERT_TRUE(tile_x(1, 1, 0).is_empty())
	}
}


function test_building_build_house_valid_desc()
{
	local public_pl = player_x(1)

	// Valid default_param: Build specific building
	{
		ASSERT_EQUAL(command_x(tool_build_house).work(public_pl, coord3d(0, 0, 0), "1#RUIN_0"), null)
		local b = building_x(0, 0, 0)
		ASSERT_EQUAL(b.get_owner().get_name(), public_pl.get_name())
		ASSERT_EQUAL(b.get_desc().get_name(), "RUIN_0")

		ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(0, 0, 0)), null)

		// check that foundations are removed
		ASSERT_TRUE(tile_x(0, 0, 0).is_empty())
		ASSERT_TRUE(tile_x(0, 1, 0).is_empty())
		ASSERT_TRUE(tile_x(1, 0, 0).is_empty())
		ASSERT_TRUE(tile_x(1, 1, 0).is_empty())
	}

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_building_build_house_invalid_desc()
{
	local public_pl = player_x(1)

	// Valid default_param: Build specific building
	{
		ASSERT_EQUAL(command_x(tool_build_house).work(public_pl, coord3d(0, 0, 0), "1#nonexistent"), "")
		ASSERT_EQUAL(tile_x(0,0,0).find_object(mo_building), null)
	}

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_building_build_house_auto_rotation_attraction()
{
	local public_pl = player_x(1)

	// TODO: Actually check rotation
	{
		ASSERT_EQUAL(command_x(tool_build_house).work(public_pl, coord3d(0, 0, 0), "1ARUIN_0"), null)
		local b = building_x(0, 0, 0)
		ASSERT_EQUAL(b.get_owner().get_name(), public_pl.get_name())
		ASSERT_EQUAL(b.get_desc().get_name(), "RUIN_0")

		ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(0, 0, 0)), null)

		// check that foundations are removed
		ASSERT_TRUE(tile_x(0, 0, 0).is_empty())
		ASSERT_TRUE(tile_x(0, 1, 0).is_empty())
		ASSERT_TRUE(tile_x(1, 0, 0).is_empty())
		ASSERT_TRUE(tile_x(1, 1, 0).is_empty())
	}

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_building_build_house_auto_rotation_citybuilding()
{
	local public_pl = player_x(1)

	// TODO: Actually check rotation
	{
		ASSERT_EQUAL(command_x(tool_build_house).work(public_pl, coord3d(0, 0, 0), "1ARES_01_23"), null)
		local b = building_x(0, 0, 0)
		ASSERT_EQUAL(b.owner.nr, 16)
		ASSERT_EQUAL(b.desc.name, "RES_01_23")
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(0, 0, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_building_build_multi_tile_sloped()
{
	local pl = player_x(1)
	local remover = command_x(tool_remover)
	local raise = command_x(tool_raise_land)
	local lower = command_x(tool_lower_land)
	local builder = command_x(tool_build_house)

	local building_desc = building_desc_x("STADIUM2") // 3x2 size

	ASSERT_EQUAL(raise.work(pl, coord3d(4, 2, 0)), null)

	{
		ASSERT_EQUAL(builder.work(pl, coord3d(3, 1, 0), "11" + building_desc.get_name()), null)
		ASSERT_EQUAL(remover.work(pl, coord3d(3, 1, 0)), null)

		ASSERT_EQUAL(tile_x(3, 1, 0).get_slope(), slope.southeast)
		ASSERT_EQUAL(tile_x(4, 1, 0).get_slope(), slope.southwest)
		ASSERT_EQUAL(tile_x(3, 2, 0).get_slope(), slope.northeast)
		ASSERT_EQUAL(tile_x(4, 2, 0).get_slope(), slope.northwest)
	}

	ASSERT_EQUAL(lower.work(pl, coord3d(4, 2, 1)), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_building_buy_house_invalid_param()
{
	local pl = player_x(0)

	// outside of map limits
	{
		ASSERT_EQUAL(command_x(tool_buy_house).work(pl, coord3d(-1,-1,-1)), "")
	}

	// cannot buy house on flat land
	{
		ASSERT_EQUAL(command_x(tool_buy_house).work(pl, coord3d(0,0,0)), "")
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_building_buy_house_from_public_player()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local building_desc = building_desc_x("RES_01_23") // does not matter which of res,com,ind
	local old_cash = pl.get_cash()[0]
	local old_maint = pl.get_current_maintenance()

	ASSERT_EQUAL(command_x(tool_build_house).work(public_pl, coord3d(0,0,0), "11" + building_desc.get_name()), null)
	ASSERT_EQUAL(building_x(0,0,0).get_owner().nr, 16) // building is unowned

	{
		ASSERT_EQUAL(command_x(tool_buy_house).work(pl, coord3d(0,0,0)), null)
		ASSERT_EQUAL(building_x(0,0,0).get_owner().nr, pl.nr)
		ASSERT_GREATER(pl.get_current_maintenance(), old_maint)
		ASSERT_LESS(pl.get_cash()[0], old_cash)
	}

	old_cash = pl.get_cash()[0]
	old_maint = pl.get_current_maintenance()

	// public player cannot buy buildings
	{
		ASSERT_EQUAL(command_x(tool_buy_house).work(public_pl, coord3d(0,0,0)), "")
		ASSERT_EQUAL(building_x(0,0,0).get_owner().nr, pl.nr)
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint)
		ASSERT_EQUAL(pl.get_cash()[0], old_cash)
	}

	// cannot buy from myself
	{
		ASSERT_EQUAL(command_x(tool_buy_house).work(pl, coord3d(0,0,0)), "")
		ASSERT_EQUAL(building_x(0,0,0).get_owner().nr, pl.nr)
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint)
		ASSERT_EQUAL(pl.get_cash()[0], old_cash)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(0,0,0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_building_buy_house_attraction()
{
	local pl = player_x(0)
	local public_pl = player_x(1)

	local building_desc = building_desc_x("STADIUM2")
	local builder = command_x(tool_build_house)

	// build stadium using public player
	ASSERT_EQUAL(builder.work(public_pl, coord3d(0,0,0), "11" + building_desc.get_name()), null)

	{
		ASSERT_EQUAL(command_x(tool_buy_house).work(pl, coord3d(0,0,0)), "Das Feld gehoert\neinem anderen Spieler\n")
		ASSERT_EQUAL(building_x(0,0,0).get_owner().nr, public_pl.nr)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(0,0,0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_building_rotate_house()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local builder = command_x(tool_build_house)
	local rotator = command_x(tool_rotate_building)
	local remover = command_x(tool_remover)

	// invalid coordinate
	{
		ASSERT_EQUAL(rotator.work(pl, coord3d(-1, -1, 0)), "")
	}

	// valid coordinate, but no house to rotate
	{
		ASSERT_EQUAL(rotator.work(pl, coord3d(0, 0, 0)), null)
	}

	ASSERT_EQUAL(builder.work(public_pl, coord3d(0, 0, 0), "01A4_SCHLOSS2"), null)

	// cannot rotate buildings owned by other players
	{
		ASSERT_EQUAL(rotator.work(pl, coord3d(0, 0, 0)), "Das Feld gehoert\neinem anderen Spieler\n")
	}

	// rotate single-tile house
	{
		ASSERT_EQUAL(rotator.work(public_pl, coord3d(0, 0, 0)), null)
	}

	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(0, 0, 0)), null)
	ASSERT_EQUAL(builder.work(public_pl, coord3d(0, 0, 0), "01RUIN_0"), null)

	// rotate multi-tile house
	{
		ASSERT_EQUAL(rotator.work(public_pl, coord3d(0, 0, 0)), null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(0, 0, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_building_rotate_harbour()
{
	local pl = player_x(0)
	local setslope = command_x(tool_setslope)
	local setclimate = command_x(tool_set_climate)
	local harbours = building_desc_x.get_building_list(building_desc_x.harbour)
	harbours = harbours.filter(@(idx, val) val.get_type() == building_desc_x.harbour)
	local harbour = harbours[0]
	local stationbuilder = command_x(tool_build_station)
	local rotator = command_x(tool_rotate_building)
	local remover = command_x(tool_remover)

	ASSERT_EQUAL(setclimate.work(pl, coord3d(4, 2, 0), coord3d(5, 2, 0), "" + cl_water), null)
	ASSERT_EQUAL(setslope.work(pl, coord3d(3, 2, 0), "" + slope.east), null)
	ASSERT_EQUAL(stationbuilder.work(pl, coord3d(3, 2, 0), harbour.get_name()), null)

	{
		ASSERT_EQUAL(rotator.work(pl, coord3d(3, 2, 0)), "Cannot rotate this building!")
	}

	// clean up
	ASSERT_EQUAL(remover.work(pl, coord3d(3, 2, 0)), null)
	ASSERT_EQUAL(setslope.work(pl, coord3d(3, 2, 0), "" + slope.flat), null)
	ASSERT_EQUAL(setclimate.work(pl, coord3d(4, 2, 0), coord3d(5, 2, 0), "" + cl_mediterran), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_building_rotate_station()
{
	local pl = player_x(0)
	local rotator = command_x(tool_rotate_building)
	local wayremover = command_x(tool_remove_way)
	local stationbuilder = command_x(tool_build_station)
	local rail_desc = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local layout2_station = building_desc_x("SandStationMHz") // this one has only 2 layouts
	local layout16_station = building_desc_x("GCGTrainStop")   // this one has 16 layouts

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), rail_desc, true), null)
	ASSERT_EQUAL(stationbuilder.work(pl, coord3d(4, 3, 0), layout2_station.get_name()), null)

	local old_cash = pl.get_current_cash()
	{
		ASSERT_EQUAL(rotator.work(pl, coord3d(4, 3, 0)), "Cannot rotate this building!")
		ASSERT_EQUAL(pl.get_current_cash(), old_cash)
	}

	stationbuilder.set_flags(2) // enable ctrl
	ASSERT_EQUAL(stationbuilder.work(pl, coord3d(4, 3, 0), layout16_station.get_name()), null)
	stationbuilder.set_flags(0) // and disable ctrl again

	old_cash = pl.get_current_cash()
	{
		ASSERT_EQUAL(rotator.work(pl, coord3d(4, 3, 0)), null)
		ASSERT_EQUAL(pl.get_current_cash(), old_cash)
	}

	// clean up
	ASSERT_EQUAL(wayremover.work(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_building_rotate_factory()
{
	local public_pl = player_x(1)
	local rotator = command_x(tool_rotate_building)
	local remover = command_x(tool_remover)

	local production = 1024 // 1/s

	// non-square building with 2 rotations
	ASSERT_EQUAL(build_factory(public_pl, coord3d(3, 4, 0), 0, 1, production, "TANKE"), null)
	ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_building) != null)
	ASSERT_TRUE(tile_x(4, 4, 0).find_object(mo_building) != null)

	{
		ASSERT_EQUAL(rotator.work(public_pl, coord3d(3, 4, 0)), "Cannot rotate this building!")
		ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_building) != null)
		ASSERT_TRUE(tile_x(4, 4, 0).find_object(mo_building) != null)
		ASSERT_TRUE(tile_x(3, 5, 0).find_object(mo_building) == null)
		ASSERT_TRUE(tile_x(3, 3, 0).find_object(mo_building) == null)
	}

	// clean up
	ASSERT_EQUAL(remover.work(public_pl, coord3d(3, 4, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}
