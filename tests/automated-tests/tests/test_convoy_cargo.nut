//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for convoy_x::get_cargo() and good_x
//

function create_convoy_cargo_schedule(waytype, stop_positions)
{
	return schedule_x(waytype, stop_positions.map(@(pos) schedule_entry_x(pos, 0, 0)))
}


function test_convoy_cargo_empty()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 3, 0), "cobblestone_road"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 3, 0), building_desc_x("CarDepot")), null)

	local depot = depot_x(4, 3, 0)
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("Buessig"))
	local cnv = depot.get_convoy_list()[0]

	local cargo = cnv.get_cargo()
	ASSERT_EQUAL(typeof cargo, "array")
	ASSERT_EQUAL(cargo.len(), 1)
	ASSERT_EQUAL(typeof cargo[0], "array")
	ASSERT_EQUAL(cargo[0].len(), 0)

	// clean up
	cnv.destroy(pl)
	sleep()
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 3, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(player_x(0), coord3d(4, 2, 0), coord3d(4, 3, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_convoy_cargo_loaded()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "cobblestone_road"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "BusStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 7, 0), "BusStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("CarDepot")), null)

	local from_halt = halt_x.get_halt(coord3d(4, 7, 0), pl)
	local to_halt   = halt_x.get_halt(coord3d(4, 2, 0), pl)
	local depot = depot_x(4, 8, 0)

	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("Buessig"))
	local cnv = depot.get_convoy_list()[0]
	cnv.change_schedule(pl, create_convoy_cargo_schedule(wt_road, [ coord3d(4, 7, 0), coord3d(4, 2, 0) ]))
	depot.start_all_convoys(pl)

	sleep()
	sleep()

	ASSERT_EQUAL(world.generate_goods(coord(3, 7), coord(3, 2), good_desc_x.passenger, 30), 1)

	// wait for convoy to arrive at from_halt and pick up passengers
	while (from_halt.convoys[0] == 0) { sleep() }
	while (from_halt.waiting[0] > 0)  { sleep() }

	// cargo is loaded: check structure without advancing simulation
	local cargo = cnv.get_cargo()
	ASSERT_EQUAL(typeof cargo, "array")
	ASSERT_TRUE(cargo.len() >= 1)

	local v0_cargo = cargo[0]
	ASSERT_EQUAL(typeof v0_cargo, "array")
	ASSERT_TRUE(v0_cargo.len() >= 1)

	local good = v0_cargo[0]
	ASSERT_TRUE(good != null)

	// get_desc
	local desc = good.get_desc()
	ASSERT_TRUE(desc != null)
	ASSERT_EQUAL(desc.get_name(), good_desc_x.passenger.get_name())

	// get_destination: may be null (no halt destination for pax in this setup) or valid
	local dest = good.get_destination()
	ASSERT_TRUE(dest == null || dest.is_valid())

	// get_destination_pos: always a coord
	local dest_pos = good.get_destination_pos()
	ASSERT_EQUAL(typeof dest_pos, "coord")

	// get_transit_halts: always an array
	local transit = good.get_transit_halts()
	ASSERT_EQUAL(typeof transit, "array")

	// clean up
	cnv.destroy(pl)
	sleep()
	sleep()
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 7, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 8, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(player_x(0), coord3d(4, 2, 0), coord3d(4, 8, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}
