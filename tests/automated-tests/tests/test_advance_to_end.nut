//
// Tests for "advance to end" flag on choose signals.
//

function _build_advance_test_infra(pl, rail, ch_desc, station_desc)
{
	command_x.build_way(pl, coord3d(6, 0, 0), coord3d(6, 14, 0), rail, true) // Main line
	command_x.build_way(pl, coord3d(7, 9, 0), coord3d(7, 13, 0), rail, true) // Alt line
	command_x.build_way(pl, coord3d(6, 9, 0), coord3d(7, 9, 0), rail, true)  // Branch
	command_x.build_way(pl, coord3d(7, 13, 0), coord3d(6, 13, 0), rail, true)// Merge

	command_x.build_station(pl, coord3d(6, 2, 0), station_desc)
	
	// Build a 3-tile station on Main line
	command_x.build_station(pl, coord3d(6, 10, 0), station_desc)
	command_x.build_station(pl, coord3d(6, 11, 0), station_desc)
	command_x.build_station(pl, coord3d(6, 12, 0), station_desc)

	// Build a 3-tile station on Alt line
	command_x.build_station(pl, coord3d(7, 10, 0), station_desc)
	command_x.build_station(pl, coord3d(7, 11, 0), station_desc)
	command_x.build_station(pl, coord3d(7, 12, 0), station_desc)

	local depot_desc = get_depot_by_wt(wt_rail)
	command_x.build_depot(pl, coord3d(6, 0, 0), depot_desc)

	command_x.build_sign_at(pl, coord3d(6, 7, 0), ch_desc)
	return tile_x(6, 7, 0).find_object(mo_signal)
}

function _start_blocker_convoy(pl)
{
	local depot = depot_x(6, 0, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("H-Trans-Pantheress"))
	local cnv = depot.get_convoy_list()[0]
	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(6, 10, 0), 200, 0), // Stop at 10 on Main line, wait forever
		schedule_entry_x(coord3d(6, 0, 0), 0, 0)
	]))
	depot.start_all_convoys(pl)
	sleep()
	return cnv
}

function _start_advance_test_convoy(pl)
{
	local depot = depot_x(6, 0, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("H-Trans-Pantheress")) // 1-tile train
	local cnv = depot.get_convoy_list()[0]
	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(6, 2, 0), 0, 0),
		schedule_entry_x(coord3d(6, 10, 0), 100, 0) // Target Main line, which is blocked
	]))
	depot.start_all_convoys(pl)
	sleep()
	return cnv
}


function test_advance_to_end_true_behavior()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local station_desc = building_desc_x.get_available_stations(
	                         building_desc_x.station, wt_rail, good_desc_x.passenger)[0]
	local ch_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                    @(idx, s) s.is_choose_sign())[0]

	local sig = _build_advance_test_infra(pl, rail, ch_desc, station_desc)
	
	// Ensure global setting is false so signal flag takes effect
	settings.set_advance_to_end(false)

	// Override global setting by setting signal flag
	command_x.set_advance_to_end(pl, coord3d(6, 7, 0), 1)

	debug.set_game_speed(5)
	
	local blocker = _start_blocker_convoy(pl)
	// Wait for blocker to arrive
	for (local i = 0; i < 4000; i++) {
		sleep()
		if (blocker.is_valid() && blocker.get_pos().y == 10 && blocker.get_speed() == 0) break
	}

	local cnv = _start_advance_test_convoy(pl)

	// Wait for test convoy to arrive at alt platform (x=7)
	local arrived = false
	for (local i = 0; i < 4000; i++) {
		sleep()
		if (cnv.is_valid() && cnv.get_pos().y >= 10 && cnv.get_speed() == 0 && cnv.get_pos().x == 7) {
			arrived = true
			break
		}
	}
	
	// Because advance_to_end is true, a 1-tile train on a 3-tile station starting at y=10 should stop at y=12
	ASSERT_EQUAL(cnv.get_pos().y, 12)
}

function test_advance_to_end_false_behavior()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local station_desc = building_desc_x.get_available_stations(
	                         building_desc_x.station, wt_rail, good_desc_x.passenger)[0]
	local ch_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                    @(idx, s) s.is_choose_sign())[0]

	local sig = _build_advance_test_infra(pl, rail, ch_desc, station_desc)
	
	// Ensure global setting is false so signal flag takes effect
	settings.set_advance_to_end(false)

	// Override global setting by setting signal flag
	command_x.set_advance_to_end(pl, coord3d(6, 7, 0), 0)

	debug.set_game_speed(5)

	local blocker = _start_blocker_convoy(pl)
	// Wait for blocker to arrive
	for (local i = 0; i < 4000; i++) {
		sleep()
		if (blocker.is_valid() && blocker.get_pos().y == 10 && blocker.get_speed() == 0) break
	}

	local cnv = _start_advance_test_convoy(pl)

	// Wait for convoy to arrive at alt platform (x=7)
	local arrived = false
	for (local i = 0; i < 4000; i++) {
		sleep()
		if (cnv.is_valid() && cnv.get_pos().y >= 10 && cnv.get_speed() == 0 && cnv.get_pos().x == 7) {
			arrived = true
			break
		}
	}
	
	// Because advance_to_end is false, the train should stop at the first available tile of the alt platform (y=10)
	ASSERT_EQUAL(cnv.get_pos().y, 10)
}
