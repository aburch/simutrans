//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//

//
// Tests that a rail convoy reroutes and releases tile reservations when
// the upper route is broken.
//
// Layout (16x16 map, flat, no signals):
//
//   depot   stop1   jct1               jct2   stop2  runout
//   (1,5)   (2,5)   (3,5)--upper--(12,5)   (13,5) (14,5)
//                    |                  |
//                   (3,8)--lower--(12,8)
//
// Stations at (2,5) and (13,5) are on plain through-track (2 connections).
// Junctions at (3,5) and (12,5) are NOT stations (3 connections).
//
// Three test cases:
//
//   test_rail_reroute_no_line
//     Convoy without a line (no route cache).  Track is cut while the convoy
//     is 3 tiles before the cut tile.  Exercises the plain
//     NO_ROUTE → clear_reserved_tiles() → drive_to() path.
//
//   test_rail_reroute_with_line_stale_cache
//     Convoy on a line.  cnv1 travels stop1→stop2 to populate the line's
//     route cache with the upper path, then is destroyed.  The upper route
//     is cut BEFORE cnv2 starts.  When cnv2 departs, drive_to() checks the
//     cached route, is_passable() returns false (tile gone), falls back to
//     A* and finds the lower route.
//
//   test_rail_reroute_with_line_mid_travel
//     Convoy on a line.  cnv1 populates the cache (upper path), destroyed.
//     cnv2 starts on the same line and begins traveling the cached upper
//     route.  The track is cut 3 tiles ahead of cnv2 while it is moving.
//     cnv2 hits NO_ROUTE mid-travel, clears reservations, re-runs drive_to()
//     (is_passable() rejects the now-stale cache), and reroutes via lower.
//
// Vehicle: 1Diesellokomotive (single-unit, confirmed available in test pak).
// All Simutrans rail vehicles can reverse when no one-way sign is present,
// so the convoy can back up to jct1(3,5) and take the lower branch.
//

local RR_X_DEPOT  = 1
local RR_X_STOP1  = 2
local RR_X_JCT1   = 3
local RR_X_JCT2   = 12
local RR_X_STOP2  = 13
local RR_X_RUNOUT = 14
local RR_Y_UPPER  = 5
local RR_Y_LOWER  = 8
local RR_CUT_X    = 7   // tile removed to break the upper route


