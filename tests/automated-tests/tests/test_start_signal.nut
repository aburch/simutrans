//
// Tests for the "start signal" flag on rail signals.
//
// When is_start_signal is true, a convoy in CAN_START (or CAN_START_ONE_MONTH)
// state checks whether the next signal ahead is clear BEFORE departing.
// If that signal is RED (block occupied) the convoy stays at its current
// position (station/loading point) instead of advancing to the signal tile.
//
// Contrast with default behaviour (is_start_signal=false):
//   The convoy advances past the station toward the signal tile even when the
//   block ahead is occupied.
//
// Infrastructure uses x=8 to avoid coordinate collision with other tests.
// The test map is 16x16 (valid coordinates 0-15); all y values stay ≤ 14.
//
// Layout for convoy tests (x=8, vertical track y=0..14):
//   y= 0 : depot_A     -- convoy_1 leaves from here
//   y= 2 : station_A   -- convoy_1 first stop; waits here when start_signal is RED
//   y= 7 : signal       -- the signal under test (start_signal flag)
//   y=10 : station_B   -- convoy_1 second stop; convoy_2 (blocker) loads here
//   y=13 : station_C	-- convoy_2 (blocker) finally go here
//   y=14 : depot_B     -- convoy_2 (blocker) departs from here
//
// convoy_2 has 200 % min-load at station_B so it never departs;
// its reserved track keeps the block y=8..y=10 occupied.
//


// ─── Test 1: default value ────────────────────────────────────────────────────
function test_start_signal_default_false()
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

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(8, 0, 0), coord3d(8, 6, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(8, 1, 0), signal_desc), null)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(8, 3, 0), lb_desc),     null)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(8, 5, 0), ch_desc),     null)

	local sig_simple = tile_x(8, 1, 0).find_object(mo_signal)
	local sig_lb     = tile_x(8, 3, 0).find_object(mo_signal)
	local sig_ch     = tile_x(8, 5, 0).find_object(mo_signal)

	ASSERT_TRUE(sig_simple != null)
	ASSERT_TRUE(sig_lb     != null)
	ASSERT_TRUE(sig_ch     != null)

	ASSERT_FALSE(sig_simple.is_start_signal())
	ASSERT_FALSE(sig_lb.is_start_signal())
	ASSERT_FALSE(sig_ch.is_start_signal())

	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(8, 1, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(8, 3, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(8, 5, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(
	                 pl, coord3d(8, 0, 0), coord3d(8, 6, 0), "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}




// ─── Shared helpers for convoy-based tests ────────────────────────────────────

// Build all infrastructure for tests 3-5.
// Returns the signal object at (8,7,0).
function _build_start_signal_infra(pl, rail, signal_desc)
{
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(8, 0, 0), coord3d(8, 14, 0), rail, true), null)

	local station_desc = building_desc_x.get_available_stations(
	                         building_desc_x.station, wt_rail, good_desc_x.passenger)[0]
	ASSERT_TRUE(station_desc != null)
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(8,  2, 0), station_desc), null)
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(8, 10, 0), station_desc), null)
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(8, 13, 0), station_desc), null)

	local depot_desc = get_depot_by_wt(wt_rail)
	ASSERT_TRUE(depot_desc != null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(8,  0, 0), depot_desc), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(8, 14, 0), depot_desc), null)

	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(8, 7, 0), signal_desc), null)
	local sig = tile_x(8, 7, 0).find_object(mo_signal)
	ASSERT_TRUE(sig != null)
	return sig
}

function _remove_start_signal_infra(pl)
{
	local sig = tile_x(8, 7, 0).find_object(mo_signal)
	if (sig != null) {
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(8, 7, 0)), null)
	}
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(8,  2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(8, 10, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(8, 13, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(8,  0, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(8, 14, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(
	    pl, coord3d(8, 0, 0), coord3d(8, 14, 0), "" + wt_rail), null)
}

// convoy_1: depot_A(8,0) → station_A(8,2) → station_B(8,10), departs immediately.
function _start_convoy_1(pl)
{
	local depot = depot_x(8, 0, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("H-Trans-Pantheress"))
	local cnv = depot.get_convoy_list()[0]
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Tiger-Car"))
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Tiger-Car"))
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Pantheress-Back"))
	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(8,  2, 0), 0, 0),
		schedule_entry_x(coord3d(8, 10, 0), 0, 0)
	]))
	depot.start_all_convoys(pl)
	sleep()
	return cnv
}

// convoy_2: blocker.  depot_B(8,14) → station_B(8,10) with 200% min-load
// so it never departs; its reserved track keeps block y=8..y=10 occupied.
function _start_convoy_2(pl)
{
	local depot = depot_x(8, 14, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("H-Trans-Pantheress"))
	local cnv = depot.get_convoy_list()[0]
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Tiger-Car"))
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Tiger-Car"))
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Pantheress-Back"))
	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(8, 10, 0), 200, 0)  // 200% min-load → stays at station_B
		schedule_entry_x(coord3d(8, 13, 0), 200, 0)  // 200% min-load → stays at station_C
	]))
	depot.start_all_convoys(pl)
	sleep()
	return cnv
}


