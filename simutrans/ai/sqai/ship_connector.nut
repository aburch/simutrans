class ship_connector_t extends manager_t
{
	// input data
	fsrc = null
	fdest = null
	freight = null
	planned_station = null // harbour on slope
	planned_harbour_flat = null
	planned_convoy = null
	planned_way = null // unused
	planned_depot = null
	finalize = true

	// step-by-step construct the connection
	phase = 0
	// can be provided optionally
	c_start = null // array, must be water tiles
	c_end   = null // array, must be water tiles
	c_harbour_tiles = null
	// generated data
	c_depot = null
	c_sched = null
	c_line  = null
	c_cnv   = null

	constructor()
	{
		base.constructor("ship_connector_t")
		c_harbour_tiles = {}
		debug = true
	}

	function work()
	{
		// TODO check if child does the right thing
		local pl = our_player
		local tic = get_ops_total();

		switch(phase) {

			case 0: {
				// find flat harbour building
				local station_list = building_desc_x.get_available_stations(building_desc_x.flat_harbour, wt_water, good_desc_x(freight))
				planned_harbour_flat = industry_connection_planner_t.select_station(station_list, 1, planned_convoy.capacity)
				phase ++
			}
			case 1:
				// find empty water tiles
				if (c_start == null) {
					c_start = find_anchorage(fsrc,  planned_station, planned_harbour_flat, c_harbour_tiles)
				}
				if (c_end == null) {
					c_end   = find_anchorage(fdest, planned_station, planned_harbour_flat, c_harbour_tiles)
				}

				if (c_start.len()>0  &&  c_end.len()>0) {
					phase ++
				}
				else {
					print("No station places found")
					return error_handler()
				}

			case 2: // find path between both factories
				{
					local err = find_route()
					if (err) {
						print("No way from " + coord_to_string(c_start[0])+ " to " + coord_to_string(c_end[0]))
						return error_handler()
					}
					phase ++
				}
			case 3: // build harbour
				{
					local key
					local err = null
					{
						key = coord3d_to_key(c_start[0])
						if (key in c_harbour_tiles) {
							err = build_harbour(c_harbour_tiles[key], c_start)
						}
					}
					if (err == null) {
						key = coord3d_to_key(c_end[0])
						if (key in c_harbour_tiles) {
							err = build_harbour(c_harbour_tiles[key], c_end)
						}
					}
					if (err) {
						print("Failed to build harbour at " + key + " / " + err)
						return error_handler()
					}

					c_harbour_tiles = null
					phase ++
				}
			case 4: // find route again after harbour was built
				{
					if (c_start.len()>1  ||  c_end.len()>1) {
						local err = find_route()
						if (err) {
							print("No way2 from " + coord_to_string(c_start[0])+ " to " + coord_to_string(c_end[0]))
							return error_handler()
						}
					}

					phase ++
				}
			case 5: // build depot
				{
					// depot already existing ?
					if (c_depot.find_object(mo_depot_water) == null) {
						// no: build
						local err = command_x.build_depot(pl, c_depot, planned_depot )
						if (err) {
							print("Failed to build depot at " + coord_to_string(c_depot))
							return error_handler()
						}
						if (finalize) {
							// store depot location
							local fs = ::station_manager.access_freight_station(fsrc)
							if (fs.ship_depot == null) {
								fs.ship_depot = c_depot
							}
						}
					}
					phase ++
				}
			case 6: // create schedule
				{
					local sched = schedule_x(wt_water, [])
					sched.entries.append( schedule_entry_x(c_start[0], 100, 0) );
					sched.entries.append( schedule_entry_x(c_end[0], 0, 0) );
					c_sched = sched
					phase ++
				}

			case 7: // create line and set schedule
				{
					pl.create_line(wt_water)

					// find the line - it is a line without schedule and convoys
					local list = pl.get_line_list()
					foreach(line in list) {
						if (line.get_waytype() == wt_water  &&  line.get_schedule().entries.len()==0) {
							// right type, no schedule -> take this.
							c_line = line
							break
						}
					}
					// set schedule
					c_line.change_schedule(pl, c_sched);
					phase ++
				}
			case 8: // append vehicle_constructor
				{
					local c = vehicle_constructor_t()
					c.p_depot  = depot_x(c_depot.x, c_depot.y, c_depot.z)
					c.p_line   = c_line
					c.p_convoy = planned_convoy
					c.p_count  = min(planned_convoy.nr_convoys, 3)
					append_child(c)

					local toc = get_ops_total();
					print("ship_connector wasted " + (toc-tic) + " ops")

					phase ++
					return r_t(RT_PARTIAL_SUCCESS)
				}
			case 9: // build station extension

		}

		if (finalize) {
			industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_built)
		}
		industry_manager.access_link(fsrc, fdest, freight).append_line(c_line)

		return r_t(RT_TOTAL_SUCCESS)
	}

	function error_handler()
	{
		local r = r_t(RT_TOTAL_FAIL)
		// TODO rollback
		if (reports.len()>0) {
			// there are alternatives
			print("Delivering alternative connector")
			r.report = reports.pop()

			if (r.report.action  &&  r.report.action.getclass() == amphibious_connection_planner_t) {
				print("Delivering amphibious_connection_planner_t")
				r.node   = r.report.action
				r.report = null
			}
		}
		else {
			industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_failed);
		}
		return r
	}

	static function find_anchorage(factory, planned_station, planned_harbour_flat, c_harbour_tiles)
	{
		// try to find tiles already covered by some harbours
		local tile_list = ::finder.find_water_places( ::finder.get_tiles_near_factory(factory) )
		local halt_list = factory.get_halt_list()
		local anch = []

		if (tile_list.len()>0  &&  halt_list.len()>0) {
			foreach(tile in tile_list) {

				local h = schedule_entry_x(tile,0,0).get_halt(our_player)
				if (h) {
					foreach(halt in halt_list) {
						if ( (h <=> halt) == 0) {
							anch.append(tile)
						}
					}
				}
			}
		}
		if (anch.len()>0) {
			return anch
		}
		// find places to build harbour
		if (tile_list.len()>0) {
			foreach(tile in tile_list) {
				for(local d = 1; d<16; d*=2) {
					local to = tile.get_neighbour(wt_all, d)

					if (to  &&  to.is_empty()) {
						local ok = false
						if (to.get_slope() !=0) {
							// check place for harbour
							local size = planned_station.get_size(0)
							ok = finder.check_harbour_place(tile, size.x*size.y, dir.backward(d))
						}
						else if (planned_harbour_flat) {
							// check place for flat one
							local size = planned_harbour_flat.get_size(0)
							ok = finder.check_harbour_place(tile, size.x*size.y, dir.backward(d))
							// TODO check that only one direction is possible
						}
						if (ok) {
							anch.append(tile)
							c_harbour_tiles[coord3d_to_key(tile)] <- to
							break
						}
					}
				}
			}
		}
		return anch
	}

	/**
	 * Build harbour at @p tile,
	 * replace water with an array containing all water tiles next to the harbour
	 */
	function build_harbour(tile, water_arr)
	{
		local water = water_arr[0]
		local err = null
		local len = 0
		local dif = { x=tile.x-water.x, y=tile.y-water.y}
		print("Place harbour at " + coord3d_to_string(tile) + " to access " + coord3d_to_string(water) )

		if (tile.get_slope()) {

			local slope = dir.to_slope(coord_to_dir(dif))
			// terraform ??
			if (tile.get_slope() != slope  &&  tile.get_slope() != 2*slope) {
				err = command_x.set_slope(our_player, tile, slope )
				if (err) {
					return err;
				}
			}
			err = command_x.build_station(our_player, tile, planned_station)

			local size = planned_station.get_size(0)
			len = size.x*size.y
		}
		else {
			err = command_x.build_station(our_player, tile, planned_harbour_flat)

			local size = planned_harbour_flat.get_size(0)
			len = size.x*size.y
		}
		if (err) {
			return err;
		}

		// halt at this harbour
		local harbour_halt = schedule_entry_x(tile,0,0).get_halt(our_player)

		water_arr.clear()
		// all water tiles near harbour
		for(local l=0; l<len; l++) {
			for(local off=-1; off<=1; off += (l==len-1?1:2)) {
				local pos = { x = water.x - l*dif.x + off*dif.y,
					        y = water.y - l*dif.y - off*dif.x }
				//
				local to = tile_x(pos.x, pos.y, water.z)
				try {
					if (finder._tile_water(to)) {

						local halt = schedule_entry_x(to,0,0).get_halt(our_player)

						if (halt  &&  ( (halt<=>harbour_halt) != 0) ) {
							// we do not want to use this halt
							continue
						}

						water_arr.append(to)
					}
				}
				catch(ev) {/* ignore */}
			}
		}
		if (water_arr.len()==0) {
			// should not happen
			print("No non-harbour water tiles found near " + coord_to_string(water))
			water_arr.append(water)
		}
		return null
	}

	function find_route()
	{
		local as = route_finder_water()

		local res = as.search_route(c_start, c_end)

		if ("err" in res) {
			return res.err
		}
		c_start = [res.start ]
		c_end   = [res.end   ]

		local asd = route_finder_water_depot()
		res = asd.search_route(as.route)

		if ("err" in res) {
			return res.err
		}
		local d = res.depot
		c_depot = tile_x(d.x, d.y, d.z)
	}
}