function _rr_build_infra(pl, rail, station_desc)
{
	local depot_desc = building_desc_x("TrainDepot")
	ASSERT_TRUE(depot_desc != null)

	ASSERT_EQUAL(command_x.build_way(pl,
		coord3d(RR_X_DEPOT, RR_Y_UPPER, 0),
		coord3d(RR_X_RUNOUT, RR_Y_UPPER, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_way(pl,
		coord3d(RR_X_JCT1, RR_Y_UPPER, 0),
		coord3d(RR_X_JCT1, RR_Y_LOWER, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_way(pl,
		coord3d(RR_X_JCT1, RR_Y_LOWER, 0),
		coord3d(RR_X_JCT2, RR_Y_LOWER, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_way(pl,
		coord3d(RR_X_JCT2, RR_Y_LOWER, 0),
		coord3d(RR_X_JCT2, RR_Y_UPPER, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_station(pl,
		coord3d(RR_X_STOP1, RR_Y_UPPER, 0), station_desc), null)
	ASSERT_EQUAL(command_x.build_station(pl,
		coord3d(RR_X_STOP2, RR_Y_UPPER, 0), station_desc), null)
	ASSERT_EQUAL(command_x.build_depot(pl,
		coord3d(RR_X_DEPOT, RR_Y_UPPER, 0), depot_desc), null)
}


// Must be called after (RR_CUT_X, RR_Y_UPPER) has already been removed.
function _rr_remove_infra(pl)
{
	ASSERT_EQUAL(command_x(tool_remover).work(pl,
		coord3d(RR_X_STOP1, RR_Y_UPPER, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl,
		coord3d(RR_X_STOP2, RR_Y_UPPER, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl,
		coord3d(RR_X_DEPOT, RR_Y_UPPER, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl,
		coord3d(RR_X_JCT1, RR_Y_UPPER, 0),
		coord3d(RR_X_JCT1, RR_Y_LOWER, 0), "" + wt_rail), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl,
		coord3d(RR_X_JCT1, RR_Y_LOWER, 0),
		coord3d(RR_X_JCT2, RR_Y_LOWER, 0), "" + wt_rail), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl,
		coord3d(RR_X_JCT2, RR_Y_LOWER, 0),
		coord3d(RR_X_JCT2, RR_Y_UPPER, 0), "" + wt_rail), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl,
		coord3d(RR_X_DEPOT, RR_Y_UPPER, 0),
		coord3d(RR_CUT_X - 1, RR_Y_UPPER, 0), "" + wt_rail), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl,
		coord3d(RR_CUT_X + 1, RR_Y_UPPER, 0),
		coord3d(RR_X_RUNOUT, RR_Y_UPPER, 0), "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}


// Remove the cut tile immediately (no convoy in motion).
function _rr_cut_upper_route(pl)
{
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl,
		coord3d(RR_CUT_X, RR_Y_UPPER, 0),
		coord3d(RR_CUT_X, RR_Y_UPPER, 0),
		"" + wt_rail), null)
}


// Create a single-locomotive convoy in the depot and return it.
function _rr_create_convoy(pl)
{
	local depot = depot_x(RR_X_DEPOT, RR_Y_UPPER, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	return depot.get_convoy_list()[0]
}


// Create a line with the standard two-stop schedule and return it.
function _rr_create_line(pl)
{
	ASSERT_EQUAL(pl.create_line(wt_rail), true)
	local line_list = pl.get_line_list()
	local line = line_list[line_list.get_count() - 1]
	line.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(RR_X_STOP1, RR_Y_UPPER, 0), 0, 0),
		schedule_entry_x(coord3d(RR_X_STOP2, RR_Y_UPPER, 0), 0, 0)
	]))
	return line
}


// Poll until cnv is in the 3-tile window before the cut tile on the upper route,
// then remove the cut tile.  Returns true if the cut was performed.
function _rr_wait_for_trigger_and_cut(pl, cnv)
{
	for (local i = 0; i < 3000; i++) {
		sleep()
		if (!cnv.is_valid()) return false
		local pos = cnv.get_pos()
		if (pos.y == RR_Y_UPPER
				&& pos.x >= RR_CUT_X - 3
				&& pos.x <  RR_CUT_X) {
			_rr_cut_upper_route(pl)
			return true
		}
	}
	return false
}


// Poll until cnv reaches stop2 (or 6000 ticks expire).
function _rr_wait_for_stop2(cnv)
{
	for (local i = 0; i < 6000; i++) {
		sleep()
		if (!cnv.is_valid()) return false
		local pos = cnv.get_pos()
		if (pos.x == RR_X_STOP2 && pos.y == RR_Y_UPPER) return true
	}
	return false
}


// ── Test 1: no line — cut while traveling ─────────────────────────────────────
function test_rail_reroute_no_line()
{
	local pl           = player_x(0)
	local rail         = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local station_desc = building_desc_x("TrainStop")
	ASSERT_TRUE(rail         != null)
	ASSERT_TRUE(station_desc != null)

	_rr_build_infra(pl, rail, station_desc)

	local cnv = _rr_create_convoy(pl)
	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(RR_X_STOP1, RR_Y_UPPER, 0), 0, 0),
		schedule_entry_x(coord3d(RR_X_STOP2, RR_Y_UPPER, 0), 0, 0)
	]))

	debug.set_game_speed(5)
	depot_x(RR_X_DEPOT, RR_Y_UPPER, 0).start_all_convoys(pl)
	sleep()

	// Cut the upper route when the convoy enters the 3-tile trigger zone.
	// The convoy then enters NO_ROUTE → clear_reserved_tiles() → drive_to()
	// and reroutes via the lower detour.
	local cut_done      = _rr_wait_for_trigger_and_cut(pl, cnv)
	local reached_stop2 = _rr_wait_for_stop2(cnv)

	debug.set_game_speed(1)
	if (cnv.is_valid()) { cnv.destroy(pl); sleep(); sleep() }
	_rr_remove_infra(pl)

	ASSERT_TRUE(cut_done)
	ASSERT_TRUE(reached_stop2)
}


// ── Test 2: with line — stale cache, cut BEFORE cnv2 starts ──────────────────
//
// cnv1 populates the line cache with the upper route, is destroyed, then the
// upper route is cut.  When cnv2 departs, drive_to() hits is_passable()=false
// on the cached route and falls back to A*, which finds the lower route.
//
function test_rail_reroute_with_line_stale_cache()
{
	local pl           = player_x(0)
	local rail         = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local station_desc = building_desc_x("TrainStop")
	ASSERT_TRUE(rail         != null)
	ASSERT_TRUE(station_desc != null)

	_rr_build_infra(pl, rail, station_desc)
	local line = _rr_create_line(pl)

	debug.set_game_speed(5)

	// cnv1: populate line cache with the upper (short) route.
	local cnv1 = _rr_create_convoy(pl)
	cnv1.set_line(pl, line)
	depot_x(RR_X_DEPOT, RR_Y_UPPER, 0).start_all_convoys(pl)
	sleep()

	local cnv1_at_stop2 = false
	for (local i = 0; i < 3000; i++) {
		sleep()
		if (!cnv1.is_valid()) break
		local pos = cnv1.get_pos()
		if (pos.x == RR_X_STOP2 && pos.y == RR_Y_UPPER) { cnv1_at_stop2 = true; break }
	}
	if (cnv1.is_valid()) { cnv1.destroy(pl); sleep(); sleep() }

	// Cut the upper route while no convoy is running.
	// cnv2's drive_to() will find the cached upper route stale (is_passable=false)
	// and fall back to A*, which discovers the lower route.
	_rr_cut_upper_route(pl)

	// cnv2: same line, cache points to the (now broken) upper route.
	local cnv2 = _rr_create_convoy(pl)
	cnv2.set_line(pl, line)
	depot_x(RR_X_DEPOT, RR_Y_UPPER, 0).start_all_convoys(pl)
	sleep()

	local reached_stop2 = _rr_wait_for_stop2(cnv2)

	debug.set_game_speed(1)
	if (cnv2.is_valid()) { cnv2.destroy(pl); sleep(); sleep() }
	_rr_remove_infra(pl)

	ASSERT_TRUE(cnv1_at_stop2)
	ASSERT_TRUE(reached_stop2)
}


// ── Test 3: with line — cut while cnv2 is traveling the cached route ─────────
//
// cnv1 populates the cache (upper route), is destroyed.  cnv2 starts on the
// same line and begins traveling the cached upper route.  The track is cut
// 3 tiles ahead of cnv2 while it is moving.  cnv2 hits NO_ROUTE mid-travel,
// clears reservations, re-runs drive_to() (is_passable() rejects stale cache),
// and reroutes via the lower detour.
//
function test_rail_reroute_with_line_mid_travel()
{
	local pl           = player_x(0)
	local rail         = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local station_desc = building_desc_x("TrainStop")
	ASSERT_TRUE(rail         != null)
	ASSERT_TRUE(station_desc != null)

	_rr_build_infra(pl, rail, station_desc)
	local line = _rr_create_line(pl)

	debug.set_game_speed(5)

	// cnv1: populate line cache with the upper route.
	local cnv1 = _rr_create_convoy(pl)
	cnv1.set_line(pl, line)
	depot_x(RR_X_DEPOT, RR_Y_UPPER, 0).start_all_convoys(pl)
	sleep()

	local cnv1_at_stop2 = false
	for (local i = 0; i < 3000; i++) {
		sleep()
		if (!cnv1.is_valid()) break
		local pos = cnv1.get_pos()
		if (pos.x == RR_X_STOP2 && pos.y == RR_Y_UPPER) { cnv1_at_stop2 = true; break }
	}
	if (cnv1.is_valid()) { cnv1.destroy(pl); sleep(); sleep() }

	// cnv2: uses the cached upper route.  Cut the track while cnv2 is 3 tiles
	// before the cut tile, forcing a mid-travel reroute via the lower detour.
	local cnv2 = _rr_create_convoy(pl)
	cnv2.set_line(pl, line)
	depot_x(RR_X_DEPOT, RR_Y_UPPER, 0).start_all_convoys(pl)
	sleep()

	local cut_done      = _rr_wait_for_trigger_and_cut(pl, cnv2)
	local reached_stop2 = _rr_wait_for_stop2(cnv2)

	debug.set_game_speed(1)
	if (cnv2.is_valid()) { cnv2.destroy(pl); sleep(); sleep() }
	_rr_remove_infra(pl)

	ASSERT_TRUE(cnv1_at_stop2)
	ASSERT_TRUE(cut_done)
	ASSERT_TRUE(reached_stop2)
}
