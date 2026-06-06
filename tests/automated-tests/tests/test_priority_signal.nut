//
// Priority signal test for PR 565
//

function test_priority_signal_reserve()
{
	local pl = player_x(0)
	local rail = way_desc_x("wooden_sleeper_track")
	ASSERT_TRUE(rail != null)

	// Layout at x=1
	// y=0: Depot
	// y=2: Priority Signal 1
	// y=4: Station A
	// y=6: Priority Signal 2
	// y=7: Switch to branch
	// y=9: Branch Station B (x=2, y=9)
	// y=10: Signal on branch (exit of B)
	// y=11: Merge back to x=1
	// y=13: Signal on main line
	// y=14: Station C
	// y=15: Station D

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 0, 0), coord3d(1, 15, 0), rail, true), null)
	// Branch
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 7, 0), coord3d(2, 8, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 8, 0), coord3d(2, 10, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 10, 0), coord3d(1, 11, 0), rail, true), null)

	local depot_desc = building_desc_x("TrainDepot")
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(1, 0, 0), depot_desc), null)

	local station_desc = building_desc_x("TrainStop")
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(1, 4, 0), station_desc), null) // A
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(2, 9, 0), station_desc), null) // B
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(1, 14, 0), station_desc), null) // C
	ASSERT_EQUAL(command_x.build_station(pl, coord3d(1, 15, 0), station_desc), null) // D

	local pri_desc = sign_desc_x.get_available_signs(wt_rail).filter(@(idx, s) s.is_priority_signal())[0]
	local sig_desc = sign_desc_x.get_available_signs(wt_rail).filter(@(idx, s) s.is_signal())[0]

	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(1, 2, 0), pri_desc), null) // Pri 1
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(1, 6, 0), pri_desc), null) // Pri 2
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 10, 0), sig_desc), null) // Exit B
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(1, 13, 0), sig_desc), null) // Mainline 

	local depot = depot_x(1, 0, 0)
	
	// Train A
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	local cnvA = depot.get_convoy_list()[0]
	cnvA.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(1, 4, 0), 100, 0), // A
		schedule_entry_x(coord3d(2, 9, 0), 0, 0), // B
		schedule_entry_x(coord3d(1, 14, 0), 0, 0)  // C
	]))
	
	debug.set_game_speed(5)
	depot.start_all_convoys(pl)


	// Wait for A to reach Station A
	for (local i=0; i<1000; i++) {
		if (cnvA.get_pos().y == 4) break;
		sleep()
	}

	// Now spawn Train B
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	local cnvB = depot.get_convoy_list()[0] // It's now the only one in the depot!
	cnvB.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(1, 14, 0), 0, 0), // C
		schedule_entry_x(coord3d(1, 15, 0), 0, 0)  // D
	]))

	depot.start_all_convoys(pl)


	// Train B spawns after A leaves depot. Train B goes to priority signal 1, then reserves up to Train A.
	// We wait until Train B reaches y=3 (just before Train A)
	for (local i=0; i<2000; i++) {
		if (cnvB.get_pos().y == 3) break;
		sleep()
	}

	ASSERT_EQUAL(cnvB.get_pos().y, 3)


	// Now B has reserved up to y=3.
	// Change A's schedule to 0% load so it departs to B.
	cnvA.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(1, 4, 0), 0, 0), // A
		schedule_entry_x(coord3d(2, 9, 0), 0, 0), // B
		schedule_entry_x(coord3d(1, 14, 0), 0, 0)  // C
	]))
	

	// A should depart and go to B (y=9).
	local reached_B = false
	for (local i=0; i<2000; i++) {
		if (cnvA.get_pos().y == 9 && cnvA.get_pos().x == 2) {
			reached_B = true
			break
		}
		sleep()
	}
	ASSERT_TRUE(reached_B)

	// We check if A reserved past B. If so, signal at y=10 (exit of B) should be green!
	local sig_exit_B = tile_x(2, 10, 0).find_object(mo_signal)
	ASSERT_TRUE(sig_exit_B != null)

	local is_green = false
	for (local i=0; i<2000; i++) {
		if (sig_exit_B.get_state() == state_green) {
			is_green = true
			break
		}
		sleep()
	}
	


	debug.set_game_speed(1)
	ASSERT_TRUE(is_green)
}
