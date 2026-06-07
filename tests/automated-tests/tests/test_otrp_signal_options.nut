//
// Tests for OTRP specific signal options
//

function get_veh_by_wt(wt) {
	local list = vehicle_desc_x.get_available_vehicles(wt)
	foreach (v in list) {
		if (v.get_power() > 0 && v.get_capacity() > 0 && !v.needs_electrification()) return v
	}
	// Fallback if no non-electric railcar exists
	foreach (v in list) {
		if (v.get_power() > 0 && !v.needs_electrification()) return v
	}
	return null
}

function get_wagon_by_wt(wt) {
	local list = vehicle_desc_x.get_available_vehicles(wt)
	foreach (v in list) {
		if (v.get_capacity() > 0) return v
	}
	return null
}

function _build_otrp_test_infra(pl, rail, sig_desc, station_desc, alt_plat_length = 3)
{
	local sig_pos = coord3d(6,7,0)
	command_x.build_way(pl, coord3d(6, 0, 0), coord3d(6, 14, 0), rail, true) // Main line
	command_x.build_way(pl, coord3d(7, 9, 0), coord3d(7, 13, 0), rail, true) // Alt line
	command_x.build_way(pl, coord3d(6, 9, 0), coord3d(7, 9, 0), rail, true)  // Branch
	command_x.build_way(pl, coord3d(7, 13, 0), coord3d(6, 13, 0), rail, true)// Merge

	command_x.build_station(pl, coord3d(6, 2, 0), station_desc)
	
	// Build a 4-tile station on Main line
	command_x.build_station(pl, coord3d(6, 10, 0), station_desc)
	command_x.build_station(pl, coord3d(6, 11, 0), station_desc)
	command_x.build_station(pl, coord3d(6, 12, 0), station_desc)
	command_x.build_station(pl, coord3d(6, 13, 0), station_desc)

	// Build a variable length station on Alt line
	for (local y = 9; y < 9 + alt_plat_length + 1; y++) {
		command_x.build_station(pl, coord3d(7, y, 0), station_desc)
	}

	local depot_desc = get_depot_by_wt(wt_rail)
	command_x.build_depot(pl, coord3d(6, 0, 0), depot_desc)
	command_x.build_depot(pl, coord3d(6, 14, 0), depot_desc) // Blocker depot

	command_x.build_sign_at(pl, sig_pos, sig_desc)
	return tile_x(sig_pos.x, sig_pos.y, sig_pos.z).find_object(mo_signal)
}

function _start_blocker_convoy(pl)
{
	local target_pos = coord3d(6,10,0)
	local depot = depot_x(6, 14, 0)
	local veh_desc = get_veh_by_wt(wt_rail)
	local wagon_desc = veh_desc.get_capacity() == 0 ? get_wagon_by_wt(wt_rail) : null
	depot.append_vehicle(pl, convoy_x(0), veh_desc)
	local cnv = depot.get_convoy_list()[0]
	if (wagon_desc) depot.append_vehicle(pl, cnv, wagon_desc)
	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(target_pos, 200, 0), // Stop at target, wait forever
		schedule_entry_x(coord3d(6, 14, 0), 0, 0)
	]))
	depot.start_all_convoys(pl)
	sleep()
	return cnv
}

function _start_test_convoy(pl)
{
	local depot = depot_x(6, 0, 0)
	local veh_desc = get_veh_by_wt(wt_rail)
	local wagon_desc = veh_desc.get_capacity() == 0 ? get_wagon_by_wt(wt_rail) : null
	depot.append_vehicle(pl, convoy_x(0), veh_desc)
	local cnv = depot.get_convoy_list()[0]
	if (wagon_desc) depot.append_vehicle(pl, cnv, wagon_desc)
	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(6, 2, 0), 0, 0),
		schedule_entry_x(coord3d(6, 10, 0), 100, 0) // Target Main line
	]))
	depot.start_all_convoys(pl)
	sleep()
	return cnv
}