// ─── Test 3: is_start_signal=true → convoy stays at station while signal is RED ─
function test_start_signal_convoy_stays_at_station()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local signal_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                        @(idx, s) s.is_signal())[0]
	ASSERT_TRUE(rail        != null)
	ASSERT_TRUE(signal_desc != null)

	local sig = _build_start_signal_infra(pl, rail, signal_desc)
	command_x.set_start_signal(pl, coord3d(8, 7, 0), 1)
	ASSERT_TRUE(sig.is_start_signal())

	debug.set_game_speed(5)

	// Start blocker first; wait until it reaches station_B (y=10).
	local cnv2 = _start_convoy_2(pl)
	for (local i = 0; i < 4000 && cnv2.is_valid() && cnv2.get_pos().y > 10; i++) {
		sleep()
	}

	// Start convoy_1.
	local cnv1 = _start_convoy_1(pl)

	// Wait for convoy_1 to arrive at station_A (y=2).
	for (local i = 0; i < 4000; i++) {
		sleep()
		if (!cnv1.is_valid()) break
		if (cnv1.get_pos().y <= 2) break
	}

	// Convoy_1 should stay at station_A for 300 ticks while signal is RED.
	local stayed_at_station = false
	if (cnv1.is_valid() && cnv1.get_pos().y <= 2) {
		stayed_at_station = true
		for (local i = 0; i < 300; i++) {
			sleep()
			if (!cnv1.is_valid() || cnv1.get_pos().y > 2) {
				stayed_at_station = false
				break
			}
		}
	}

	// Remove blocker; convoy_1 should then depart and pass the signal.
	local eventually_passed = false
	if (stayed_at_station) {
		if (cnv2.is_valid()) {
			cnv2.destroy(pl)
			sleep()
			sleep()
		}
		for (local i = 0; i < 3000; i++) {
			sleep()
			if (!cnv1.is_valid()) break
			if (cnv1.get_pos().y > 7) {
				eventually_passed = true
				break
			}
		}
	}

	print("  stayed_at_station: " + stayed_at_station)
	print("  eventually_passed: " + eventually_passed)

	// ── Cleanup BEFORE assertions ─────────────────────────────────────────────
	debug.set_game_speed(1)
	if (cnv2.is_valid()) {
		cnv2.destroy(pl)
		sleep()
		sleep()
	}
	if (cnv1.is_valid()) {
		cnv1.destroy(pl)
		sleep()
		sleep()
	}
	_remove_start_signal_infra(pl)
	RESET_ALL_PLAYER_FUNDS()

	ASSERT_TRUE(stayed_at_station)
	ASSERT_TRUE(eventually_passed)
}


// ─── Test 4: is_start_signal=false → convoy leaves station even when block occupied ─
function test_start_signal_false_convoy_advances_to_signal()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local signal_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                        @(idx, s) s.is_signal())[0]
	ASSERT_TRUE(rail        != null)
	ASSERT_TRUE(signal_desc != null)

	local sig = _build_start_signal_infra(pl, rail, signal_desc)
	// is_start_signal remains false (default)
	ASSERT_FALSE(sig.is_start_signal())

	debug.set_game_speed(5)

	// Start blocker and wait until it reaches station_B.
	local cnv2 = _start_convoy_2(pl)
	for (local i = 0; i < 4000 && cnv2.is_valid() && cnv2.get_pos().y > 10; i++) {
		sleep()
	}

	// Start convoy_1.
	local cnv1 = _start_convoy_1(pl)

	// Without the flag, convoy_1 must leave station_A (y>2) even though block is occupied.
	local convoy_left_station = false
	for (local i = 0; i < 3000; i++) {
		sleep()
		if (!cnv1.is_valid()) break
		if (cnv1.get_pos().y > 2) {
			convoy_left_station = true
			break
		}
	}

	print("  convoy_left_station: " + convoy_left_station)

	// ── Cleanup BEFORE assertions ─────────────────────────────────────────────
	debug.set_game_speed(1)
	if (cnv2.is_valid()) {
		cnv2.destroy(pl)
		sleep()
		sleep()
	}
	if (cnv1.is_valid()) {
		cnv1.destroy(pl)
		sleep()
		sleep()
	}
	_remove_start_signal_infra(pl)
	RESET_ALL_PLAYER_FUNDS()

	ASSERT_TRUE(convoy_left_station)
}


// ─── Test 5: is_start_signal=true, block is clear → convoy passes normally ────
function test_start_signal_convoy_passes_when_clear()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local signal_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                        @(idx, s) s.is_signal())[0]
	ASSERT_TRUE(rail        != null)
	ASSERT_TRUE(signal_desc != null)

	local sig = _build_start_signal_infra(pl, rail, signal_desc)
	command_x.set_start_signal(pl, coord3d(8, 7, 0), 1)
	ASSERT_TRUE(sig.is_start_signal())

	debug.set_game_speed(5)

	// No blocker; convoy_1 should depart and pass the signal freely.
	local cnv1 = _start_convoy_1(pl)

	local convoy_passed = false
	for (local i = 0; i < 4000; i++) {
		sleep()
		if (!cnv1.is_valid()) break
		if (cnv1.get_pos().y > 7) {
			convoy_passed = true
			break
		}
	}

	print("  convoy_passed: " + convoy_passed)

	// ── Cleanup BEFORE assertions ─────────────────────────────────────────────
	debug.set_game_speed(1)
	if (cnv1.is_valid()) {
		cnv1.destroy(pl)
		sleep()
		sleep()
	}
	_remove_start_signal_infra(pl)
	RESET_ALL_PLAYER_FUNDS()

	ASSERT_TRUE(convoy_passed)
}
