//
// Tests for longblock signal reservation behaviour.
//
// Layout (x=10, track runs south y=0→14):
//   y= 0 : D1  – depot for main convoy
//   y= 2 : S1  – optional signal (none / pre-signal / priority, varies per test)
//   y= 4 : L   – longblock signal (always present)
//   y= 6 : H1  – first stop for main convoy
//   y= 8 : H2  – second stop; blocker convoy parks here for "blocked" tests
//   y=10 : S2  – simple signal
//   y=12 : H3  – third stop for main convoy (final destination)
//   y=14 : D2  – depot for blocker convoy (approaches H2 from the south)
//
// Main convoy schedule : H1 → H2 → H3  (0 % min-load, departs immediately)
// Blocker schedule     : H2 only        (200 % min-load, never departs)
//
// Test matrix
//   TC1  no signal at S1, no blocker  → convoy passes L, reaches H3
//   TC2  no signal at S1, blocker     → convoy stops AT L, after removal reaches H3
//   TC3  pre-signal at S1, no blocker → same as TC1
//   TC4  pre-signal at S1, blocker    → convoy stops near S1, after removal reaches H3
//   TC5  priority  at S1, no blocker  → same as TC1 (priority defers to step)
//   TC6  priority  at S1, blocker     → convoy stops AT L, after removal reaches H3
//
// Uses x=10 to avoid coordinate collisions with other test files.
//


// ──────────────────────────────────────────────────────────────────────────────
// Infrastructure helpers
// ──────────────────────────────────────────────────────────────────────────────

// Build shared track, depots, stations.  S1 tile is left empty (caller may
// optionally place a signal there).  Returns [sig_L, sig_S2].
function _lb_build_infra(pl, rail)
{
	local station_desc = building_desc_x.get_available_stations(
	                         building_desc_x.station, wt_rail, good_desc_x.passenger)[0]
	local depot_desc   = get_depot_by_wt(wt_rail)
	ASSERT_TRUE(station_desc != null)
	ASSERT_TRUE(depot_desc   != null)

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(10, 0, 0), coord3d(10, 14, 0), rail, true), null)

	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(10,  0, 0), depot_desc), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(10, 14, 0), depot_desc), null)

	ASSERT_EQUAL(command_x.build_station(pl, coord3d(10,  6, 0), station_desc), null)  // H1
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(10,  8, 0), station_desc), null)  // H2
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(10, 12, 0), station_desc), null)  // H3

	local lb_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                    @(idx, s) s.is_longblock_signal())[0]
	local sg_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                    @(idx, s) s.is_signal())[0]
	ASSERT_TRUE(lb_desc != null)
	ASSERT_TRUE(sg_desc != null)

	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(10,  4, 0), lb_desc), null)  // L
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(10, 10, 0), sg_desc), null)  // S2

	local sig_L  = tile_x(10,  4, 0).find_object(mo_signal)
	local sig_S2 = tile_x(10, 10, 0).find_object(mo_signal)
	ASSERT_TRUE(sig_L  != null)
	ASSERT_TRUE(sig_S2 != null)

	return [sig_L, sig_S2]
}

function _lb_remove_infra(pl)
{
	// signals
	foreach (y in [2, 4, 10]) {
		local sig = tile_x(10, y, 0).find_object(mo_signal)
		if (sig != null) {
			command_x(tool_remover).work(pl, coord3d(10, y, 0))
		}
	}
	// stations & depots
	foreach (y in [0, 6, 8, 12, 14]) {
		command_x(tool_remover).work(pl, coord3d(10, y, 0))
	}
	command_x(tool_remove_way).work(pl, coord3d(10, 0, 0), coord3d(10, 14, 0), "" + wt_rail)
}