function test_otrp_signal_options_roundtrip()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local ch_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                    @(idx, s) s.is_choose_sign())[0]

	local pos = coord3d(2, 10, 0)
	command_x.build_way(pl, coord3d(2, 9, 0), coord3d(2, 12, 0), rail, true)
	command_x.build_sign_at(pl, pos, ch_desc)

	local sig = tile_x(pos.x, pos.y, pos.z).find_object(mo_signal)
	ASSERT_TRUE(sig != null)
	
	// Test start_signal
	command_x.set_start_signal(pl, pos, 1)
	ASSERT_TRUE(sig.is_start_signal())
	command_x.set_start_signal(pl, pos, 0)
	ASSERT_FALSE(sig.is_start_signal())

	// Test advance_to_end
	command_x.set_advance_to_end(pl, pos, 1)
	ASSERT_TRUE(sig.is_advance_to_end())
	command_x.set_advance_to_end(pl, pos, 0)
	ASSERT_FALSE(sig.is_advance_to_end())

	// Test two_ways
	command_x.set_two_ways(pl, pos, 1)
	ASSERT_TRUE(sig.get_two_ways())
	command_x.set_two_ways(pl, pos, 0)
	ASSERT_FALSE(sig.get_two_ways())

	// Test guide_signal
	command_x.set_guide_signal(pl, pos, 1)
	ASSERT_TRUE(sig.is_guide_signal())
	command_x.set_guide_signal(pl, pos, 0)
	ASSERT_FALSE(sig.is_guide_signal())

	// Test choose_signal
	command_x.set_choose_signal(pl, pos, 1)
	ASSERT_TRUE(sig.is_choose_signal())
	command_x.set_choose_signal(pl, pos, 0)
	ASSERT_FALSE(sig.is_choose_signal())

	// Test skip_default_route
	command_x.set_skip_default_route(pl, pos, 1)
	ASSERT_TRUE(sig.is_skip_default_route())
	command_x.set_skip_default_route(pl, pos, 0)
	ASSERT_FALSE(sig.is_skip_default_route())

	// Test length_based
	command_x.set_length_based(pl, pos, 1)
	ASSERT_TRUE(sig.is_length_based())
	command_x.set_length_based(pl, pos, 0)
	ASSERT_FALSE(sig.is_length_based())

	// Test margin_length
	command_x.set_margin_length(pl, pos, 5)
	ASSERT_EQUAL(sig.get_margin_length(), 5)
}

function test_choose_signal_false_behavior()
{
	// If choose_signal is set to false, it should act like a normal signal and NOT redirect.
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local station_desc = building_desc_x.get_available_stations(
	                         building_desc_x.station, wt_rail, good_desc_x.passenger)[0]
	local ch_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                    @(idx, s) s.is_choose_sign())[0]

	local sig = _build_otrp_test_infra(pl, rail, ch_desc, station_desc)
	
	// Turn OFF the choose signal functionality
	command_x.set_choose_signal(pl, coord3d(6, 7, 0), 0)

	debug.set_game_speed(5)
	
	local cnv = _start_test_convoy(pl)
	sleep() // let test convoy generate its route to x=6
	
	local blocker = _start_blocker_convoy(pl)
	for (local i = 0; i < 4000; i++) {
		sleep()
		if (blocker.is_valid() && blocker.get_pos().y >= 10 && blocker.get_pos().y <= 12 && blocker.get_state() == 7) break
	}

	for (local i = 0; i < 4000; i++) {
		sleep()
		// If choose_signal is OFF, it will just wait at the signal (y=7)
		local state = cnv.get_state()
		if (cnv.is_valid() && cnv.get_pos().y == 7) {
			if (state == 8 || state == 9 || state == 13) break
		}
	}
	
	// Because choose is false, it should wait at the signal, not routing to alt line.
	// It should stop at the signal (y=7)
	ASSERT_EQUAL(cnv.get_pos().x, 6)
	ASSERT_EQUAL(cnv.get_pos().y, 7)
}

