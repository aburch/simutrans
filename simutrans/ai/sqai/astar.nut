class astar_node extends coord3d
{
	previous = null // previous node
	cost     = -1   // cost to reach this node
	weight   = -1   // heuristic cost to reach target
	dist     = -1   // distance to target
	constructor(c, p, co, w, d)
	{
		x = c.x
		y = c.y
		z = c.z
		previous = p
		cost     = co
		weight   = w
		dist     = d
	}
}

function abs(x) { return x>0 ? x : -x }

class astar
{
	closed_list = null // table
	nodes       = null // array of astar_node
	heap        = null // binary heap
	targets     = null // array of coord3d's

	boundingbox = null // box containing all the targets

	route       = null // route, reversed: target to start

	constructor()
	{
		closed_list = {}
		heap        = simple_heap_x()
		targets     = []

	}

	function prepare_search()
	{
		closed_list = {}
		nodes       = []
		route       = []
		heap.clear()
		targets     = []
	}

	function add_to_close(c)
	{
		closed_list[ coord3d_to_key(c) ] <- 1
	}

	function test_and_close(c)
	{
		local key = coord3d_to_key(c)
		if (key in closed_list) {
			return false
		}
		else {
			closed_list[ key ] <- 1
			return true
		}
	}

	function is_closed(c)
	{
		local key = coord3d_to_key(c)
		return (key in closed_list)
	}

	function add_to_open(c, weight)
	{
		local i = nodes.len()
		nodes.append(c)
		heap.insert(weight, i)
	}

	function search()
	{
		// compute bounding box of targets
		compute_bounding_box()

		local current_node = null
		while (!heap.is_empty()) {

			local wi = heap.pop()
			current_node = nodes[wi.value]
			local dist = current_node.dist

			// target reached
			if (dist == 0) break;
			// already visited previously
			if (!test_and_close(current_node)) {
				current_node = null
				continue;
			}
			// investigate neighbours and put them into open list
			process_node(current_node)

			current_node = null
		}

		route = []
		if (current_node) {
			// found route
			route.append(current_node)

			while (current_node.previous) {
				current_node = current_node.previous
				route.append(current_node)
			}
		}
	}

	function compute_bounding_box()
	{
		if (targets.len()>0) {
			local first = targets[0]
			boundingbox = { xmin = first.x, xmax = first.x, ymin = first.y, ymax = first.y }

			for(local i=1; i < targets.len(); i++) {
				local t = targets[i]
				if (boundingbox.xmin > t.x) boundingbox.xmin = t.x;
				if (boundingbox.xmax < t.x) boundingbox.xmax = t.x;
				if (boundingbox.ymin > t.y) boundingbox.ymin = t.y;
				if (boundingbox.ymax < t.y) boundingbox.ymax = t.y;
			}
		}
	}

	function estimate_distance(c)
	{
		local d = 0

		local t
		t = boundingbox.xmin - c.x; if (t>0) d += t;
		t = c.x - boundingbox.xmax; if (t>0) d += t;
		t = boundingbox.ymin - c.y; if (t>0) d += t;
		t = c.y - boundingbox.ymax; if (t>0) d += t;

		if (d==0) {
			local t = targets[0]
			d = abs(t.x-c.x) + abs(t.y-c.y)

			// inside bounding box
			for(local i=1; i < targets.len(); i++) {
				local t = targets[i]
				d = min(d, abs(t.x-c.x) + abs(t.y-c.y) )
			}
		}
		return d
	}

	function coord3d_to_key(c)
	{
		return c.x + ":" + c.y + ":" + c.z;
	}
}

class ab_node extends ::astar_node
{
	dir = 0 // direction to reach this node
	flag = 0
	constructor(c, p, co, w, d, di, fl=0)
	{
		base.constructor(c, p, co, w, d)
		dir  = di
		flag = fl
	}
}


class pontifex
{
	player = null
	bridge = null

	constructor(pl, way)
	{
		player = pl
		local list = bridge_desc_x.get_available_bridges(way.get_waytype())
		local len = list.len()
		local way_speed = way.get_topspeed()
		if (len>0) {
			bridge = list[0]
			for(local i=1; i<len; i++) {
				local b = list[i]
				if (bridge.get_topspeed() < way_speed) {
					if (b.get_topspeed() > bridge.get_topspeed()) {
						bridge = b
					}
				}
				else {
					if (way_speed < b.get_topspeed()  && b.get_topspeed()  < bridge.get_topspeed()) {
						bridge = b
					}
				}
			}
		}
	}

	function find_end(pos, dir, min_length)
	{
		return bridge_planner_x.find_end(player, pos, dir, bridge, min_length)
	}
}


class astar_builder extends astar
{
	builder = null
	bridger = null
	way     = null



	function process_node(cnode)
	{
		local from = tile_x(cnode.x, cnode.y, cnode.z)
		local back = dir.backward(cnode.dir)

		for(local d = 1; d<16; d*=2) {
			// do not go backwards
			if (d == back) {
				continue
			}
			// continue straight after a bridge
			if (cnode.flag == 1  &&  d != cnode.dir) {
				continue
			}

			local to = from.get_neighbour(wt_all, d)
			if (to) {
				if (builder.is_allowed_step(from, to)  &&  !is_closed(to)) {
					// estimate moving cost
					local move = ((dir.double(d) & cnode.dir) != 0) ? /* straight */ 14 : /* curve */ 10
					local dist   = 10*estimate_distance(to)
					// is there already a road?
					if (!to.has_way(wt_road)) {
						move += 8
					}

					local cost   = cnode.cost + move
					local weight = cost + dist
					local node = ab_node(to, cnode, cost, weight, dist, d)

					add_to_open(node, weight)
				}
				// try bridges
				else if (bridger  &&  d == cnode.dir) {
					local len = 1
					local max_len = bridger.bridge.get_max_length()

					do {
						local to = bridger.find_end(to, d, len)
						if (to.x < 0  ||  is_closed(to)) {
							break
						}
						local bridge_len = abs(from.x-to.x) + abs(from.y-to.y)

						local move = bridge_len * 14 /* straight */  * 3  /*extra bridge penalty */;
						local dist = 10*estimate_distance(to)

						local cost   = cnode.cost + move
						local weight = cost + dist
						local node = ab_node(to, cnode, cost, weight, dist, d, 1 /*bridge*/)

						add_to_open(node, weight)

						len = bridge_len + 1
					} while (len <= max_len)
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
			add_to_open(ab_node(s, null, 1, dist+1, dist, 0), dist+1)
		}

		search()

		if (route.len() > 0) {
			local w = command_x(tool_build_way);
			w.set_flags(2)
			local b = command_x(tool_build_bridge);
			b.set_flags(2)

			for (local i = 1; i<route.len(); i++) {
				local err

				if (route[i-1].flag == 0) {
					w.set_flags(0)
					err = w.work(our_player, route[i-1], route[i], way.get_name() )
				}
				else if (route[i-1].flag == 1) {
					b.set_flags(2)
					err = b.work(our_player, route[i], route[i-1], bridger.bridge.get_name() )
				}
				if (err) {
					label_x.create(route[i], our_player, "<" + err + "> " + coord3d_to_string(route[i-1]) + " / " +  route[i-1].flag + " / " + route[i].flag )
					return { err =  err }
				}
			}
			return { start = route[ route.len()-1], end = route[0] }
		}
		print("No route found")
		return { err =  "No route" }
	}
}