// Start the main convoy (D1 → H1 → H2 → H3).
function _lb_start_main(pl)
{
	local depot = depot_x(10, 0, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("H-Trans-Pantheress"))
	local cnv = depot.get_convoy_list()[0]
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Tiger-Car"))
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Tiger-Car"))
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Pantheress-Back"))
	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(10,  6, 0), 0, 0),   // H1
		schedule_entry_x(coord3d(10,  8, 0), 0, 0),   // H2
		schedule_entry_x(coord3d(10, 12, 0), 0, 0),   // H3
	]))
	depot.start_all_convoys(pl)
	sleep()
	return cnv
}

// Start the blocker convoy (D2 → H2 at 200 % min-load, never departs).
function _lb_start_blocker(pl)
{
	local depot = depot_x(10, 14, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("H-Trans-Pantheress"))
	local cnv = depot.get_convoy_list()[0]
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Tiger-Car"))
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Tiger-Car"))
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Pantheress-Back"))
	// Two entries required (schedule validation rejects single-entry schedules).
	// H2 at 200% min-load keeps the blocker parked there permanently;
	// H3 is the "return" entry but is never reached because the convoy never departs H2.
	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(10,  8, 0), 200, 0),  // H2 – 200% min-load → stays forever
		schedule_entry_x(coord3d(10, 12, 0), 200, 0),  // H3 – backup entry (never reached)
	]))
	depot.start_all_convoys(pl)
	// Blocker travels north: y decreases from 14 → 8.  Wait until it reaches H2.
	for (local i = 0; i < 6000 && cnv.is_valid() && cnv.get_pos().y > 8; i++) {
		sleep()
	}
	return cnv
}

// Wait until cnv.get_pos().y reaches target_y (or timeout).
function _lb_wait_until_y(cnv, target_y, max_steps)
{
	for (local i = 0; i < max_steps; i++) {
		sleep()
		if (!cnv.is_valid()) return false
		if (cnv.get_pos().y == target_y) return true
	}
	return false
}

// Wait until cnv.get_pos().y exceeds target_y (or timeout).
function _lb_wait_past_y(cnv, target_y, max_steps)
{
	for (local i = 0; i < max_steps; i++) {
		sleep()
		if (!cnv.is_valid()) return false
		if (cnv.get_pos().y > target_y) return true
	}
	return false
}

// Destroy convoy safely and wait for removal.
function _lb_destroy(pl, cnv)
{
	if (cnv.is_valid()) {
		cnv.destroy(pl)
		sleep()
		sleep()
	}
}


// ──────────────────────────────────────────────────────────────────────────────
// TC1 – plain longblock, no blocker
// L reserves through H1/H2 to S2; convoy reaches H3 unobstructed.
// ──────────────────────────────────────────────────────────────────────────────
function test_longblock_open_no_prefix_signal()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	ASSERT_TRUE(rail != null)

	local sigs = _lb_build_infra(pl, rail)
	// No signal at S1.

	debug.set_game_speed(5)
	local cnv = _lb_start_main(pl)

	// Convoy must reach H3 (y=12).
	local reached_H3 = _lb_wait_until_y(cnv, 12, 8000)

	print("  reached_H3: " + reached_H3)

	debug.set_game_speed(1)
	_lb_destroy(pl, cnv)
	_lb_remove_infra(pl)
	RESET_ALL_PLAYER_FUNDS()

	ASSERT_TRUE(reached_H3)
}


