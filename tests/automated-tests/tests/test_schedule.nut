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
