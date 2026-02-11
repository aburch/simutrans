//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests convoi reservation
//


function test_reservation_clear_ground()
{
	local clear_reservation = command_x(tool_clear_reservation)
	local pl = player_x(0)

	// invalid coord
	{
		ASSERT_EQUAL(clear_reservation.work(pl, coord3d(-1, -1, 0)), null)
	}

	// flat ground
	{
		ASSERT_EQUAL(clear_reservation.work(pl, coord3d(0, 0, 0)), null)
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_reservation_clear_road()
{
	local clear_reservation = command_x(tool_clear_reservation)
	local pl = player_x(0)
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), road, true), null)

	{
		ASSERT_EQUAL(clear_reservation.work(pl, coord3d(4, 3, 0)), null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_reservation_clear_rail()
{
	local clear_reservation = command_x(tool_clear_reservation)
	local pl = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), rail, true), null)

	{
		ASSERT_EQUAL(clear_reservation.work(pl, coord3d(4, 3, 0)), null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_reservation_clear_rail_occupied()
{
	// TODO
}
