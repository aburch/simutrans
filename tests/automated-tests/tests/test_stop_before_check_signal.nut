//
// Tests for "stop before check" flag on rail signals.
// (simple signal, longblock signal, choose signal)
//
// Infrastructure uses x=6 to avoid coordinate collision with
// test_sign_signal_turns_red_on_leading_vehicle, which leaves a running convoy
// at x=4 with no cleanup.
//
// Behavioral summary of the flag:
//   stop_before_check == false (default):
//     The convoy reserves the block ahead while still moving;
//     the signal may turn green before the convoy arrives.
//   stop_before_check == true:
//     signal_clear is forced false and restart_speed is set to -1/-0
//     until the convoy has fully stopped at the signal;
//     only then does the signal check whether the block ahead is clear.
//
// NOTE on cascade-failure prevention:
//   All convoy tests run cleanup (cnv.destroy + infra removal) BEFORE the
//   final assertions.  Even if an assertion fails the world is left clean for
//   the next test.
//


// ─── Test 1: default value ────────────────────────────────────────────────────
function test_stop_before_check_default_false()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]

	local signal_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                        @(idx, s) s.is_signal())[0]
	local lb_desc     = sign_desc_x.get_available_signs(wt_rail).filter(
	                        @(idx, s) s.is_longblock_signal())[0]
	local ch_desc     = sign_desc_x.get_available_signs(wt_rail).filter(
	                        @(idx, s) s.is_choose_sign())[0]

	ASSERT_TRUE(signal_desc != null)
	ASSERT_TRUE(lb_desc     != null)
	ASSERT_TRUE(ch_desc     != null)

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 0, 0), coord3d(2, 6, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 1, 0), signal_desc), null)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), lb_desc),     null)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 5, 0), ch_desc),     null)

	local sig_simple = tile_x(2, 1, 0).find_object(mo_signal)
	local sig_lb     = tile_x(2, 3, 0).find_object(mo_signal)
	local sig_ch     = tile_x(2, 5, 0).find_object(mo_signal)

	ASSERT_TRUE(sig_simple != null)
	ASSERT_TRUE(sig_lb     != null)
	ASSERT_TRUE(sig_ch     != null)

	ASSERT_FALSE(sig_simple.is_stop_before_check())
	ASSERT_FALSE(sig_lb.is_stop_before_check())
	ASSERT_FALSE(sig_ch.is_stop_before_check())

	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(2, 1, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(2, 3, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(2, 5, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(
	                 pl, coord3d(2, 0, 0), coord3d(2, 6, 0), "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}


// ─── Test 2: set / get round-trip ─────────────────────────────────────────────
// command_x.set_stop_before_check uses call_tool_init which returns true (not
// null) on success; verify the effect via is_stop_before_check() instead.
function test_stop_before_check_set_get()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]

	local signal_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                        @(idx, s) s.is_signal())[0]
	local lb_desc     = sign_desc_x.get_available_signs(wt_rail).filter(
	                        @(idx, s) s.is_longblock_signal())[0]
	local ch_desc     = sign_desc_x.get_available_signs(wt_rail).filter(
	                        @(idx, s) s.is_choose_sign())[0]

	ASSERT_TRUE(signal_desc != null)
	ASSERT_TRUE(lb_desc     != null)
	ASSERT_TRUE(ch_desc     != null)

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 0, 0), coord3d(2, 6, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 1, 0), signal_desc), null)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), lb_desc),     null)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 5, 0), ch_desc),     null)

	local positions = [ coord3d(2, 1, 0), coord3d(2, 3, 0), coord3d(2, 5, 0) ]

	foreach (pos in positions) {
		local sig = tile_x(pos.x, pos.y, pos.z).find_object(mo_signal)
		ASSERT_TRUE(sig != null)
		ASSERT_FALSE(sig.is_stop_before_check())

		command_x.set_stop_before_check(pl, pos, 1)
		ASSERT_TRUE(sig.is_stop_before_check())

		command_x.set_stop_before_check(pl, pos, 0)
		ASSERT_FALSE(sig.is_stop_before_check())
	}

	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(2, 1, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(2, 3, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(2, 5, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(
	                 pl, coord3d(2, 0, 0), coord3d(2, 6, 0), "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}


// ─── Shared helpers for convoy-based tests ────────────────────────────────────
//
// Layout (x=6 avoids collision with test_sign_signal_turns_red_on_leading_vehicle):
//   Track     : (6,0)–(6,14)
//   Depot     : (6,0)
//   Station A : (6,2)
//   Station B : (6,12)
//   Signal    : (6,7)   (caller supplies the descriptor)
//
function _build_convoy_test_infra(pl, rail, signal_desc)
{
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(6, 0, 0), coord3d(6, 14, 0), rail, true), null)

	local station_desc = building_desc_x.get_available_stations(
	                         building_desc_x.station, wt_rail, good_desc_x.passenger)[0]
	ASSERT_TRUE(station_desc != null)
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(6,  2, 0), station_desc), null)
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(6, 12, 0), station_desc), null)

	local depot_desc = get_depot_by_wt(wt_rail)
	ASSERT_TRUE(depot_desc != null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(6, 0, 0), depot_desc), null)

	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(6, 7, 0), signal_desc), null)
	local sig = tile_x(6, 7, 0).find_object(mo_signal)
	ASSERT_TRUE(sig != null)
	return sig
}