// ──────────────────────────────────────────────────────────────────────────────
// TC2 – plain longblock, blocker at H2
// Convoy must stop AT L (y=4) while blocker holds H2.
// After removing the blocker the convoy reaches H3.
// ──────────────────────────────────────────────────────────────────────────────
function test_longblock_blocked_no_prefix_signal()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	ASSERT_TRUE(rail != null)

	local sigs = _lb_build_infra(pl, rail)

	debug.set_game_speed(5)

	// Blocker first, then main convoy.
	local blocker = _lb_start_blocker(pl)
	local cnv     = _lb_start_main(pl)

	// Convoy should stop at L (y=4) – the longblock signal blocks because H2 is occupied.
	local stopped_at_L = false
	for (local i = 0; i < 6000; i++) {
		sleep()
		if (!cnv.is_valid()) break
		if (cnv.get_pos().y == 4 && (cnv.is_waiting() || cnv.get_speed() <= 0)) {
			stopped_at_L = true
			break
		}
	}

	// Remove blocker and verify convoy proceeds to H3.
	local reached_H3 = false
	if (stopped_at_L) {
		_lb_destroy(pl, blocker)
		reached_H3 = _lb_wait_until_y(cnv, 12, 8000)
	}

	print("  stopped_at_L: " + stopped_at_L)
	print("  reached_H3:   " + reached_H3)

	debug.set_game_speed(1)
	_lb_destroy(pl, blocker)
	_lb_destroy(pl, cnv)
	_lb_remove_infra(pl)
	RESET_ALL_PLAYER_FUNDS()

	ASSERT_TRUE(stopped_at_L)
	ASSERT_TRUE(reached_H3)
}


// ──────────────────────────────────────────────────────────────────────────────
// TC3 – pre-signal at S1, no blocker
// Pre-signal evaluates L; convoy reaches H3 unobstructed.
// ──────────────────────────────────────────────────────────────────────────────
function test_longblock_open_pre_signal()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	ASSERT_TRUE(rail != null)

	local pre_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                     @(idx, s) s.is_pre_signal())[0]
	ASSERT_TRUE(pre_desc != null)

	local sigs = _lb_build_infra(pl, rail)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(10, 2, 0), pre_desc), null)
	local sig_S1 = tile_x(10, 2, 0).find_object(mo_signal)
	ASSERT_TRUE(sig_S1 != null)

	debug.set_game_speed(5)
	local cnv = _lb_start_main(pl)

	local reached_H3 = _lb_wait_until_y(cnv, 12, 8000)

	print("  reached_H3: " + reached_H3)

	debug.set_game_speed(1)
	_lb_destroy(pl, cnv)
	_lb_remove_infra(pl)
	RESET_ALL_PLAYER_FUNDS()

	ASSERT_TRUE(reached_H3)
}


// ──────────────────────────────────────────────────────────────────────────────
// TC4 – pre-signal at S1, blocker at H2
// Pre-signal cascades to L; because L cannot reserve through H2 the pre-signal
// shows YELLOW and convoy stops near S1 (y ≤ 3).
// After removing the blocker the convoy reaches H3.
// ──────────────────────────────────────────────────────────────────────────────
function test_longblock_blocked_pre_signal()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	ASSERT_TRUE(rail != null)

	local pre_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                     @(idx, s) s.is_pre_signal())[0]
	ASSERT_TRUE(pre_desc != null)

	local sigs = _lb_build_infra(pl, rail)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(10, 2, 0), pre_desc), null)
	local sig_S1 = tile_x(10, 2, 0).find_object(mo_signal)
	ASSERT_TRUE(sig_S1 != null)

	debug.set_game_speed(5)

	local blocker = _lb_start_blocker(pl)
	local cnv     = _lb_start_main(pl)

	// Convoy should be held near S1 (y <= 3) because pre-signal returns YELLOW/RED.
	local stopped_near_S1 = false
	for (local i = 0; i < 6000; i++) {
		sleep()
		if (!cnv.is_valid()) break
		local pos = cnv.get_pos()
		if (pos.y <= 3 && (cnv.is_waiting() || cnv.get_speed() <= 0)) {
			stopped_near_S1 = true
			break
		}
	}

	// Remove blocker; convoy should now proceed all the way to H3.
	local reached_H3 = false
	if (stopped_near_S1) {
		_lb_destroy(pl, blocker)
		reached_H3 = _lb_wait_until_y(cnv, 12, 8000)
	}

	print("  stopped_near_S1: " + stopped_near_S1)
	print("  reached_H3:      " + reached_H3)

	debug.set_game_speed(1)
	_lb_destroy(pl, blocker)
	_lb_destroy(pl, cnv)
	_lb_remove_infra(pl)
	RESET_ALL_PLAYER_FUNDS()

	ASSERT_TRUE(stopped_near_S1)
	ASSERT_TRUE(reached_H3)
}