function test_skip_default_route_false_behavior()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local station_desc = building_desc_x.get_available_stations(
	                         building_desc_x.station, wt_rail, good_desc_x.passenger)[0]
	local ch_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                    @(idx, s) s.is_choose_sign())[0]

	// Custom layout
	command_x.build_way(pl, coord3d(2, 0, 0), coord3d(2, 4, 0), rail, true) // Main approach

	// Alt line goes STRAIGHT (Shorter path, length = 5)
	command_x.build_way(pl, coord3d(2, 4, 0), coord3d(2, 10, 0), rail, true)

	// Main line branches off, zigzags, making it LONGER (length = 7)
	command_x.build_way(pl, coord3d(2, 4, 0), coord3d(3, 4, 0), rail, true)
	command_x.build_way(pl, coord3d(3, 4, 0), coord3d(3, 9, 0), rail, true)
	command_x.build_way(pl, coord3d(3, 9, 0), coord3d(2, 9, 0), rail, true) // Merge back
	
	command_x.build_station(pl, coord3d(2, 2, 0), station_desc) // Start station
	
	// Platforms are orthogonally adjacent to guarantee they form ONE station!
	// Alt line platform (Shorter path, x=2)
	command_x.build_station(pl, coord3d(2, 6, 0), station_desc)
	command_x.build_station(pl, coord3d(2, 7, 0), station_desc)

	// Main line platform (Longer path, default route, x=3)
	command_x.build_station(pl, coord3d(3, 6, 0), station_desc)
	command_x.build_station(pl, coord3d(3, 7, 0), station_desc)

	local depot_desc = get_depot_by_wt(wt_rail)
	command_x.build_depot(pl, coord3d(2, 0, 0), depot_desc)

	command_x.build_sign_at(pl, coord3d(2, 3, 0), ch_desc) // Signal right before branch at y=4
	
	// Ensure the choose signal is ON
	command_x.set_choose_signal(pl, coord3d(2, 3, 0), 1)
	
	// We want to test that when skip_default_route is FALSE (0),
	// it MUST pick the default route even if there's a shorter alternate route!
	command_x.set_skip_default_route(pl, coord3d(2, 3, 0), 0)

	debug.set_game_speed(5)

	// No blocker! Both routes are open.
	local depot = depot_x(2, 0, 0)
	local veh_desc = get_veh_by_wt(wt_rail)
	local wagon_desc = veh_desc.get_capacity() == 0 ? get_wagon_by_wt(wt_rail) : null
	depot.append_vehicle(pl, convoy_x(0), veh_desc)
	local cnv = depot.get_convoy_list()[0]
	if (wagon_desc) depot.append_vehicle(pl, cnv, wagon_desc)
	
	local sched = schedule_x(wt_rail, [
		schedule_entry_x(coord3d(2, 2, 0), 0, 0),
		schedule_entry_x(coord3d(3, 7, 0), 0, 0) // Default destination is Main line (x=3, Longer)
	])
	sched.entries[1].spacing = 1 // 1 per month departure time wait
	cnv.change_schedule(pl, sched)
	
	depot.start_all_convoys(pl)
	sleep()

	for (local i = 0; i < 4000; i++) {
		sleep()
		if (cnv.is_valid() && cnv.get_pos().y >= 6) {
			// Train has arrived at the destination platform (y=6 or 7)
			// Because spacing = 1, it will wait here, so we can reliably validate its position!
			break
		}
	}
	
	// Because skip_default_route is OFF, it checks the scheduled default route FIRST.
	// Since Main line (x=3) is empty, it blindly takes it, even though Alt line (x=2) is shorter!
	ASSERT_EQUAL(cnv.get_pos().x, 3)
}

function test_margin_length_behavior()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local station_desc = building_desc_x.get_available_stations(
	                         building_desc_x.station, wt_rail, good_desc_x.passenger)[0]
	local ch_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                    @(idx, s) s.is_choose_sign())[0]

	local sig = _build_otrp_test_infra(pl, rail, ch_desc, station_desc)
	
	// Settings
	settings.set_advance_to_end(false)
	command_x.set_choose_signal(pl, coord3d(6, 7, 0), 1)
	command_x.set_advance_to_end(pl, coord3d(6, 7, 0), 1)
	
	// Set margin length = 1
	command_x.set_margin_length(pl, coord3d(6, 7, 0), 1)

	debug.set_game_speed(5)
	
	local cnv = _start_test_convoy(pl)
	sleep()
	
	local blocker = _start_blocker_convoy(pl)
	for (local i = 0; i < 4000; i++) {
		sleep()
		if (blocker.is_valid() && blocker.get_pos().y >= 10 && blocker.get_pos().y <= 12 && blocker.get_state() == 7) break
	}

	for (local i = 0; i < 4000; i++) {
		sleep()
		if (cnv.is_valid() && cnv.get_pos().y >= 10 && cnv.get_pos().x == 7 && cnv.get_state() == 7) {
			break
		}
	}
	
	// Station is from y=10 to y=12. Normal advance_to_end stops at y=12. 
	// Margin=1 means it stops 1 tile earlier -> y=11.
	// Since convoy length might be 2, it might stop at y=10.
	local y = cnv.get_pos().y
	ASSERT_TRUE(y == 10 || y == 11)
}