class route_finder_water extends astar
{
	function process_node(cnode)
	{
		local from = tile_x(cnode.x, cnode.y, cnode.z)
		local back = dir.backward(cnode.dir)
		local water_dir = from.get_way_dirs(wt_water)

		local test_dir = cnode.previous ? (back ^ 0x0f) & cnode.flag : 0x0f

		for(local d = 1; d<16; d*=2) {
			// do not go backwards
			if (( d &test_dir) ==0 ) {
				continue
			}

			local to = from.get_neighbour(wt_water, d)
			if (to) {
				if (::finder._tile_water_way(to)  &&  !is_closed(to)) {
					// estimate moving cost
					local move = ((dir.double(d) & cnode.dir) != 0) ? /* straight */ 14 : /* curve */ 10
					local dist   = 10*estimate_distance(to)

					local cost   = cnode.cost + move
					local weight = cost + dist
					// use jump-point search (see dataobj/route.cc)
					local jps = cnode.previous ? (water_dir ^ 0x0f) | d | cnode.dir | from.get_canal_ribi() : 0x0f

					local node = ab_node(to, cnode, cost, weight, dist, d, jps)
					add_to_open(node, weight)
				}
			}
		}
	}

	function search_route(start, end)
	{
		prepare_search()
		foreach (e in end) {
			targets.append(e);
		}
		compute_bounding_box()

		foreach (s in start)
		{
			local dist = estimate_distance(s)
			if (dist == 0) {
				continue
			}
			add_to_open(ab_node(s, null, 1, dist+1, dist, 0), dist+1)
		}

		search()

		if (route.len() == 0) {
			print("No water route found")
			return { err =  "No route" }
		}

		if (route.len() > 0) {
			return { start = route[ route.len()-1], end = route[0] }
		}

		print("No water depot route found")
		return { err =  "No route" }
	}
}

class route_finder_water_depot extends route_finder_water
{
	function estimate_distance(tile)
	{
		return tile.get_objects().get_count()
		// take first empty tile

	}

	function search_route(watertiles)
	{
		prepare_search()

		// do not place depot on route between harbours
		foreach(w in watertiles) {
			add_to_close(w)
		}
		// add neighboring tiles of route to open list
		for(local i=0; i<watertiles.len(); i++)
		{
			local dist = (watertiles.len() - i)*10;
			process_node(ab_node(watertiles[i], null, 1, dist+1, dist, 0))
		}

		search()

		if (route.len() > 0) {
			return { depot = route[0] }
		}
		print("No water depot route found")
		return { err =  "No route" }
	}
}