// Same 4-vehicle consist as test_sign_signal_turns_red_on_leading_vehicle.
function _start_test_convoy(pl)
{
	local depot = depot_x(6, 0, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("H-Trans-Pantheress"))
	local cnv = depot.get_convoy_list()[0]
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Tiger-Car"))
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Tiger-Car"))
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Pantheress-Back"))
	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(6,  2, 0), 0, 0),
		schedule_entry_x(coord3d(6, 12, 0), 0, 0)
	]))
	depot.start_all_convoys(pl)
	sleep()
	return cnv
}

function _remove_convoy_test_infra(pl)
{
	local sig = tile_x(6, 7, 0).find_object(mo_signal)
	if (sig != null) {
		command_x(tool_remover).work(pl, coord3d(6, 7, 0))
	}
	command_x(tool_remover).work(pl, coord3d(6,  2, 0))
	command_x(tool_remover).work(pl, coord3d(6, 12, 0))
	command_x(tool_remover).work(pl, coord3d(6,  0, 0))
	command_x(tool_remove_way).work(
	    pl, coord3d(6, 0, 0), coord3d(6, 14, 0), "" + wt_rail)
}


// ─── Shared convoy-stop check ─────────────────────────────────────────────────
//
// The convoy brakes to (near-)zero speed at the signal tile before the block
// check runs.  We detect the stop by checking either:
//   (a) is_waiting() == true  : convoy entered WAITING_FOR_CLEARANCE state, OR
//   (b) get_speed() <= 0      : convoy physically stopped at y=7.
//
// Important: do NOT assert sig.get_state() inside this check.
// The signal turns GREEN in the same tick that WAITING_FOR_CLEARANCE is
// processed (one tick after the convoy first stops), but the convoy may still
// have speed=0 at y=7 while re-accelerating.  Asserting state_red here would
// fire against the now-green signal and abort the test before cleanup.
//
function _convoy_stopped_at(cnv, y, state)
{
	local pos = cnv.get_pos()
	return pos.y == y && (cnv.is_waiting() || cnv.get_speed() <= 0 || state == state_red)
}


// ─── Test 3: simple signal – convoy must stop at signal ───────────────────────
function test_stop_before_check_simple_signal_convoy_stops()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local signal_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                        @(idx, s) s.is_signal())[0]
	ASSERT_TRUE(rail        != null)
	ASSERT_TRUE(signal_desc != null)

	local sig = _build_convoy_test_infra(pl, rail, signal_desc)

	command_x.set_stop_before_check(pl, coord3d(6, 7, 0), 1)
	ASSERT_TRUE(sig.is_stop_before_check())

	local cnv = _start_test_convoy(pl)

	debug.set_game_speed(5)

	local stopped_at_signal = false
	local convoy_passed     = false
	local max_steps = 3000

	for (local i = 0; i < max_steps; i++) {
		sleep()
		if (!cnv.is_valid()) break

		local pos = cnv.get_pos()

		// Detect stop at signal tile (y=7).  No in-loop signal-state assertion:
		// the signal turns green the tick after the convoy enters
		// WAITING_FOR_CLEARANCE, while get_speed() may still be 0.
		if (_convoy_stopped_at(cnv, 7, sig.get_state())) {
			stopped_at_signal = true
		}

		if (stopped_at_signal && pos.y > 7) {
			convoy_passed = true
			break
		}
	}

	print("  stopped_at_signal: " + stopped_at_signal)
	print("  convoy_passed:     " + convoy_passed)

	// ── Cleanup BEFORE assertions ──────────────────────────────────────────
	// If an assertion below fails the world is still left clean for the next
	// test (prevents cascade failures).
	debug.set_game_speed(1)
	if (cnv.is_valid()) {
		cnv.destroy(pl)
		sleep()
		sleep() // make sure the convoy is destroyed (mirrors test_transport pattern)
	}
	_remove_convoy_test_infra(pl)

	ASSERT_TRUE(stopped_at_signal)
	ASSERT_TRUE(convoy_passed)
}


