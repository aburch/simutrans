//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//

// --- Property 1: schedule_entry_x.maximum_load ---

function test_schedule_entry_maximum_load()
{
	local pl = player_x(0)

	// Build minimal rail infrastructure: track, two stations, depot
	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "sand_track"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 7, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("TrainDepot")), null)

	local depot = depot_x(4, 8, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	local cnv = depot.get_convoy_list()[0]

	// Set initial schedule (convoy stays in depot)
	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),
		schedule_entry_x(coord3d(4, 7, 0), 0, 0),
	]))

	// Read the current schedule and set non-default maximum_load values
	local sched = cnv.get_schedule()
	sched.entries[0].maximum_load = 75
	sched.entries[1].maximum_load = 50
	// change_schedule returns true in non-network mode (tool runs synchronously)
	cnv.change_schedule(pl, sched)

	// Read back and verify the values survived the round-trip
	local sched2 = cnv.get_schedule()
	ASSERT_EQUAL(sched2.entries[0].maximum_load, 75)
	ASSERT_EQUAL(sched2.entries[1].maximum_load, 50)
}

// --- Property 2: schedule_entry_x.spacing ---

function test_schedule_entry_spacing()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "sand_track"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 7, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("TrainDepot")), null)

	local depot = depot_x(4, 8, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	local cnv = depot.get_convoy_list()[0]

	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),
		schedule_entry_x(coord3d(4, 7, 0), 0, 0),
	]))

	local sched = cnv.get_schedule()
	sched.entries[0].spacing = 3
	sched.entries[0].spacing_shift = 2
	sched.entries[0].delay_tolerance = 1
	cnv.change_schedule(pl, sched)

	local sched2 = cnv.get_schedule()
	ASSERT_EQUAL(sched2.entries[0].spacing, 3)
	ASSERT_EQUAL(sched2.entries[0].spacing_shift, 2)
	ASSERT_EQUAL(sched2.entries[0].delay_tolerance, 1)
}

// --- Property 3: schedule_entry_x.length_coupling_done ---

function test_schedule_entry_length_coupling_done()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "sand_track"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 7, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("TrainDepot")), null)

	local depot = depot_x(4, 8, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	local cnv = depot.get_convoy_list()[0]

	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),
		schedule_entry_x(coord3d(4, 7, 0), 0, 0),
	]))

	local sched = cnv.get_schedule()
	sched.entries[0].length_coupling_done = 8
	cnv.change_schedule(pl, sched)

	local sched2 = cnv.get_schedule()
	ASSERT_EQUAL(sched2.entries[0].length_coupling_done, 8)
}

// --- Property 4: schedule_entry_x.max_speed ---

function test_schedule_entry_max_speed()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "sand_track"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 7, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("TrainDepot")), null)

	local depot = depot_x(4, 8, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	local cnv = depot.get_convoy_list()[0]

	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),
		schedule_entry_x(coord3d(4, 7, 0), 0, 0),
	]))

	local sched = cnv.get_schedule()
	sched.entries[0].max_speed = 120
	cnv.change_schedule(pl, sched)

	local sched2 = cnv.get_schedule()
	ASSERT_EQUAL(sched2.entries[0].max_speed, 120)
}

// --- Property 5: schedule_entry_x.balance_speed ---

function test_schedule_entry_balance_speed()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "sand_track"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 7, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("TrainDepot")), null)

	local depot = depot_x(4, 8, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	local cnv = depot.get_convoy_list()[0]

	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),
		schedule_entry_x(coord3d(4, 7, 0), 0, 0),
	]))

	local sched = cnv.get_schedule()
	sched.entries[0].balance_speed = 80
	cnv.change_schedule(pl, sched)

	local sched2 = cnv.get_schedule()
	ASSERT_EQUAL(sched2.entries[0].balance_speed, 80)
}

// --- Property 6: schedule_x.flags ---

function test_schedule_flags()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "sand_track"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 7, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("TrainDepot")), null)

	local depot = depot_x(4, 8, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	local cnv = depot.get_convoy_list()[0]

	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),
		schedule_entry_x(coord3d(4, 7, 0), 0, 0),
	]))

	// SAME_DEP_TIME=2, FULL_LOAD_ACCELERATION=4 => flags=6
	local sched = cnv.get_schedule()
	sched.flags = 6
	cnv.change_schedule(pl, sched)

	local sched2 = cnv.get_schedule()
	ASSERT_EQUAL(sched2.flags, 6)
}

// --- Property 7: schedule_x.max_speed ---

function test_schedule_max_speed()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "sand_track"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 7, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("TrainDepot")), null)

	local depot = depot_x(4, 8, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	local cnv = depot.get_convoy_list()[0]

	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),
		schedule_entry_x(coord3d(4, 7, 0), 0, 0),
	]))

	local sched = cnv.get_schedule()
	sched.max_speed = 100
	cnv.change_schedule(pl, sched)

	local sched2 = cnv.get_schedule()
	ASSERT_EQUAL(sched2.max_speed, 100)
}

// --- Property 8: schedule_x.departure_slot_group_id ---

function test_schedule_departure_slot_group_id()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "sand_track"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 7, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("TrainDepot")), null)

	local depot = depot_x(4, 8, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	local cnv = depot.get_convoy_list()[0]

	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),
		schedule_entry_x(coord3d(4, 7, 0), 0, 0),
	]))

	// departure_slot_group_id defaults to null when not assigned to any group
	local sched = cnv.get_schedule()
	ASSERT_EQUAL(sched.departure_slot_group_id, null)
	// round-trip preserves null
	cnv.change_schedule(pl, sched)
	local sched2 = cnv.get_schedule()
	ASSERT_EQUAL(sched2.departure_slot_group_id, null)
}

