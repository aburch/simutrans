//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//

//
// Tests that a rail convoy reroutes and releases tile reservations when
// its planned route is cut by removing a track tile.
//
// Layout (16x16 map, flat, no signals):
//
//   depot   stop1                           stop2  runout
//   (1,5)   (2,5)----[upper route y=5]----(13,5) (14,5)
//            |  \                          /  |
//        junction                       junction
//            \                              /
//            (2,8)---[lower route y=8]---(13,8)
//
// The upper route between the two stops is ~11 tiles.
// The lower detour is longer (~17 tiles).
//
// Test sequence:
//   1. A convoy travels stop1 → stop2 via the shorter upper route.
//   2. While the convoy is stopped at stop2, the center tile (8,5) of
//      the upper route is removed.
//   3. The convoy must reroute stop2 → stop1 via the lower detour and
//      arrive at stop1 successfully.
//
// This exercises the behavior in convoi_t::step() where NO_ROUTE triggers
// clear_reserved_tiles() followed by drive_to() to find an alternative path.
//

local RR_X_DEPOT  = 1    // depot x (one tile west of stop1)
local RR_X_STOP1  = 2    // stop1 / left  junction x
local RR_X_STOP2  = 13   // stop2 / right junction x
local RR_X_RUNOUT = 14   // run-out tile east of stop2
local RR_Y_UPPER  = 5    // y of the upper (shorter) route
local RR_Y_LOWER  = 8    // y of the lower (longer)  detour
local RR_CUT_X    = 8    // x of the single tile removed from the upper route


function _rr_build_infra(pl, rail, station_desc)
{
	local depot_desc = get_depot_by_wt(wt_rail)
	ASSERT_TRUE(depot_desc != null)

	// Upper route: depot tile through stop1, across to stop2, and a run-out tile
	ASSERT_EQUAL(command_x.build_way(pl,
		coord3d(RR_X_DEPOT,  RR_Y_UPPER, 0),
		coord3d(RR_X_RUNOUT, RR_Y_UPPER, 0), rail, true), null)

	// Lower detour: left vertical leg, horizontal straight, right vertical leg
	ASSERT_EQUAL(command_x.build_way(pl,
		coord3d(RR_X_STOP1, RR_Y_UPPER, 0),
		coord3d(RR_X_STOP1, RR_Y_LOWER, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_way(pl,
		coord3d(RR_X_STOP1, RR_Y_LOWER, 0),
		coord3d(RR_X_STOP2, RR_Y_LOWER, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_way(pl,
		coord3d(RR_X_STOP2, RR_Y_LOWER, 0),
		coord3d(RR_X_STOP2, RR_Y_UPPER, 0), rail, true), null)

	// Stations at the junction tiles (coincide with the branch points)
	ASSERT_EQUAL(command_x.build_station(pl,
		coord3d(RR_X_STOP1, RR_Y_UPPER, 0), station_desc), null)
	ASSERT_EQUAL(command_x.build_station(pl,
		coord3d(RR_X_STOP2, RR_Y_UPPER, 0), station_desc), null)

	// Depot on the run-in tile just west of stop1
	ASSERT_EQUAL(command_x.build_depot(pl,
		coord3d(RR_X_DEPOT, RR_Y_UPPER, 0), depot_desc), null)
}


function _rr_remove_infra(pl)
{
	// Remove station buildings and depot first
	ASSERT_EQUAL(command_x(tool_remover).work(pl,
		coord3d(RR_X_STOP1, RR_Y_UPPER, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl,
		coord3d(RR_X_STOP2, RR_Y_UPPER, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl,
		coord3d(RR_X_DEPOT, RR_Y_UPPER, 0)), null)

	// Lower detour segments (remove before upper route so junction tiles
	// at stop1/stop2 still have their north-south connections intact)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl,
		coord3d(RR_X_STOP1, RR_Y_UPPER, 0),
		coord3d(RR_X_STOP1, RR_Y_LOWER, 0), "" + wt_rail), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl,
		coord3d(RR_X_STOP1, RR_Y_LOWER, 0),
		coord3d(RR_X_STOP2, RR_Y_LOWER, 0), "" + wt_rail), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl,
		coord3d(RR_X_STOP2, RR_Y_LOWER, 0),
		coord3d(RR_X_STOP2, RR_Y_UPPER, 0), "" + wt_rail), null)

	// Upper route: left segment (depot → one tile before cut)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl,
		coord3d(RR_X_DEPOT, RR_Y_UPPER, 0),
		coord3d(RR_CUT_X - 1, RR_Y_UPPER, 0), "" + wt_rail), null)
	// Upper route: right segment (one tile after cut → run-out)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl,
		coord3d(RR_CUT_X + 1, RR_Y_UPPER, 0),
		coord3d(RR_X_RUNOUT, RR_Y_UPPER, 0), "" + wt_rail), null)

	RESET_ALL_PLAYER_FUNDS()
}


