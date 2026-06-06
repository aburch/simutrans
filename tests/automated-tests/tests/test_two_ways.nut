//
// Tests for "allow reverse passage" flag on signals.
//

function _build_two_ways_test_infra(pl, rail, signal_desc, station_desc)
{
	command_x.build_way(pl, coord3d(6, 0, 0), coord3d(6, 14, 0), rail, true)

	command_x.build_station(pl, coord3d(6, 2, 0), station_desc)
	command_x.build_station(pl, coord3d(6, 12, 0), station_desc)

	local depot_desc = get_depot_by_wt(wt_rail)
	command_x.build_depot(pl, coord3d(6, 0, 0), depot_desc) // North depot
	command_x.build_depot(pl, coord3d(6, 14, 0), depot_desc) // South depot

	command_x.build_sign_at(pl, coord3d(6, 7, 0), signal_desc)
	// Rotate the signal so it becomes a single-direction signal
	command_x.build_sign_at(pl, coord3d(6, 7, 0), signal_desc)
}

function _start_convoy(pl, start_y, end_y, depot_y)
{
	local depot = depot_x(6, depot_y, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("H-Trans-Pantheress"))
	local cnv = depot.get_convoy_list()[depot.get_convoy_list().len() - 1]
	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(6, start_y, 0), 0, 0),
		schedule_entry_x(coord3d(6, end_y, 0), 100, 0)
	]))
	depot.start_all_convoys(pl)
	sleep()
	return cnv
}

function test_two_ways_set_get()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]

	local signal_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                        @(idx, s) s.is_signal())[0]

	ASSERT_TRUE(signal_desc != null)

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 0, 0), coord3d(2, 6, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 5, 0), signal_desc), null)

	local sig = tile_x(2, 5, 0).find_object(mo_signal)
	ASSERT_TRUE(sig != null)

	local pos = coord3d(2, 5, 0)
	
	command_x.set_two_ways(pl, pos, 1)
	ASSERT_TRUE(sig.get_two_ways())

	command_x.set_two_ways(pl, pos, 0)
	ASSERT_FALSE(sig.get_two_ways())
}

function test_two_ways_false_blocks_reverse()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local station_desc = building_desc_x.get_available_stations(
	                         building_desc_x.station, wt_rail, good_desc_x.passenger)[0]
	local signal_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                        @(idx, s) s.is_signal())[0]

	_build_two_ways_test_infra(pl, rail, signal_desc, station_desc)
	
	// Set two_ways to false (One-way signal behavior)
	command_x.set_two_ways(pl, coord3d(6, 7, 0), 0)

	debug.set_game_speed(5)
	
	local cnv_southbound = _start_convoy(pl, 2, 12, 0)
	local southbound_reached = false
	local southbound_blocked = false
	for (local i = 0; i < 600; i++) {
		sleep()
		if (cnv_southbound.is_valid()) {
			if (cnv_southbound.get_pos().y >= 12) {
				southbound_reached = true
				break
			}
			if (cnv_southbound.get_state() == 5) { // NO_ROUTE
				southbound_blocked = true
				break
			}
		}
	}
	
	cnv_southbound.destroy(pl)
	while (cnv_southbound.is_valid()) {
		sleep()
	}

	// Start a train going North (from 12 to 2)
	local cnv_northbound = _start_convoy(pl, 12, 2, 14)
	local northbound_reached = false
	local northbound_blocked = false
	for (local i = 0; i < 600; i++) {
		sleep()
		if (cnv_northbound.is_valid()) {
			if (cnv_northbound.get_pos().y <= 2) {
				northbound_reached = true
				break
			}
			if (cnv_northbound.get_state() == 5) { // NO_ROUTE
				northbound_blocked = true
				break
			}
		}
	}
	
	// One should reach destination, one should be blocked by the one-way signal
	print("southbound_reached: " + southbound_reached)
	print("southbound_blocked: " + southbound_blocked)
	print("northbound_reached: " + northbound_reached)
	print("northbound_blocked: " + northbound_blocked)
	ASSERT_TRUE( (southbound_reached && northbound_blocked) || (southbound_blocked && northbound_reached) )
}

function test_two_ways_true_allows_reverse()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local station_desc = building_desc_x.get_available_stations(
	                         building_desc_x.station, wt_rail, good_desc_x.passenger)[0]
	local signal_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                        @(idx, s) s.is_signal())[0]

	_build_two_ways_test_infra(pl, rail, signal_desc, station_desc)
	
	// Set two_ways to true (Standard two-way signal behavior)
	command_x.set_two_ways(pl, coord3d(6, 7, 0), 1)

	debug.set_game_speed(5)
	
	// Start Southbound first
	local cnv_southbound = _start_convoy(pl, 2, 12, 0)
	local southbound_reached = false
	for (local i = 0; i < 600; i++) {
		sleep()
		if (cnv_southbound.is_valid() && cnv_southbound.get_pos().y >= 12) {
			southbound_reached = true
			break
		}
	}
	ASSERT_TRUE(southbound_reached)
	
	// Remove Southbound train so track is free
	cnv_southbound.destroy(pl)
	while (cnv_southbound.is_valid()) {
		sleep()
	}

	// Start Northbound train
	local cnv_northbound = _start_convoy(pl, 12, 2, 14)
	local northbound_reached = false
	for (local i = 0; i < 600; i++) {
		sleep()
		if (cnv_northbound.is_valid() && cnv_northbound.get_pos().y <= 2) {
			northbound_reached = true
			break
		}
	}
	
	ASSERT_TRUE(northbound_reached)
}
