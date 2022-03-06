class amphibious_connection_planner_t extends industry_connection_planner_t
{
	constructor(s,d,f)
	{
		base.constructor(s,d,f);
		name = "amphibious_connection_planner_t"
	}

	// print messages box
	// 1 =
	// 2 =
	// 3 =
	print_message_box_x = 0

	function step()
	{

		local wt = null

		print("amphibious_connection_planner_t " + this)
		if ( print_message_box_x > 1 ) {
			gui.add_message_at(our_player, " ** amphibious_connection_planner_t " + this, world.get_time())
		}
		// compute monthly production
		if (prod < 0) {
			prod = calc_production()
		}
		dbgprint("production = " + prod);

		// road - initial
		local rprt_road = plan_simple_connection(wt_road, null, null)
		if (rprt_road == null) {
			//return failed()
		}
		// rail - initial
		local rprt_rail = plan_simple_connection(wt_rail, null, null)
		if (rprt_rail == null) {
			//return failed()
		}
		// water - initial
		local rprt_water = plan_simple_connection(wt_water, null, null)

		if (rprt_water == null && ( rprt_road == null || rprt_rail == null )) {
			return failed()
		}

		if ( print_message_box_x == 1 ) {
			gui.add_message_at(our_player, " **** rprt_water - gain_per_m : " + rprt_water.gain_per_m + " # distance : " + rprt_water.distance, world.get_time())
			gui.add_message_at(our_player, " **** rprt_rail - gain_per_m : " + rprt_rail.gain_per_m + " # distance : " + rprt_rail.distance, world.get_time())
			gui.add_message_at(our_player, " **** rprt_road - gain_per_m : " + rprt_road.gain_per_m + " # distance : " + rprt_road.distance, world.get_time())
		}

		if (rprt_water == null ) {
			return failed()
			//rprt_water = report_t()
		}
		// find flat harbour building
		local station_list = building_desc_x.get_available_stations(building_desc_x.flat_harbour, wt_water, good_desc_x(freight))
		rprt_water.action.planned_harbour_flat = industry_connection_planner_t.select_station(station_list, 1, rprt_water.action.planned_convoy.capacity)

		// find amphibious path
		local marine = null
		if ( rprt_rail != null && ( rprt_road==null ||  rprt_rail.gain_per_m > rprt_road.gain_per_m ) ) {
			marine = amphibious_pathfinder_t(rprt_rail.action.planned_way, rprt_water.action.planned_station, rprt_water.action.planned_harbour_flat)
			wt = wt_rail
			if ( print_message_box_x == 1 ) {
				gui.add_message_at(our_player, " ---> rprt_rail  ", world.get_time())
			}
		} else if ( rprt_road != null ) {
			marine = amphibious_pathfinder_t(rprt_road.action.planned_way, rprt_water.action.planned_station, rprt_water.action.planned_harbour_flat)
			wt = wt_road
			if ( print_message_box_x == 1 ) {
				gui.add_message_at(our_player, " ---> rprt_road  ", world.get_time())
			}
		}

		if (marine == null) {
			return r_t(RT_TOTAL_FAIL)
		}

		// check build cost
		if ( wt == wt_rail && rprt_rail.cost_fix > our_player.get_current_cash() ) {
			return r_t(RT_TOTAL_FAIL)
		} else if ( wt == wt_road && rprt_road.cost_fix > our_player.get_current_cash() ) {
			return r_t(RT_TOTAL_FAIL)
		}

		marine.search_route(fsrc,fdest)

		local route = marine.route
		if (route.len() == 0) {
			return r_t(RT_TOTAL_FAIL)
		}
		// generate report
		local report = report_t()
		report.action = node_seq_t()

		if ( print_message_box_x == 1 ) {
			gui.add_message_at(our_player, " ---> marine.route.len(fsrc,fdest)  " + route.len(), world.get_time())
			gui.add_message_at(our_player, " ---> route[0] " + coord3d_to_string(route[0]) + " - route[route.len()-1] " + coord3d_to_string(route[route.len()-1]), world.get_time())
		}

		// now loop through route backwards
		local i = route.len()-1;
		local from = tile_x(route[i].x, route[i].y, route[i].z)
		local on_water = ::finder._tile_water_way(from)
		local from_i = on_water ? i : i+1;
		i--
		for(; i>=0; i--) {
			local to = tile_x(route[i].x, route[i].y, route[i].z)
			local to_water = ::finder._tile_water_way(to)
			local change = to_water != on_water

			if (change || i==0) {
				print("-- Plan arc from " + coord_to_string(from) + " to " + coord_to_string(route[i]))
				if ( print_message_box_x == 1 ) {
					gui.add_message_at(our_player, " **-- Plan arc from " + coord_to_string(from) + " to " + coord_to_string(route[i]), world.get_time())
				}
				// change between land and sea
				local r = null
				if (on_water) {
					// from_i = first water tile
					// i      = first land tile (or i==0)
					// plan between water tiles
					r = plan_simple_connection(wt_water,
							from_i < route.len()-1 ? from : null,
							change ? route[i+1] : null, from_i - (i+1))
					// set harbour positions
					if (r) {
						local shipc = r.action
						if (from_i < route.len()-1) {
							local c = route[from_i+1]
							shipc.c_harbour_tiles[ coord3d_to_key(from) ] <- tile_x(c.x, c.y, c.z)
						}
						if (change) {
							shipc.c_harbour_tiles[ coord3d_to_key(route[i+1]) ] <- to
						}
					}
				}
				else {
					// from_i = first land tile, which is harbour slope
					// i      = first water tile
					print("Try to catch index error")
					if (i+2 < route.len()) {
						r = plan_simple_connection(wt, route[from_i-1], change ? route[i+2] : null, from_i-1 - (i+2))
					}
					else {
						// else: first tile of route but second already in water - no need to plan road
						r = report_t()
					}
				}
				if (r) {
					if (r.action) {
						r.action.finalize = !change
					}
					report.merge_report(r)
				}
				else {
					return failed()
				}

				from = to
				from_i = i
				on_water = to_water
			}
		}


		return r_t(RT_TOTAL_SUCCESS, report)
	}

	function failed()
	{
		//industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_missing)
		return r_t(RT_TOTAL_FAIL)
	}
}