//
// Verify that after removing the center tile of the active route the
// convoy reroutes via the surviving lower detour and reaches stop1.
//
function test_rail_reroute_releases_reservation_after_track_cut()
{
	local pl           = player_x(0)
	local rail         = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local station_desc = building_desc_x.get_available_stations(
	                         building_desc_x.station, wt_rail, good_desc_x.passenger)[0]
	ASSERT_TRUE(rail         != null)
	ASSERT_TRUE(station_desc != null)

	_rr_build_infra(pl, rail, station_desc)

	// Create a single-vehicle train in the depot
	local depot = depot_x(RR_X_DEPOT, RR_Y_UPPER, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	local cnv = depot.get_convoy_list()[0]
	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(RR_X_STOP1, RR_Y_UPPER, 0), 0, 0),
		schedule_entry_x(coord3d(RR_X_STOP2, RR_Y_UPPER, 0), 0, 0)
	]))

	debug.set_game_speed(5)
	depot.start_all_convoys(pl)
	sleep()

	// ── Step 1: wait for convoy to reach stop2 and stop there ─────────────────
	local reached_stop2 = false
	for (local i = 0; i < 3000; i++) {
		sleep()
		if (!cnv.is_valid()) break
		if (cnv.get_pos().x == RR_X_STOP2 && cnv.get_pos().y == RR_Y_UPPER
				&& (cnv.is_loading() || cnv.is_waiting())) {
			reached_stop2 = true
			break
		}
	}

	// ── Step 2: cut the center tile of the upper route ────────────────────────
	// single-tile removal: start == end removes exactly one rail tile
	ASSERT_EQUAL(
		command_x(tool_remove_way).work(pl,
			coord3d(RR_CUT_X, RR_Y_UPPER, 0),
			coord3d(RR_CUT_X, RR_Y_UPPER, 0),
			"" + wt_rail),
		null)

	// ── Step 3: convoy must reroute and arrive at stop1 via lower detour ──────
	// The convoy enters NO_ROUTE when the upper path is found to be broken,
	// clears all tile reservations, then re-plans the route through the lower
	// branch (the only remaining connected path between the two stops).
	local reached_stop1_via_lower = false
	for (local i = 0; i < 6000; i++) {
		sleep()
		if (!cnv.is_valid()) break
		if (cnv.get_pos().x == RR_X_STOP1 && cnv.get_pos().y == RR_Y_UPPER
				&& (cnv.is_loading() || cnv.is_waiting())) {
			reached_stop1_via_lower = true
			break
		}
	}

	print("  reached_stop2:           " + reached_stop2)
	print("  reached_stop1_via_lower: " + reached_stop1_via_lower)

	// ── Cleanup before assertions so teardown always executes ─────────────────
	debug.set_game_speed(1)
	if (cnv.is_valid()) {
		cnv.destroy(pl)
		sleep()
		sleep()
	}
	_rr_remove_infra(pl)

	ASSERT_TRUE(reached_stop2)
	ASSERT_TRUE(reached_stop1_via_lower)
}