function test_schedule_departure_slot_group_id_non_null()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "sand_track"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 7, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("TrainDepot")), null)

	local depot = depot_x(4, 8, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	local cnv = depot.get_convoy_list()[0]

	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),
		schedule_entry_x(coord3d(4, 7, 0), 0, 0),
	]))

	ASSERT_EQUAL(pl.create_line(wt_rail), true)
	local line_list = pl.get_line_list()
	local line = line_list[line_list.get_count() - 1]

	// Set departure_slot_group_id to an existing line and verify round-trip
	local sched = cnv.get_schedule()
	sched.departure_slot_group_id = line
	cnv.change_schedule(pl, sched)

	local sched2 = cnv.get_schedule()
	ASSERT_TRUE(sched2.departure_slot_group_id != null)
	ASSERT_EQUAL(sched2.departure_slot_group_id.get_name(), line.get_name())
}

// --- Property 9: schedule_x.next_line ---

function test_schedule_next_line()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "sand_track"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 7, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("TrainDepot")), null)

	local depot = depot_x(4, 8, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	local cnv = depot.get_convoy_list()[0]

	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),
		schedule_entry_x(coord3d(4, 7, 0), 0, 0),
	]))

	// next_line defaults to null when no connected route is set
	local sched = cnv.get_schedule()
	ASSERT_EQUAL(sched.next_line, null)
	// round-trip preserves null
	cnv.change_schedule(pl, sched)
	local sched2 = cnv.get_schedule()
	ASSERT_EQUAL(sched2.next_line, null)
}

function test_schedule_next_line_non_null()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "sand_track"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 7, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("TrainDepot")), null)

	local depot = depot_x(4, 8, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	local cnv = depot.get_convoy_list()[0]

	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),
		schedule_entry_x(coord3d(4, 7, 0), 0, 0),
	]))

	ASSERT_EQUAL(pl.create_line(wt_rail), true)
	local line_list = pl.get_line_list()
	local line = line_list[line_list.get_count() - 1]

	// Give the line a valid schedule so it passes set_next_line() validity checks
	line.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),
		schedule_entry_x(coord3d(4, 7, 0), 0, 0),
	]))

	// Set next_line via set_next_line() and verify round-trip
	local sched = cnv.get_schedule()
	sched.set_next_line(line)
	cnv.change_schedule(pl, sched)

	local sched2 = cnv.get_schedule()
	ASSERT_EQUAL(sched2.next_line.get_name(), line.get_name())
}

// --- Property 10: schedule_x.current ---

function test_schedule_current()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "sand_track"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 7, 0), "TrainStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("TrainDepot")), null)

	local depot = depot_x(4, 8, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("1Diesellokomotive"))
	local cnv = depot.get_convoy_list()[0]

	cnv.change_schedule(pl, schedule_x(wt_rail, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),
		schedule_entry_x(coord3d(4, 7, 0), 0, 0),
	]))

	local sched = cnv.get_schedule()
	sched.current = 1
	cnv.change_schedule(pl, sched)

	local sched2 = cnv.get_schedule()
	ASSERT_EQUAL(sched2.current, 1)
}


// --- Properties: schedule_entry_x journey_time / waiting_time / convoy_stopping_time ---

function test_schedule_entry_time_statistics()
{
	local pl = player_x(0)

	// Build 3-stop road route: A(4,2) - B(4,5) - C(4,8) - depot(4,9)
	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 9, 0), "cobblestone_road"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "BusStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 5, 0), "BusStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 8, 0), "BusStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 9, 0), building_desc_x("CarDepot")), null)

	// Create line: stats are recorded on the line's schedule, not the convoy's.
	ASSERT_TRUE(pl.create_line(wt_road))
	local line_list = pl.get_line_list()
	local line = line_list[line_list.get_count() - 1]
	local the_schedule = schedule_x(wt_road, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),
		schedule_entry_x(coord3d(4, 5, 0), 0, 0),
		schedule_entry_x(coord3d(4, 8, 0), 0, 0),
	])
	line.change_schedule(pl, the_schedule)

	local halt_b = halt_x.get_halt(coord3d(4, 5, 0), pl)

	local depot = depot_x(4, 9, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("Buessig"))
	local cnv = depot.get_convoy_list()[0]
	// Assign to line while in depot (state=INITIAL) to avoid EDIT_SCHEDULE on set_line.
	// This copies the line's schedule to the convoy without triggering re-routing.
	cnv.set_line(pl, line)
	depot.start_all_convoys(pl)
	debug.set_game_speed(5)

	// Wait until A->B routing graph is established (generate_goods returns ROUTE_OK=1).
	while (world.generate_goods(coord(3, 2), coord(3, 5), good_desc_x.passenger, 1) == 0) {
		sleep()
	}

	// Count 4 B arrivals after routing is ready to ensure 3 A visits with passengers.
	local base_b = halt_b.convoys[0]
	while (halt_b.convoys[0] < base_b + 4) {
		world.generate_goods(coord(3, 2), coord(3, 5), good_desc_x.passenger, 30)
		sleep()
	}

	// Read stats from the LINE's schedule (not the convoy's schedule).
	local sched = line.get_schedule()
	local entry_b = sched.entries[1]

	// journey_time at B: A->B travel time, recorded on each arrival at B
	ASSERT_EQUAL(typeof entry_b.journey_time, "array")
	ASSERT_EQUAL(entry_b.journey_time.len(), 5)
	ASSERT_TRUE(entry_b.journey_time[0] > 0)
	ASSERT_TRUE(entry_b.journey_time[1] > 0)
	ASSERT_TRUE(entry_b.journey_time[2] > 0)
}