// ──────────────────────────────────────────────────────────────────────────────
// TC5 – priority signal at S1, no blocker
// Priority cannot evaluate L in sync_step; it defers and evaluates in step().
// The convoy is briefly held at S1 (WAITING_FOR_CLEARANCE for one step) while
// the longblock is checked, then proceeds and reaches H3.
// ──────────────────────────────────────────────────────────────────────────────
function test_longblock_open_priority_signal()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	ASSERT_TRUE(rail != null)

	local pri_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                     @(idx, s) s.is_priority_signal())[0]
	ASSERT_TRUE(pri_desc != null)

	local sigs = _lb_build_infra(pl, rail)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(10, 2, 0), pri_desc), null)
	local sig_S1 = tile_x(10, 2, 0).find_object(mo_signal)
	ASSERT_TRUE(sig_S1 != null)

	debug.set_game_speed(5)
	local cnv = _lb_start_main(pl)

	// The convoy should pass S1 (briefly waiting) and then L, then reach H3.
	// We observe a brief wait near S1 (y <= 3) before the convoy proceeds.
	local waited_at_S1 = false
	local reached_H3   = false

	for (local i = 0; i < 8000; i++) {
		sleep()
		if (!cnv.is_valid()) break
		local pos = cnv.get_pos()
		if (pos.y <= 3 && cnv.is_waiting()) {
			waited_at_S1 = true
		}
		if (pos.y == 12) {
			reached_H3 = true
			break
		}
	}

	print("  waited_at_S1: " + waited_at_S1)
	print("  reached_H3:   " + reached_H3)

	debug.set_game_speed(1)
	_lb_destroy(pl, cnv)
	_lb_remove_infra(pl)
	RESET_ALL_PLAYER_FUNDS()

	ASSERT_TRUE(waited_at_S1)
	ASSERT_TRUE(reached_H3)
}


// ──────────────────────────────────────────────────────────────────────────────
// TC6 – priority signal at S1, blocker at H2
// Priority "can still go" with longblock: reserves S1→L, convoy proceeds to L.
// At L the longblock section (L→S2) cannot be reserved because H2 is occupied,
// so the convoy stops AT L (y=4).
// After removing the blocker check_longblock_signal succeeds, convoy reaches H3.
// ──────────────────────────────────────────────────────────────────────────────
function test_longblock_blocked_priority_signal()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	ASSERT_TRUE(rail != null)

	local pri_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                     @(idx, s) s.is_priority_signal())[0]
	ASSERT_TRUE(pri_desc != null)
	print("  pri_desc ok: " + pri_desc.get_name())

	local sigs = _lb_build_infra(pl, rail)
	print("  infra built")
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(10, 2, 0), pri_desc), null)
	local sig_S1 = tile_x(10, 2, 0).find_object(mo_signal)
	ASSERT_TRUE(sig_S1 != null)
	print("  S1 placed")

	debug.set_game_speed(5)

	local blocker = _lb_start_blocker(pl)
	print("  blocker at y=" + blocker.get_pos().y)
	local cnv     = _lb_start_main(pl)
	print("  main started at y=" + cnv.get_pos().y)

	// Convoy should stop AT L (y=4).  Priority passes S1 ("can still go") but
	// the longblock at L cannot reserve through the occupied H2.
	local stopped_at_L = false
	local last_y = -1
	for (local i = 0; i < 6000; i++) {
		sleep()
		if (!cnv.is_valid()) { print("  cnv invalid at step " + i); break }
		local y = cnv.get_pos().y
		if (y != last_y) { print("  cnv y=" + y + " waiting=" + cnv.is_waiting()); last_y = y }
		if (y == 4 && (cnv.is_waiting() || cnv.get_speed() <= 0)) {
			stopped_at_L = true
			break
		}
		if (y > 4) break  // convoy passed L already
	}

	// Remove blocker; check_longblock_signal now succeeds, convoy proceeds to H3.
	local reached_H3 = false
	if (stopped_at_L) {
		_lb_destroy(pl, blocker)
		reached_H3 = _lb_wait_until_y(cnv, 12, 8000)
	}

	print("  stopped_at_L: " + stopped_at_L)
	print("  reached_H3:   " + reached_H3)

	debug.set_game_speed(1)
	_lb_destroy(pl, blocker)
	_lb_destroy(pl, cnv)
	_lb_remove_infra(pl)
	RESET_ALL_PLAYER_FUNDS()

	ASSERT_TRUE(stopped_at_L)
	ASSERT_TRUE(reached_H3)
}


