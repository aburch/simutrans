class road_connector_t extends manager_t
{
	// input data
	fsrc = null
	fdest = null
	freight = null
	planned_way = null
	planned_station = null
	planned_depot = null
	planned_convoy = null

	// step-by-step construct the connection
	phase = 0
	// generated data
	c_start = null
	c_end   = null
	c_depot = null
	c_sched = null
	c_line  = null
	c_cnv   = null

	constructor()
	{
		base.constructor("road_connector_t")
		debug = true
	}

	function work()
	{
		// TODO check if child does the right thing
		local pl = our_player
		local tic = get_ops_total();

		switch(phase) {
			case 0: // find places for stations
				c_start = find_station_place(fsrc, fdest)
				if (c_start) {
					c_end   = find_station_place(fdest, c_start, true)
				}

				if (c_start.len()>0  &&  c_end.len()>0) {
					phase ++
				}
				else {
					print("No station places found")
					error_handler()
					return r_t(RT_TOTAL_FAIL)
				}
			case 1: // build way
				{
					sleep()
					local d = pl.get_current_cash();
					local err = construct_road(pl, c_start, c_end, planned_way )
					print("Way construction cost: " + (d-pl.get_current_cash()) )
					if (err) {
						print("Failed to build way from " + coord_to_string(c_start[0])+ " to " + coord_to_string(c_end[0]))
						error_handler()
						return r_t(RT_TOTAL_FAIL)
					}
					phase ++
				}
			case 2: // build station
				{
					local err = command_x.build_station(pl, c_start, planned_station )
					if (err) {
						print("Failed to build station at " + coord_to_string(c_start))
						error_handler()
						return r_t(RT_TOTAL_FAIL)
					}
					local err = command_x.build_station(pl, c_end, planned_station )
					if (err) {
						print("Failed to build station at " + coord_to_string(c_end))
						error_handler()
						return r_t(RT_TOTAL_FAIL)
					}
					{
						// store place of unload station for future use
						local fs = ::station_manager.access_freight_station(fdest)
						if (fs.road_unload == null) {
							fs.road_unload = c_end
						}
					}
					phase ++
				}
			case 3: // find depot place
				{
					local err = construct_road_to_depot(pl, c_start, planned_way)
					if (err) {
						print("Failed to build depot access from " + coord_to_string(c_start))
						error_handler()
						return r_t(RT_TOTAL_FAIL)
					}
					phase += 2
				}
			case 4: // build way to depot
				{
					local err = command_x.build_way(pl, c_start, c_depot, planned_way, false)
					if (err) {
						print("Failed to build depot access from " + coord_to_string(c_start)+ " to " + coord_to_string(c_depot))
						error_handler()
						return r_t(RT_TOTAL_FAIL)
					}
					phase ++
				}
			case 5: // build depot
				{
					// depot already existing ?
					if (c_depot.find_object(mo_depot_road) == null) {
						// no: build
						local err = command_x.build_depot(pl, c_depot, planned_depot )
						if (err) {
							print("Failed to build depot at " + coord_to_string(c_depot))
							error_handler()
							return r_t(RT_TOTAL_FAIL)
						}
						{
							// store depot location
							local fs = ::station_manager.access_freight_station(fsrc)
							if (fs.depot == null) {
								fs.depot = c_depot
							}
						}
					}
					phase ++
				}
			case 6: // create schedule
				{
					local sched = schedule_x(wt_road, [])
					sched.entries.append( schedule_entry_x(c_start, 100, 0) );
					sched.entries.append( schedule_entry_x(c_end, 0, 0) );
					c_sched = sched
					phase ++
				}

			case 7: // create line and set schedule
				{
					pl.create_line(wt_road)

					// find the line - it is a line without schedule and convoys
					local list = pl.get_line_list()
					foreach(line in list) {
						if (line.get_waytype() == wt_road  &&  line.get_schedule().entries.len()==0) {
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
					print("road_connector wasted " + (toc-tic) + " ops")

					phase ++
					return r_t(RT_PARTIAL_SUCCESS)
				}
			case 9: // build station extension

		}

		industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_built)
		industry_manager.access_link(fsrc, fdest, freight).append_line(c_line)

		return r_t(RT_TOTAL_SUCCESS)
	}

	function error_handler()
	{
		industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_failed);
	}

	function get_tiles_near_factory(factory)
	{
		local cov = settings.get_station_coverage()
		local area = []

		// generate a list of tiles that will reach the factory
		local ftiles = factory.get_tile_list()
		foreach (c in ftiles) {
			for(local dx = -cov; dx <= cov; dx++) {
				for(local dy = -cov; dy <= cov; dy++) {
					if (dx==0 && dy==0) continue;

					local x = c.x+dx
					local y = c.y+dy

					if (x>=0 && y>=0) area.append( (x << 16) + y );
				}
			}
		}
		// sort
		sleep()
		area.sort(/*compare_coord*/)
		return area
	}

	function find_empty_place(area, target)
	{
		// find place closest to target
		local tx = target.x
		local ty = target.y

		local best = null
		local dist = 10000
		// check for flat and empty ground
		for(local i = 0; i<area.len(); i++) {

			local h = area[i]
			if (i>0  &&  h == area[i-1]) continue;

			local x = h >> 16
			local y = h & 0xffff

			if (world.is_coord_valid({x=x,y=y})) {
				local tile = square_x(x, y).get_ground_tile()

				if (tile.is_empty()  &&  tile.get_slope()==0) {
					local d = abs(x - tx) + abs(y - ty)
					if (d < dist) {
						dist = d
						best = tile
					}
				}
			}
		}
		return best
	}

	function find_empty_places(area)
	{
		local list = []
		// check for flat and empty ground
		for(local i = 0; i<area.len(); i++) {

			local h = area[i]
			if (i>0  &&  h == area[i-1]) continue;

			local x = h >> 16
			local y = h & 0xffff

			if (world.is_coord_valid({x=x,y=y})) {
				local tile = square_x(x, y).get_ground_tile()

				if (tile.is_empty()  &&  tile.get_slope()==0) {
					list.append(tile)
				}
			}
		}
		return list.len() > 0 ?  list : []
	}

	function find_station_place(factory, target, unload = false)
	{
		if (unload) {
			// try unload station from station manager
			local res = ::station_manager.access_freight_station(factory).road_unload
			if (res) {
				return [res]
			}
		}
		local area = get_tiles_near_factory(factory)

		return find_empty_places(area)
	}

	function find_depot_place(start, target)
	{
		{
			// try depot location from station manager
			local res = ::station_manager.access_freight_station(fsrc).depot
			if (res) {
				return res
			}
		}

		local cov = 5
		local area = []

		local t = tile_x(start.x, start.y, start.z)

		// follow the road up to the next crossing
		// check whether depot can be built next to road
		local d = t.get_way_dirs(wt_road)
		if (!dir.is_single(d)) {
			d = d	& dir.southeast // only s or e
		}
		for(local i=0; i<4; i++) {
			local nt = t.get_neighbour(wt_road, d)
			if (nt == null) break
			// should have a road
			local rd = nt.get_way_dirs(wt_road)
			// find direction to proceed: not going back, where we were coming from
			local nd = rd & (~(dir.backward(d)))
			// loop through neighbor tiles not on the road
			foreach(d1 in dir.nsew) {
				if (d1 & rd ) continue
				// test this spot
				local dp = nt.get_neighbour(wt_all, d1)
				if (dp  &&  dp.is_empty()  &&  dp.get_slope()==0) {
					return dp
				}
			}
			if (!dir.is_single(nd)  ||  nd==0) break
			// proceed
			t = nt
			d = nd
		}

		t = tile_x(start.x, start.y, start.z)
		// now go into direction perpendicular
		// to the road in the station
		local dirs = dir.double( t.get_way_dirs(wt_road) ) ^ dir.all

		// try to find depot spot next to the station
		for(local i = 1;  i < 16; i=i<<1)
		{
			if (i & dirs) {
				local n = t.get_neighbour(wt_all, i)
				if (n  &&  n.is_empty()  &&  n.get_slope()==0) {
					return n
				}
			}
		}

		// generate a list of tiles near the station
		for(local dx = -cov; dx <= cov; dx++) {
			for(local dy = -cov; dy <= cov; dy++) {
				if (dx==0 && dy==0) continue;

				local x = start.x+dx
				local y = start.y+dy

				if (x>=0 && y>=0) area.append( (x << 16) + y );
			}
		}
		return find_empty_place(area, target)
	}

	function construct_road(pl, starts, ends, way)
	{
		local as = astar_builder()
		as.builder = way_planner_x(pl)
		as.way = way
		as.builder.set_build_types(way)
		as.bridger = pontifex(pl, way)
		if (as.bridger.bridge == null) {
			as.bridger = null
		}
		local res = as.search_route(starts, ends)

		if ("err" in res) {
			return res.err
		}
		c_start = res.start
		c_end   = res.end
	}

	function construct_road_to_depot(pl, start, way)
	{
		local as = depot_pathfinder()
		as.builder = way_planner_x(pl)
		as.way = way
		as.builder.set_build_types(way)
		local res = as.search_route(start)

		if ("err" in res) {
			return res.err
		}
		local d = res.end
		c_depot = tile_x(d.x, d.y, d.z)
	}
}


class depot_pathfinder extends astar_builder
{
	function estimate_distance(c)
	{
		local t = tile_x(c.x, c.y, c.z)
		if (t.is_empty()  &&  t.get_slope()==0) {
			return 0
		}
		return 10
	}
	function search_route(start)
	{
		prepare_search()

		local dist = estimate_distance(start)
		add_to_open(ab_node(start, null, 1, dist+1, dist, 0), dist+1)

		search()

		if (route.len() > 0) {

			for (local i = 1; i<route.len(); i++) {
				local err = command_x.build_way(our_player, route[i-1], route[i], way, false )
				if (err) {
					label_x.create(node, our_player, "<" + err + ">")
					return { err =  err }
				}
			}
			return { start = route[ route.len()-1], end = route[0] }
		}
		print("No route found")
		return { err =  "No route" }
	}
}



