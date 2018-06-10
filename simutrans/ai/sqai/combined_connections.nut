class amphibious_connection_planner_t extends industry_connection_planner_t
{
	constructor(s,d,f)
	{
		base.constructor(s,d,f);
		name = "amphibious_connection_planner_t"
	}


	function step()
	{
		print("amphibious_connection_planner_t " + this)
		// compute monthly production
		if (prod < 0) {
			prod = calc_production()
		}
		dbgprint("production = " + prod);

		// road - initial
		local rprt_road = plan_simple_connection(wt_road, null, null)
		if (rprt_road == null) {
			return failed()
		}
		// water - initial
		local rprt_water = plan_simple_connection(wt_water, null, null)
		if (rprt_water == null) {
			return failed()
		}

		// find amphibious path
		local marine = amphibious_pathfinder_t(rprt_road.action.planned_way, rprt_water.action.planned_station)
		marine.search_route(fsrc,fdest)

		local route = marine.route
		if (route.len() == 0) {
			return r_t(RT_TOTAL_FAIL)
		}
		// generate report
		local report = report_t()
		report.action = node_seq_t()

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
					r = plan_simple_connection(wt_road, route[from_i-1], change ? route[i+2] : null, from_i-1 - (i+2))
				}
				if (r) {
					r.action.finalize = !change
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

	c_harbour_tiles = null

	constructor(way_, harbour_)
	{
		base.constructor()
		way = way_
		builder = way_planner_x(our_player)
		builder.set_build_types(way)
		planned_harbour = harbour_

		local size = planned_harbour.get_size(0)
		planned_harbour_len = size.x*size.y

		// TODO find flat harbour building
	}

	function process_node(cnode)
	{
		local from = tile_x(cnode.x, cnode.y, cnode.z)
		local back = dir.backward(cnode.dir)
		local from_water = ::finder._tile_water_way(from)
		local water_dir = from.get_way_dirs(wt_water)

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
			if (( d &test_dir) ==0 ) {
				continue
			}

			local to = from.get_neighbour(wt_all, d)

			if (to  &&  !is_closed(to)) {
				local to_water = ::finder._tile_water_way(to)

				// water -> water
				// land -> land
				if (from_water ==  to_water) {
					// can we build way here
					if (!from_water  &&  !builder.is_allowed_step(from, to)) {
						continue;
					}
					// estimate moving cost
					local move = ((dir.double(d) & cnode.dir) != 0) ? /* straight */ 14 : /* curve */ 10
					local dist   = 10*estimate_distance(to)

					local cost   = cnode.cost + move
					local weight = cost + dist
					// use jump-point search (see dataobj/route.cc)
					local jps = to_water  &&  (cnode.previous) ? (water_dir ^ 0x0f) | d | cnode.dir | from.get_canal_ribi() : 0x0f

					if (cnode.flag & 0x20) jps = jps | 0x40;

					local node = ab_node(to, cnode, cost, weight, dist, d, jps)
					add_to_open(node, weight)
				}
				else if (from_water) {
					// water -> land
					if (!from.is_water()  ||  !to.is_empty()  ||  to.get_slope()==0
						||  !finder.check_harbour_place(from, planned_harbour_len, dir.backward(d)))
					{
						continue
					}
					local move   = 333;
					local dist   = 10*estimate_distance(to)
					local weight = cnode.cost + dist

					local node = ab_node(to, cnode, cnode.cost + move, weight, dist, d, 0x10)
					add_to_open(node, weight)
				}
				else {
					// land -> water
					if (!to.is_water()  ||  !from.is_empty()  ||  from.get_slope()==0
						||  (cnode.flag & 0x60)
						||  !finder.check_harbour_place(to, planned_harbour_len, d))
					{
						continue
					}
					local move   = 333;
					local dist   = 10*estimate_distance(to)
					local weight = cnode.cost + dist

					local node = ab_node(to, cnode, cnode.cost + move, weight, dist, d, 0x0f)
					add_to_open(node, weight)
				}
			}
		}
	}

	function process_node_to_land(cnode, from)
	{
		local pos = coord(cnode.x, cnode.y)
		//label_x.create(cnode, our_player, "n2l")

		for(local d0 = 1; d0<16; d0*=2) {
			for (local d = d0; d <= 3*d0; d+=2*d0) {

				local c = pos + dir.to_coord(d)

				try {
					local to = square_x(c.x, c.y).get_ground_tile()
					if (to  &&  to.is_empty()  &&  to.get_slope()==0) {
						// can place station here
						local move   = 17
						local dist   = 10*estimate_distance(to)

						local cost   = cnode.cost + move
						local weight = cost + dist

						local node = ab_node(to, cnode, cost, weight, dist, 0x0f, 0x0f)
						add_to_open(node, weight)
					}
				}
				catch(ev) {}
			}
		}
	}

	function search_route(fsrc, fdest)
	{
		print("Search amphibious connection")
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
	}
}
