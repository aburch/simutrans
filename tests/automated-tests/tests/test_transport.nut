//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


function create_simple_schedule(waytype, stop_positions)
{
	return schedule_x(waytype, stop_positions.map(@(pos) schedule_entry_x(pos, 0, 0)))
}

//
// Tests for transporting passengers, mail and goods
//

function test_transport_generate_pax_invalid_pos()
{
	ASSERT_EQUAL(world.generate_goods(coord(-1, -1), coord(-1, -1), good_desc_x.passenger, 42), 0)
	ASSERT_EQUAL(world.generate_goods(coord( 0,  0), coord(-1, -1), good_desc_x.passenger, 42), 0)
	ASSERT_EQUAL(world.generate_goods(coord(-1, -1), coord( 0,  0), good_desc_x.passenger, 42), 0)
}


function test_transport_generate_pax_walked()
{
	ASSERT_EQUAL(command_x(tool_build_way).work(player_x(0), coord3d(4, 2, 0), coord3d(4, 3, 0), "cobblestone_road"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(player_x(0), coord3d(4, 2, 0), "BusStop"), null)

	{
		ASSERT_EQUAL(world.generate_goods(coord(3, 2), coord(5, 2), good_desc_x.passenger, 42), 2) // 2 == walked

		ASSERT_EQUAL(halt_x.get_halt(coord3d(4, 2, 0), player_x(0)).get_walked()[0], 42)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(player_x(0), coord3d(4, 2, 0), coord3d(4, 3, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_transport_generate_pax_no_route()
{
	ASSERT_EQUAL(command_x(tool_build_way).work(player_x(0), coord3d(4, 2, 0), coord3d(4, 7, 0), "cobblestone_road"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(player_x(0), coord3d(4, 2, 0), "BusStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(player_x(0), coord3d(4, 7, 0), "BusStop"), null)

	{
		ASSERT_EQUAL(world.generate_goods(coord(3, 2), coord(3, 7), good_desc_x.passenger, 42), 0)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 7, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(player_x(0), coord3d(4, 2, 0), coord3d(4, 7, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_transport_pax_valid_route()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "cobblestone_road"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "BusStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 7, 0), "BusStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("CarDepot")), null)

	local depot     = depot_x(4, 8, 0)
	local from_halt = halt_x.get_halt(coord3d(4, 7, 0), pl)
	local to_halt   = halt_x.get_halt(coord3d(4, 2, 0), pl)

	// create vehicle ...
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("Buessig"))
	local cnv = depot.get_convoy_list()[0]
	// ... with simple 2-entry schedule ...
	cnv.change_schedule(pl, create_simple_schedule(wt_road, [ coord3d(4, 7, 0), coord3d(4, 2, 0) ]))
	// and start the convoy
	depot.start_all_convoys(pl)

	// make sure that the graph linking all halts is updated
	sleep()
	sleep()

	{
		ASSERT_EQUAL(world.generate_goods(coord(3, 7), coord(3, 2), good_desc_x.passenger, 30), 1) // 1 == OK

		ASSERT_EQUAL(from_halt.waiting[0], 30)
		ASSERT_EQUAL(from_halt.get_freight_to_halt(good_desc_x.passenger, to_halt), 30)
		ASSERT_EQUAL(from_halt.get_freight_to_dest(good_desc_x.passenger, coord3d(3, 2, 0)), 30)
		ASSERT_EQUAL(from_halt.get_freight_to_dest(good_desc_x.passenger, coord3d(4, 2, 0)), 0)
		ASSERT_EQUAL(from_halt.happy[0], 30)
		ASSERT_EQUAL(from_halt.unhappy[0], 0)
		ASSERT_EQUAL(from_halt.noroute[0], 0)
		ASSERT_EQUAL(from_halt.walked[0], 0)
		ASSERT_EQUAL(from_halt.departed[0], 0)
		ASSERT_EQUAL(from_halt.arrived[0], 0)
		ASSERT_EQUAL(from_halt.convoys[0], 0)
	}

	while (from_halt.convoys[0] == 0) {
		sleep()
	}

	// Loading is not instant, so we need to make sure loading has finished
	while (from_halt.waiting[0] > 0) {
		sleep()
	}

	{
		ASSERT_EQUAL(from_halt.convoys[0], 1)
		ASSERT_EQUAL(from_halt.waiting[0], 0)
		ASSERT_EQUAL(from_halt.get_freight_to_halt(good_desc_x.passenger, to_halt), 0)
		ASSERT_EQUAL(from_halt.get_freight_to_dest(good_desc_x.passenger, coord3d(3, 2, 0)), 0)
		ASSERT_EQUAL(from_halt.get_freight_to_dest(good_desc_x.passenger, coord3d(4, 2, 0)), 0)
		ASSERT_EQUAL(from_halt.happy[0], 30)
		ASSERT_EQUAL(from_halt.unhappy[0], 0)
		ASSERT_EQUAL(from_halt.noroute[0], 0)
		ASSERT_EQUAL(from_halt.walked[0], 0)
		ASSERT_EQUAL(from_halt.departed[0], 30)
		ASSERT_EQUAL(from_halt.arrived[0], 0)
	}

	while (to_halt.convoys[0] == 0) {
		sleep()
	}

	// make sure unloading has finished
	while (to_halt.arrived[0] < 30) {
		sleep()
	}

	{
		ASSERT_EQUAL(to_halt.waiting[0], 0)
		ASSERT_EQUAL(to_halt.happy[0], 0)
		ASSERT_EQUAL(to_halt.unhappy[0], 0)
		ASSERT_EQUAL(to_halt.noroute[0], 0)
		ASSERT_EQUAL(to_halt.walked[0], 0)
		ASSERT_EQUAL(to_halt.departed[0], 0)
		ASSERT_EQUAL(to_halt.arrived[0], 30)
	}

	// clean up
	cnv.destroy(pl)
	sleep()
	sleep() // make sure the convoy is destroyed
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 7, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 8, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(player_x(0), coord3d(4, 2, 0), coord3d(4, 8, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_transport_mail_valid_route()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "cobblestone_road"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "PostStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 7, 0), "PostStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("CarDepot")), null)

	local depot = depot_x(4, 8, 0)
	local from_halt = halt_x.get_halt(coord3d(4, 7, 0), pl)
	local to_halt   = halt_x.get_halt(coord3d(4, 2, 0), pl)

	// create vehicle ...
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("Posttransporter"))
	local cnv = depot.get_convoy_list()[0]
	// ... with simple 2-entry schedule ...
	cnv.change_schedule(pl, create_simple_schedule(wt_road, [ coord3d(4, 7, 0), coord3d(4, 2, 0) ]))
	// and start the convoy
	depot.start_all_convoys(pl)

	// make sure that the graph linking all halts is updated
	sleep()
	sleep()

	{
		ASSERT_EQUAL(world.generate_goods(coord(3, 7), coord(3, 2), good_desc_x.mail, 30), 1) // 1 == OK
	}

	sleep()
	sleep()

	{
		ASSERT_EQUAL(from_halt.waiting[0], 30)
		ASSERT_EQUAL(from_halt.get_freight_to_halt(good_desc_x.mail, to_halt), 30)
		ASSERT_EQUAL(from_halt.get_freight_to_dest(good_desc_x.mail, coord3d(3, 2, 0)), 30)
		ASSERT_EQUAL(from_halt.get_freight_to_dest(good_desc_x.mail, coord3d(4, 2, 0)), 0)
		ASSERT_EQUAL(from_halt.happy[0], 0)
		ASSERT_EQUAL(from_halt.unhappy[0], 0)
		ASSERT_EQUAL(from_halt.noroute[0], 0)
		ASSERT_EQUAL(from_halt.walked[0], 0)
		ASSERT_EQUAL(from_halt.departed[0], 0)
		ASSERT_EQUAL(from_halt.arrived[0], 0)
		ASSERT_EQUAL(from_halt.convoys[0], 0)
	}

	while (from_halt.convoys[0] == 0) {
		sleep()
	}

	// Loading is not instant, so we need to make sure loading has finished
	while (from_halt.waiting[0] > 0) {
		sleep()
	}

	{
		ASSERT_EQUAL(from_halt.convoys[0], 1)
		ASSERT_EQUAL(from_halt.waiting[0], 0)
		ASSERT_EQUAL(from_halt.get_freight_to_halt(good_desc_x.mail, to_halt), 0)
		ASSERT_EQUAL(from_halt.get_freight_to_dest(good_desc_x.mail, coord3d(3, 2, 0)), 0)
		ASSERT_EQUAL(from_halt.get_freight_to_dest(good_desc_x.mail, coord3d(4, 2, 0)), 0)
		ASSERT_EQUAL(from_halt.happy[0], 0)
		ASSERT_EQUAL(from_halt.unhappy[0], 0)
		ASSERT_EQUAL(from_halt.noroute[0], 0)
		ASSERT_EQUAL(from_halt.walked[0], 0)
		ASSERT_EQUAL(from_halt.departed[0], 30)
		ASSERT_EQUAL(from_halt.arrived[0], 0)
	}

	while (to_halt.convoys[0] == 0) {
		sleep()
	}

	// make sure unloading has finished
	while (to_halt.arrived[0] < 30) {
		sleep()
	}

	{
		ASSERT_EQUAL(to_halt.waiting[0], 0)
		ASSERT_EQUAL(to_halt.happy[0], 0)
		ASSERT_EQUAL(to_halt.unhappy[0], 0)
		ASSERT_EQUAL(to_halt.noroute[0], 0)
		ASSERT_EQUAL(to_halt.walked[0], 0)
		ASSERT_EQUAL(to_halt.departed[0], 0)
		ASSERT_EQUAL(to_halt.arrived[0], 30)
	}

	// clean up
	cnv.destroy(pl)
	sleep()
	sleep() // make sure the convoy is destroyed
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 7, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 8, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(player_x(0), coord3d(4, 2, 0), coord3d(4, 8, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_transport_freight_valid_route()
{
	local pl = player_x(0)

	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 8, 0), "cobblestone_road"), null)
	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 7, 0), coord3d(5, 7, 0), "cobblestone_road"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "CarStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(5, 7, 0), "CarStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 8, 0), building_desc_x("CarDepot")), null)

	local depot = depot_x(4, 8, 0)
	local from_halt = halt_x.get_halt(coord3d(5, 7, 0), pl)
	local to_halt   = halt_x.get_halt(coord3d(4, 2, 0), pl)

	// create vehicle ...
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("Kohletransporter"))
	local cnv = depot.get_convoy_list()[0]
	// ... with simple 2-entry schedule ...
	cnv.change_schedule(pl, create_simple_schedule(wt_road, [ coord3d(5, 7, 0), coord3d(4, 2, 0) ]))
	// and start the convoy
	depot.start_all_convoys(pl)

	// make sure that the graph linking all halts is updated
	sleep()
	sleep()

	{
		ASSERT_EQUAL(world.generate_goods(coord(3, 7), coord(3, 2), good_desc_x("Kohle"), 18), 1) // 1 == OK
	}

	sleep()
	sleep()

	{
		ASSERT_EQUAL(from_halt.waiting[0], 18)
		ASSERT_EQUAL(from_halt.get_freight_to_halt(good_desc_x("Kohle"), to_halt), 18)
		ASSERT_EQUAL(from_halt.get_freight_to_dest(good_desc_x("Kohle"), coord3d(3, 2, 0)), 18)
		ASSERT_EQUAL(from_halt.get_freight_to_dest(good_desc_x("Kohle"), coord3d(4, 2, 0)), 0)
		ASSERT_EQUAL(from_halt.happy[0], 0)
		ASSERT_EQUAL(from_halt.unhappy[0], 0)
		ASSERT_EQUAL(from_halt.noroute[0], 0)
		ASSERT_EQUAL(from_halt.walked[0], 0)
		ASSERT_EQUAL(from_halt.departed[0], 0)
		ASSERT_EQUAL(from_halt.arrived[0], 0)
		ASSERT_EQUAL(from_halt.convoys[0], 0)
	}

	while (from_halt.convoys[0] == 0) {
		sleep()
	}

	// Loading is not instant, so we need to make sure loading has finished
	while (from_halt.waiting[0] > 0) {
		sleep()
	}

	{
		ASSERT_EQUAL(from_halt.convoys[0], 1)
		ASSERT_EQUAL(from_halt.waiting[0], 0)
		ASSERT_EQUAL(from_halt.get_freight_to_halt(good_desc_x("Kohle"), to_halt), 0)
		ASSERT_EQUAL(from_halt.get_freight_to_dest(good_desc_x("Kohle"), coord3d(3, 2, 0)), 0)
		ASSERT_EQUAL(from_halt.get_freight_to_dest(good_desc_x("Kohle"), coord3d(4, 2, 0)), 0)
		ASSERT_EQUAL(from_halt.happy[0], 0)
		ASSERT_EQUAL(from_halt.unhappy[0], 0)
		ASSERT_EQUAL(from_halt.noroute[0], 0)
		ASSERT_EQUAL(from_halt.walked[0], 0)
		ASSERT_EQUAL(from_halt.departed[0], 18)
		ASSERT_EQUAL(from_halt.arrived[0], 0)
	}

	while (to_halt.convoys[0] == 0) {
		sleep()
	}

	// make sure unloading has finished
	while (to_halt.arrived[0] < 18) {
		sleep()
	}

	{
		ASSERT_EQUAL(to_halt.waiting[0], 0)
		ASSERT_EQUAL(to_halt.happy[0], 0)
		ASSERT_EQUAL(to_halt.unhappy[0], 0)
		ASSERT_EQUAL(to_halt.noroute[0], 0)
		ASSERT_EQUAL(to_halt.walked[0], 0)
		ASSERT_EQUAL(to_halt.departed[0], 0)
		ASSERT_EQUAL(to_halt.arrived[0], 18)
	}

	// clean up
	cnv.destroy(pl)
	sleep()
	sleep() // make sure the convoy is destroyed
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(5, 7, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 8, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(player_x(0), coord3d(4, 2, 0), coord3d(4, 8, 0), "" + wt_road), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(player_x(0), coord3d(5, 7, 0), coord3d(4, 7, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_transport_pax_no_load()
{
	local pl = player_x(0)
	local NO_LOAD = 4

	// Speed up simulation
	debug.set_game_speed(5)

	// Build road A(4,2) -- B(4,5) -- C(4,8) -- depot(4,9)
	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 9, 0), "cobblestone_road"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "BusStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 5, 0), "BusStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 8, 0), "BusStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 9, 0), building_desc_x("CarDepot")), null)

	local depot  = depot_x(4, 9, 0)
	local halt_a = halt_x.get_halt(coord3d(4, 2, 0), pl)
	local halt_b = halt_x.get_halt(coord3d(4, 5, 0), pl)
	local halt_c = halt_x.get_halt(coord3d(4, 8, 0), pl)

	// Create schedule: A -> B(NO_LOAD) -> C
	local sched = schedule_x(wt_road, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),           // A: normal
		schedule_entry_x(coord3d(4, 5, 0), 0, 0, NO_LOAD),  // B: no loading
		schedule_entry_x(coord3d(4, 8, 0), 0, 0)            // C: normal
	])

	// Create and start bus
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("Buessig"))
	local cnv = depot.get_convoy_list()[0]
	cnv.change_schedule(pl, sched)
	depot.start_all_convoys(pl)

	// Wait for halt graph update
	sleep()
	sleep()

	// Generate passengers A -> C: should succeed (route exists via A)
	ASSERT_EQUAL(world.generate_goods(coord(3, 2), coord(3, 8), good_desc_x.passenger, 30), 1)
	ASSERT_EQUAL(halt_a.waiting[0], 30)

	// Generate passengers B -> C: should fail (no route, B has NO_LOAD)
	ASSERT_EQUAL(world.generate_goods(coord(3, 5), coord(3, 8), good_desc_x.passenger, 20), 0)

	// Wait for convoy to visit A and load
	while (halt_a.convoys[0] == 0) {
		sleep()
	}
	while (halt_a.waiting[0] > 0) {
		sleep()
	}

	ASSERT_EQUAL(halt_a.departed[0], 30)

	// Wait for convoy to arrive at C and unload
	while (halt_c.arrived[0] < 30) {
		sleep()
	}

	ASSERT_EQUAL(halt_c.arrived[0], 30)
	// B should have had no departures (nothing loaded there)
	ASSERT_EQUAL(halt_b.departed[0], 0)

	// clean up
	debug.set_game_speed(1)
	cnv.destroy(pl)
	sleep()
	sleep()
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 5, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 8, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 9, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 2, 0), coord3d(4, 9, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_transport_pax_no_unload()
{
	local pl = player_x(0)
	local NO_UNLOAD = 8

	// Speed up simulation
	debug.set_game_speed(5)

	// Build road A(4,2) -- B(4,5) -- C(4,8) -- depot(4,9)
	ASSERT_EQUAL(command_x(tool_build_way).work(pl, coord3d(4, 2, 0), coord3d(4, 9, 0), "cobblestone_road"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 2, 0), "BusStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 5, 0), "BusStop"), null)
	ASSERT_EQUAL(command_x(tool_build_station).work(pl, coord3d(4, 8, 0), "BusStop"), null)
	ASSERT_EQUAL(command_x.build_depot(pl, coord3d(4, 9, 0), building_desc_x("CarDepot")), null)

	local depot  = depot_x(4, 9, 0)
	local halt_a = halt_x.get_halt(coord3d(4, 2, 0), pl)
	local halt_b = halt_x.get_halt(coord3d(4, 5, 0), pl)
	local halt_c = halt_x.get_halt(coord3d(4, 8, 0), pl)

	// Create schedule: A -> B(NO_UNLOAD) -> C
	local sched = schedule_x(wt_road, [
		schedule_entry_x(coord3d(4, 2, 0), 0, 0),             // A: normal
		schedule_entry_x(coord3d(4, 5, 0), 0, 0, NO_UNLOAD),  // B: no unloading
		schedule_entry_x(coord3d(4, 8, 0), 0, 0)              // C: normal
	])

	// Create and start bus
	depot.append_vehicle(pl, convoy_x(0), vehicle_desc_x("Buessig"))
	local cnv = depot.get_convoy_list()[0]
	cnv.change_schedule(pl, sched)
	depot.start_all_convoys(pl)

	// Wait for halt graph update
	sleep()
	sleep()

	// Passengers A -> B: should fail (no route, B has NO_UNLOAD)
	ASSERT_EQUAL(world.generate_goods(coord(3, 2), coord(3, 5), good_desc_x.passenger, 20), 0)

	// Passengers A -> C: should succeed (C is normal, route exists)
	ASSERT_EQUAL(world.generate_goods(coord(3, 2), coord(3, 8), good_desc_x.passenger, 30), 1)
	ASSERT_EQUAL(halt_a.waiting[0], 30)

	// Wait for convoy to visit A, load, then reach C
	while (halt_a.convoys[0] == 0) {
		sleep()
	}
	while (halt_a.waiting[0] > 0) {
		sleep()
	}

	ASSERT_EQUAL(halt_a.departed[0], 30)

	// Wait for arrival at C
	while (halt_c.arrived[0] < 30) {
		sleep()
	}

	ASSERT_EQUAL(halt_c.arrived[0], 30)
	// B should have no arrivals (nothing was unloaded there)
	ASSERT_EQUAL(halt_b.arrived[0], 0)

	// clean up
	debug.set_game_speed(1)
	cnv.destroy(pl)
	sleep()
	sleep()
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 5, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 8, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(4, 9, 0)), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 2, 0), coord3d(4, 9, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}
