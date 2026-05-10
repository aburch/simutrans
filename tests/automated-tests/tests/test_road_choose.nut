//
// Tests for road_vehicle_t::is_target when a choose sign routes a bus
// behind another bus that is already parked at the far stop tile.
//
// Layout (x=5):
//   y=0  CarDepot
//   y=2  road + choose sign (south-facing, built twice)
//   y=3  BusStop  (near tile)
//   y=4  BusStop  (far  tile)  <- Bus A parked here
//   y=5  road dead-end
//
// Bus A targets far stop (y=4) with min_load=200 so it never departs.
// Bus B also targets far stop (y=4); the choose sign re-routes it.
//

local OT_HALT         = -1   // halt_mode
local OT_ONEWAY       =  0   // oneway_mode
local OT_TWOWAY       =  1   // twoway_mode
local OT_LOADING_ONLY =  2   // loading_only_mode
local OT_PROHIBITED   =  3   // prohibited_mode
local OT_INVERTED     =  4   // inverted_mode

local RC_X    = 5   // column used by all road-choose tests
local RC_NEAR = 3   // y of near stop tile
local RC_FAR  = 4   // y of far  stop tile


// ── Infrastructure helpers ────────────────────────────────────────────────

function _rc_build(mode)
{
	local pl    = player_x(0)
	local x     = RC_X
	local road  = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local stop  = building_desc_x("BusStop")
	local depot = building_desc_x("CarDepot")
	local sign  = sign_desc_x.get_available_signs(wt_road).filter(
	                  @(idx, s) s.is_choose_sign())[0]

	ASSERT_TRUE(road  != null)
	ASSERT_TRUE(stop  != null)
	ASSERT_TRUE(depot != null)
	ASSERT_TRUE(sign  != null)

	// Road y=0..5 with the requested overtaking mode
	ASSERT_EQUAL(command_x.build_road(pl, coord3d(x, 0, 0), coord3d(x, 5, 0),
	                                  road, false, {overtaking_mode = mode}), null)

	// Depot at y=0 (overwrites road tile)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(x, 0, 0), depot), null)

	// Choose sign at y=2, built twice -> direction becomes south
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(x, 2, 0), sign), null)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(x, 2, 0), sign), null)

	// Near (y=3) and far (y=4) stops
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(x, 3, 0),
	                                                stop.get_name()), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(x, 4, 0),
	                                                stop.get_name()), null)
}


// ── bus running helper ───────────────────────────────────────────────────────

function _rc_run(target_y)
{
	local pl = player_x(0)
	local x  = RC_X
	local dep = depot_x(x, 0, 0)
	local sched = schedule_x(wt_road, [
		schedule_entry_x(coord3d(x, RC_FAR, 0), 200, 0),
		schedule_entry_x(coord3d(x, 5,      0),   0, 0)
	])

	debug.set_game_speed(5)

	// Bus: park at far stop forever
	dep.append_vehicle(pl, convoy_x(0), vehicle_desc_x("Buessig"))
	local cnv = dep.get_convoy_list()[0]
	cnv.change_schedule(pl, sched)
	dep.start_all_convoys(pl)

	for (local i = 0; i < 3000; i++) {
		sleep()
		if (!cnv.is_valid()) break
		local cnv_stopping = cnv.is_waiting() || cnv.is_loading()
		// Stopped at the target y pos -> return
		if (cnv.get_pos().y == target_y && cnv_stopping) break
	}
	ASSERT_TRUE(cnv.is_valid())

	return cnv
}


// ── Tests ─────────────────────────────────────────────────────────────────

// halt_mode: both lanes of the far stop fill up; Bus C waits before the choose sign
function test_road_choose_stop_behind_halt_mode()
{
	_rc_build(OT_HALT)
	local cnv_a = _rc_run(RC_FAR)
	local cnv_b = _rc_run(RC_FAR)
	local cnv_c = _rc_run(1)
	ASSERT_EQUAL(cnv_a.get_pos().y, RC_FAR)
	ASSERT_EQUAL(cnv_b.get_pos().y, RC_FAR)
	ASSERT_EQUAL(cnv_c.get_pos().y, 1) // before the choose sign
}

// oneway/twoway/loading_only/inverted: Bus B stops at the near tile
function test_road_choose_stop_behind_oneway_mode()
{
	_rc_build(OT_ONEWAY)
	local cnv_a = _rc_run(RC_FAR)
	local cnv_b = _rc_run(RC_NEAR)
	ASSERT_EQUAL(cnv_a.get_pos().y, RC_FAR)
	ASSERT_EQUAL(cnv_b.get_pos().y, RC_NEAR)
}

function test_road_choose_stop_behind_twoway_mode()
{
	_rc_build(OT_TWOWAY)
	local cnv_a = _rc_run(RC_FAR)
	local cnv_b = _rc_run(RC_NEAR)
	ASSERT_EQUAL(cnv_a.get_pos().y, RC_FAR)
	ASSERT_EQUAL(cnv_b.get_pos().y, RC_NEAR)
}

function test_road_choose_stop_behind_loading_only_mode()
{
	_rc_build(OT_LOADING_ONLY)
	local cnv_a = _rc_run(RC_FAR)
	local cnv_b = _rc_run(RC_NEAR)
	ASSERT_EQUAL(cnv_a.get_pos().y, RC_FAR)
	ASSERT_EQUAL(cnv_b.get_pos().y, RC_NEAR)
}

function test_road_choose_stop_behind_inverted_mode()
{
	_rc_build(OT_INVERTED)
	local cnv_a = _rc_run(RC_FAR)
	local cnv_b = _rc_run(RC_NEAR)
	ASSERT_EQUAL(cnv_a.get_pos().y, RC_FAR)
	ASSERT_EQUAL(cnv_b.get_pos().y, RC_NEAR)
}

// prohibited_mode: Bus B cannot stop behind Bus A; waits before the choose sign
function test_road_choose_no_stop_behind_prohibited_mode()
{
	_rc_build(OT_PROHIBITED)
	local cnv_a = _rc_run(RC_FAR)
	local cnv_b = _rc_run(1)
	ASSERT_EQUAL(cnv_a.get_pos().y, RC_FAR)
	ASSERT_EQUAL(cnv_b.get_pos().y, 1) // before the choose sign
}