// ──────────────────────────────────────────────────────────────────────────────
// Helper: remove signal at y=3 (TC7-9 place a second signal there)
// ──────────────────────────────────────────────────────────────────────────────
function _lb_remove_y3_signal(pl)
{
	local sig = tile_x(10, 3, 0).find_object(mo_signal)
	if (sig != null) {
		command_x(tool_remover).work(pl, coord3d(10, 3, 0))
	}
}


// ──────────────────────────────────────────────────────────────────────────────
// TC7 – priority(y=2)→priority(y=3)→longblock(y=4), blocker at H2
// Both priority signals cascade "can still go" and let the convoy through to L.
// At L the longblock section cannot be reserved because H2 is occupied.
// Convoy stops AT L (y=4). After removing the blocker convoy reaches H3.
// ──────────────────────────────────────────────────────────────────────────────
function test_longblock_blocked_priority_priority_long()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	ASSERT_TRUE(rail != null)

	local pri_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                     @(idx, s) s.is_priority_signal())[0]
	ASSERT_TRUE(pri_desc != null)

	local sigs = _lb_build_infra(pl, rail)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(10, 2, 0), pri_desc), null)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(10, 3, 0), pri_desc), null)
	ASSERT_TRUE(tile_x(10, 2, 0).find_object(mo_signal) != null)
	ASSERT_TRUE(tile_x(10, 3, 0).find_object(mo_signal) != null)

	debug.set_game_speed(5)

	local blocker = _lb_start_blocker(pl)
	local cnv     = _lb_start_main(pl)

	local stopped_at_L = false
	for (local i = 0; i < 6000; i++) {
		sleep()
		if (!cnv.is_valid()) break
		local y = cnv.get_pos().y
		if (y == 4 && (cnv.is_waiting() || cnv.get_speed() <= 0)) {
			stopped_at_L = true
			break
		}
		if (y > 4) break
	}

	local reached_H3 = false
	if (stopped_at_L) {
		_lb_destroy(pl, blocker)
		reached_H3 = _lb_wait_until_y(cnv, 12, 8000)
	}

	print("  stopped_at_L: " + stopped_at_L)
	print("  reached_H3:   " + reached_H3)

	debug.set_game_speed(1)
	_lb_destroy(pl, blocker)
	_lb_destroy(pl, cnv)
	_lb_remove_y3_signal(pl)
	_lb_remove_infra(pl)
	RESET_ALL_PLAYER_FUNDS()

	ASSERT_TRUE(stopped_at_L)
	ASSERT_TRUE(reached_H3)
}