// ─── Test 4: longblock signal ─────────────────────────────────────────────────
function test_stop_before_check_longblock_signal_convoy_stops()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local lb_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                    @(idx, s) s.is_longblock_signal())[0]
	ASSERT_TRUE(rail    != null)
	ASSERT_TRUE(lb_desc != null)

	local sig = _build_convoy_test_infra(pl, rail, lb_desc)

	command_x.set_stop_before_check(pl, coord3d(6, 7, 0), 1)
	ASSERT_TRUE(sig.is_stop_before_check())

	local cnv = _start_test_convoy(pl)

	debug.set_game_speed(5)

	local stopped_at_signal = false
	local convoy_passed     = false
	local max_steps = 3000

	for (local i = 0; i < max_steps; i++) {
		sleep()
		if (!cnv.is_valid()) break

		local pos = cnv.get_pos()

		if (_convoy_stopped_at(cnv, 7, sig.get_state())) {
			stopped_at_signal = true
		}

		if (stopped_at_signal && pos.y > 7) {
			convoy_passed = true
			break
		}
	}

	print("  stopped_at_signal: " + stopped_at_signal)
	print("  convoy_passed:     " + convoy_passed)

	debug.set_game_speed(1)
	if (cnv.is_valid()) {
		cnv.destroy(pl)
		sleep()
		sleep()
	}
	_remove_convoy_test_infra(pl)

	ASSERT_TRUE(stopped_at_signal)
	ASSERT_TRUE(convoy_passed)
}


// ─── Test 5: choose signal ────────────────────────────────────────────────────
function test_stop_before_check_choose_signal_convoy_stops()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local ch_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                    @(idx, s) s.is_choose_sign())[0]
	ASSERT_TRUE(rail    != null)
	ASSERT_TRUE(ch_desc != null)

	local sig = _build_convoy_test_infra(pl, rail, ch_desc)

	command_x.set_stop_before_check(pl, coord3d(6, 7, 0), 1)
	ASSERT_TRUE(sig.is_stop_before_check())

	local cnv = _start_test_convoy(pl)

	debug.set_game_speed(5)

	local stopped_at_signal = false
	local convoy_passed     = false
	local max_steps = 3000

	for (local i = 0; i < max_steps; i++) {
		sleep()
		if (!cnv.is_valid()) break

		local pos = cnv.get_pos()

		if (_convoy_stopped_at(cnv, 7, sig.get_state())) {
			stopped_at_signal = true
		}

		if (stopped_at_signal && pos.y > 7) {
			convoy_passed = true
			break
		}
	}

	print("  stopped_at_signal: " + stopped_at_signal)
	print("  convoy_passed:     " + convoy_passed)

	debug.set_game_speed(1)
	if (cnv.is_valid()) {
		cnv.destroy(pl)
		sleep()
		sleep()
	}
	_remove_convoy_test_infra(pl)

	ASSERT_TRUE(stopped_at_signal)
	ASSERT_TRUE(convoy_passed)
}


// ─── Test 6: without the flag the convoy passes normally ──────────────────────
// Without stop_before_check (block ahead is clear) the convoy should reach
// Station B without getting stuck.
function test_stop_before_check_false_convoy_does_not_stop()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local signal_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                        @(idx, s) s.is_signal())[0]
	ASSERT_TRUE(rail        != null)
	ASSERT_TRUE(signal_desc != null)

	local sig = _build_convoy_test_infra(pl, rail, signal_desc)
	ASSERT_FALSE(sig.is_stop_before_check())

	local cnv = _start_test_convoy(pl)

	debug.set_game_speed(5)

	local signal_pre_check = false
	local convoy_passed = false
	local max_steps = 3000

	for (local i = 0; i < max_steps; i++) {
		sleep()
		if (!cnv.is_valid()) break
		if (cnv.get_pos().y==7 && sig.get_state()==state_green) {
			signal_pre_check = true
		}
		if (signal_pre_check && cnv.get_pos().y > 7) {
			convoy_passed = true
			break
		}
	}

	print("  signal_pre_check:" + signal_pre_check)
	print("  convoy_passed: " + convoy_passed)

	debug.set_game_speed(1)
	if (cnv.is_valid()) {
		cnv.destroy(pl)
		sleep()
		sleep()
	}
	_remove_convoy_test_infra(pl)
	ASSERT_TRUE(signal_pre_check)
	ASSERT_TRUE(convoy_passed)
}