class amphibious_pathfinder_t extends astar
{
	way = 0
	builder = null
	planned_harbour = null
	planned_harbour_flat = null
	planned_harbour_len = 0
	planned_harbour_flat_len = 0

	cost_harbour = 333
	cost_road_stop = 17

	c_harbour_tiles = null

	// print messages box
	// 1
	print_message_box_x = 0

	constructor(way_, harbour_, harbour_flat_)
	{
		base.constructor()

		way = way_
		builder = way_planner_x(our_player)
		builder.set_build_types(way)
		planned_harbour = harbour_
		planned_harbour_flat = harbour_flat_

		local size = planned_harbour.get_size(0)
		planned_harbour_len = size.x*size.y

		if (planned_harbour_flat) {
			local size = planned_harbour_flat.get_size(0)
			planned_harbour_flat_len = size.x*size.y
		}
	}

	function process_node(cnode)
	{
		local from = tile_x(cnode.x, cnode.y, cnode.z)
		local back = dir.backward(cnode.dir)
		local from_water = ::finder._tile_water_way(from)
		local water_dir = from.get_way_dirs(wt_water)

		//
		local message = [0, 0, 0]

		// flags
		// 0x0f -> jps
		// 0x10 -> find flat place
		// 0x20 -> flat place (mark next step with 0x40)
		// 0x40 -> cannot go into water
		if (cnode.flag == 0x10) {
			process_node_to_land(cnode, from)
			return
		}

		local test_dir = cnode.previous ? (back ^ 0x0f) & (cnode.flag & 0x0f) : 0x0f

		for(local d = 1; d<16; d*=2) {
			// do not go backwards
			if (( d &test_dir) == 0 ) {
				continue
			}

			local to = from.get_neighbour(wt_all, d)

			if (to  &&  !is_closed(to)) {
				local to_water = ::finder._tile_water_way(to)

				// water -> water
				// land -> land
				if (from_water ==  to_water) {
					// can we build way here?
					if (!from_water) {
						// empty space for station near a halt with harbour
						local halt = to.get_halt()
						if (halt  &&  halt.get_owner().nr == our_player_nr  &&  from.is_empty()) {
							// try to connect to this halt and to use its harbours
							process_node_to_land_halt(cnode, halt)
						}
						// can we build way here?
						if (!builder.is_allowed_step(from, to)) {
							// could be a halt on the other tile that has harbour that could be used
							continue;
						}
					}
					// estimate moving cost
					local move   = cnode.is_straight_move(d)  ?  cost_straight  :  cost_curve
					local dist   = estimate_distance(to)

					local cost   = cnode.cost + move
					local weight = cost + dist
					// use jump-point search (see dataobj/route.cc)
					local jps = to_water  &&  (cnode.previous) ? (water_dir ^ 0x0f) | d | cnode.dir | from.get_canal_ribi() : 0x0f

					if (cnode.flag & 0x20) jps = jps | 0x40;

					local node = ab_node(to, cnode, cost, dist, d, jps)
					add_to_open(node, weight)

					message[0] = 1
				}
				else if (from_water) {
					// water -> land
					if (from.is_water()  &&  to.get_halt()) {
						// check for existing harbour
						local halt = ship_connector_t.get_harbour_halt(to)
						if (halt) {
							// create node as end node of water route
							local harbour_node = ab_node(to, cnode, cnode.cost, cnode.dist, 0x0f, 0x0f)
							// now look for empty spots to connect to this halt
							process_node_to_harbour(harbour_node, halt)
						}
					}
					// we have to build ourselves
					if (!from.is_water()  ||  !can_place_harbour(to, d) )
					{
						continue
					}
					local move   = cost_harbour
					local dist   = estimate_distance(to)
					local cost   = cnode.cost + move
					local weight = cost + dist

					local node = ab_node(to, cnode, cost, dist, d, 0x10)

					add_to_open(node, weight)

					message[1] = 1
				}
				else {
					// land -> water
					if (!to.is_water()
						||  !can_place_harbour(from, dir.backward(d))
						||  (cnode.flag & 0x60) )
					{
						continue
					}
					if (cnode.previous) {
						local fromfrom = tile_x(cnode.previous.x, cnode.previous.y, cnode.previous.z)
						// want to build station here with one connecting road
						if (fromfrom == null  ||  !fromfrom.is_empty()  ||  fromfrom.get_slope()!=0  ||  cnode.previous.previous == null) {
							continue
						}
					}

					local move   = cost_harbour
					local dist   = estimate_distance(to)
					local cost   = cnode.cost + move
					local weight = cost + dist

					local node = ab_node(to, cnode, cost, dist, d, 0x0f)
					add_to_open(node, weight)

					message[2] = 1
				}
			}
    }
		/*
		if ( print_message_box_x == 1 ) {
			if ( message[0] == 1 ) {
							gui.add_message_at(our_player, "---- water -> water", world.get_time())
							gui.add_message_at(our_player, "---- found", world.get_time())
			}
			if ( message[1] == 1 ) {
							gui.add_message_at(our_player, "---- water -> land", world.get_time())
							gui.add_message_at(our_player, "---- found", world.get_time())
						}
			if ( message[2] == 1 ) {
							gui.add_message_at(our_player, "---- land -> water", world.get_time())
							gui.add_message_at(our_player, "---- found", world.get_time())
			}
		}
	  */
	}