function test_length_based_behavior()
{
	local pl   = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local station_desc = building_desc_x.get_available_stations(
	                         building_desc_x.station, wt_rail, good_desc_x.passenger)[0]
	local ch_desc = sign_desc_x.get_available_signs(wt_rail).filter(
	                    @(idx, s) s.is_choose_sign())[0]

	local sig_pos = coord3d(6,7,0)
	
	// Approach line
	command_x.build_way(pl, coord3d(6, 0, 0), coord3d(6, 8, 0), rail, true)

	// Cross branch track
	command_x.build_way(pl, coord3d(5, 8, 0), coord3d(7, 8, 0), rail, true)

	// 3 lines going down from y=8
	command_x.build_way(pl, coord3d(5, 8, 0), coord3d(5, 11, 0), rail, true) // x=5: 2 tiles (y=10..11)
	command_x.build_way(pl, coord3d(6, 8, 0), coord3d(6, 13, 0), rail, true) // x=6: 4 tiles (y=10..13)
	command_x.build_way(pl, coord3d(7, 8, 0), coord3d(7, 15, 0), rail, true) // x=7: 6 tiles (y=10..15)

	command_x.build_station(pl, coord3d(6, 5, 0), station_desc)

	// Build platforms
	// x=5, length 2
	command_x.build_station(pl, coord3d(5, 10, 0), station_desc)
	command_x.build_station(pl, coord3d(5, 11, 0), station_desc)

	// x=6, length 4
	command_x.build_station(pl, coord3d(6, 10, 0), station_desc)
	command_x.build_station(pl, coord3d(6, 11, 0), station_desc)
	command_x.build_station(pl, coord3d(6, 12, 0), station_desc)
	command_x.build_station(pl, coord3d(6, 13, 0), station_desc)

	// x=7, length 6
	for (local y = 10; y <= 15; y++) {
		command_x.build_station(pl, coord3d(7, y, 0), station_desc)
	}

	local depot_desc = get_depot_by_wt(wt_rail)
	command_x.build_depot(pl, coord3d(6, 0, 0), depot_desc)
	command_x.build_sign_at(pl, sig_pos, ch_desc)
	
	// Configure signal
	command_x.set_choose_signal(pl, sig_pos, 1)
	command_x.set_skip_default_route(pl, sig_pos, 1) // Force find_route
	command_x.set_length_based(pl, sig_pos, 1)

	debug.set_game_speed(5)
	
	// Build 4-tile long convoy using known vehicle combination.
	// Each car is length 8 (0.5 tiles), so 8 cars = 4 tiles.
	local depot = depot_x(6, 0, 0)
	
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("H-Trans-Pantheress"))
	local cnv = depot.get_convoy_list()[0]
	for (local i = 0; i < 6; i++) {
		depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Tiger-Car"))
	}
	depot.append_vehicle(pl, cnv, vehicle_desc_x("H-Trans-Pantheress-Back"))

	// Set default route to the 6-tile platform (x=7)
	local sched = schedule_x(wt_rail, [
		schedule_entry_x(coord3d(6, 5, 0), 0, 0),
		schedule_entry_x(coord3d(7, 10, 0), 0, 0)
	])
	cnv.change_schedule(pl, sched)
	depot.start_all_convoys(pl)

	for (local i = 0; i < 4000; i++) {
		sleep()
		if (cnv.is_valid() && cnv.get_pos().y >= 11) {
			break
		}
	}
	
	// Because length_based is true, the 2-tile is too short.
	// Among 4-tile and 6-tile, the 4-tile is chosen.
	ASSERT_EQUAL(cnv.get_pos().x, 6)
}