// ──────────────────────────────────────────────────────────────────────────────
// TC8 – priority(y=2)→pre-signal(y=3)→longblock(y=4), blocker at H2
// Priority at y=2 "can still go" (cascades to the pre which is YELLOW because
// L is blocked), advancing the convoy to the pre-signal at y=3.
// The pre-signal blocks because L cannot reserve through H2.
// Convoy stops AT the pre-signal (y=3). After removing the blocker reaches H3.
// ──────────────────────────────────────────────────────────────────────────────
function test_longblock_blocked_priority_pre_long()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	ASSERT_TRUE(rail != null)

	local pri_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                     @(idx, s) s.is_priority_signal())[0]
	local pre_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                     @(idx, s) s.is_pre_signal())[0]
	ASSERT_TRUE(pri_desc != null)
	ASSERT_TRUE(pre_desc != null)

	local sigs = _lb_build_infra(pl, rail)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(10, 2, 0), pri_desc), null)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(10, 3, 0), pre_desc), null)
	ASSERT_TRUE(tile_x(10, 2, 0).find_object(mo_signal) != null)
	ASSERT_TRUE(tile_x(10, 3, 0).find_object(mo_signal) != null)

	debug.set_game_speed(5)

	local blocker = _lb_start_blocker(pl)
	local cnv     = _lb_start_main(pl)

	local stopped_at_pre = false
	for (local i = 0; i < 6000; i++) {
		sleep()
		if (!cnv.is_valid()) break
		local y = cnv.get_pos().y
		if (y == 3 && (cnv.is_waiting() || cnv.get_speed() <= 0)) {
			stopped_at_pre = true
			break
		}
		if (y > 3) break
	}

	local reached_H3 = false
	if (stopped_at_pre) {
		_lb_destroy(pl, blocker)
		reached_H3 = _lb_wait_until_y(cnv, 12, 8000)
	}

	print("  stopped_at_pre: " + stopped_at_pre)
	print("  reached_H3:     " + reached_H3)

	debug.set_game_speed(1)
	_lb_destroy(pl, blocker)
	_lb_destroy(pl, cnv)
	_lb_remove_y3_signal(pl)
	_lb_remove_infra(pl)
	RESET_ALL_PLAYER_FUNDS()

	ASSERT_TRUE(stopped_at_pre)
	ASSERT_TRUE(reached_H3)
}


// ──────────────────────────────────────────────────────────────────────────────
// TC9 – pre-signal(y=2)→priority(y=3)→longblock(y=4), blocker at H2
// Pre-signal evaluates priority which evaluates L.  L is blocked, but priority
// "can still go" so the pre-signal sees GREEN from priority and shows GREEN.
// The convoy proceeds through both y=2 and y=3 and stops AT L (y=4).
// After removing the blocker convoy reaches H3.
// ──────────────────────────────────────────────────────────────────────────────
function test_longblock_blocked_pre_priority_long()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	ASSERT_TRUE(rail != null)

	local pre_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                     @(idx, s) s.is_pre_signal())[0]
	local pri_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                     @(idx, s) s.is_priority_signal())[0]
	ASSERT_TRUE(pre_desc != null)
	ASSERT_TRUE(pri_desc != null)

	local sigs = _lb_build_infra(pl, rail)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(10, 2, 0), pre_desc), null)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(10, 3, 0), pri_desc), null)
	ASSERT_TRUE(tile_x(10, 2, 0).find_object(mo_signal) != null)
	ASSERT_TRUE(tile_x(10, 3, 0).find_object(mo_signal) != null)

	debug.set_game_speed(5)

	local blocker = _lb_start_blocker(pl)
	local cnv     = _lb_start_main(pl)

	local stopped_at_L = false
	for (local i = 0; i < 6000; i++) {
		sleep()
		if (!cnv.is_valid()) break
		local y = cnv.get_pos().y
		if (y == 4 && (cnv.is_waiting() || cnv.get_speed() <= 0)) {
			stopped_at_L = true
			break
		}
		if (y > 4) break
	}

	local reached_H3 = false
	if (stopped_at_L) {
		_lb_destroy(pl, blocker)
		reached_H3 = _lb_wait_until_y(cnv, 12, 8000)
	}

	print("  stopped_at_L: " + stopped_at_L)
	print("  reached_H3:   " + reached_H3)

	debug.set_game_speed(1)
	_lb_destroy(pl, blocker)
	_lb_destroy(pl, cnv)
	_lb_remove_y3_signal(pl)
	_lb_remove_infra(pl)
	RESET_ALL_PLAYER_FUNDS()

	ASSERT_TRUE(stopped_at_L)
	ASSERT_TRUE(reached_H3)
}