	function can_place_harbour(tile, d_water_to_land)
	{
		return ship_connector_t.get_harbour_type_for_tile(tile, planned_harbour, planned_harbour_flat, d_water_to_land) > 0
	}

	function can_place_harbour(tile, d_water_to_land)
	{
		return ship_connector_t.get_harbour_type_for_tile(tile, planned_harbour, planned_harbour_flat, d_water_to_land) > 0
	}

	function process_node_to_land(cnode, from)
	{
		local pos = coord(cnode.x, cnode.y)

		for(local d = 1; d<16; d++) {
			// test all 8 neighbouring tiles
			if (!dir.is_threeway(d)) {

				local c = pos + dir.to_coord(d)

				try {
					local to = square_x(c.x, c.y).get_ground_tile()
					if (to  &&  to.is_empty()  &&  to.get_slope()==0) {
						// can place station here
						local move   = cost_road_stop
						local dist   = estimate_distance(to)

						local cost   = cnode.cost + move
						local weight = cost + dist

						local node = ab_node(to, cnode, cost, dist, 0x0f, 0x0f)
						add_to_open(node, weight)
					}
				}
				catch(ev) {}
			}
		}
	}

	function process_node_to_harbour(cnode, halt)
	{
		local visited = {}
		// process all tiles of this halt
		foreach(tile in halt.get_tile_list()) {
			// check all neighbouring tiles for empty spots
			for(local d = 1; d<16; d*=2) {
				local c = coord(tile.x, tile.y) + dir.to_coord(d)
				try {
					local to = square_x(c.x, c.y).get_ground_tile()

					if (to  &&  to.is_empty()  &&  to.get_slope()==0  &&  !(coord3d_to_key(to) in visited) ) {
						// can place station here
						local move   = cost_road_stop
						local dist   = estimate_distance(to)

						local cost   = cnode.cost + move
						local weight = cost + dist

						local node = ab_node(to, cnode, cost, dist, 0x0f, 0x0f)
						add_to_open(node, weight)

						// mark as visited
						visited[ coord3d_to_key(to) ] <- 1
					}
				}
				catch(ev) {}
			}
		}
	}

	/**
	 * Does steps land-tile -> halt -> harbour -> water
	 * Last route tile is cnode, check if station can be placed.
	 * Check for harbour tiles, put corresponding nodes to open list.
	 */
	function process_node_to_land_halt(cnode, halt)
	{
		// process all tiles of this halt
		foreach(tile in halt.get_tile_list()) {
			if (!tile.is_water()  &&  ship_connector_t.get_harbour_halt(tile)!=null) {
				local harbour_node = ab_node(tile, cnode, cnode.cost, cnode.dist, 0x0f, 0x0f)
				local d = 0
				if (tile.get_slope()) {
					// get direction from slope
					d = dir.backward(slope.to_dir(tile.get_slope()))
				}
				else {
					// flat dock, check all four neighbour tiles for water + harbour
					d = 1
				}
				do {
					local to = coord3d(tile.x, tile.y, tile.z) + dir.to_coord(d)
					if (world.is_coord_valid(to)) {
						local move   = cost_road_stop
						local dist   = estimate_distance(to)

						local cost   = cnode.cost + move
						local weight = cost + dist

						local node = ab_node(to, harbour_node, cost, dist, d, 0x0f)
						add_to_open(node, weight)
					}

					d = d*2
				} while (tile.get_slope()==0  &&  d<16)
			}
		}
	}

	function search_route(fsrc, fdest)
	{
		print("Search amphibious connection")
		if ( print_message_box_x > 0 ) {
			local fs = fsrc.get_tile_list()
			local fd = fdest.get_tile_list()
			gui.add_message_at(our_player, "____________ Search amphibious connection ___________", world.get_time())
			gui.add_message_at(our_player, " line from " + fsrc.get_name() + " (" + coord_to_string(fs[0]) + ") to " + fdest.get_name() + " (" + coord_to_string(fd[0]) + ")", world.get_time())
		}
		// find station places
		local s = []
		local e = []
		c_harbour_tiles = {}

		s.append(::finder.find_station_place(fsrc, fdest) )
		e.append(::finder.find_station_place(fdest, fsrc) )
		s.append(::ship_connector_t.find_anchorage(fsrc,  planned_harbour, planned_harbour_flat, c_harbour_tiles) )
		e.append(::ship_connector_t.find_anchorage(fdest, planned_harbour, planned_harbour_flat, c_harbour_tiles) )

		// init search
		prepare_search()
		for(local i=0; i<2; i++) {
			foreach (e in e[i]) {
				targets.append(e);
			}
		}

		if (targets.len() == 0) {
			route = []
			return
		}

		compute_bounding_box()

		for(local i=0; i<2; i++) {
			foreach (s in s[i])
			{
				local dist = estimate_distance(s)
				add_to_open(ab_node(s, null, 1, dist+1, dist, 0), dist+1)
			}
		}

		search()

		print("End amphibious route search")
		if ( print_message_box_x > 0 ) {
			gui.add_message_at(our_player, "------------ End amphibious route search -------------", world.get_time())
		}
	}
}
