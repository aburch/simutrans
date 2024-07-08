/**
 * Classes to help with route-searching.
 * Based on the A* algorithm.
 */


/**
 * Nodes for A*
 */
class astar_node extends coord3d
{
  previous = null // previous node
  cost     = -1   // cost to reach this node
  dist     = -1   // distance to target
  constructor(c, p, co, d)
  {
    x = c.x
    y = c.y
    z = c.z
    previous = p
    cost     = co
    dist     = d
  }
  function is_straight_move(d)
  {
    return d == dir ||  (previous  &&  previous.dir == d)
  }
}

/**
 * Class to perform A* searches.
 *
 * Derived classes have to implement:
 *    process_node(node): add nodes to open list reachable by node
 *
 * To use this:
 * 1) call prepare_search
 * 2) add tiles to target array
 * 3) call compute_bounding_box
 * 4) add start tiles to open list
 * 5) call search()
 * 6) use route
 */
class astar
{
  closed_list = null // table
  nodes       = null // array of astar_node
  heap        = null // binary heap
  targets     = null // array of coord3d's

  boundingbox = null // box containing all the targets

  route       = null // route, reversed: target to start

  // statistics
  calls_open = 0
  calls_closed = 0
  calls_pop = 0

  // costs - can be fine-tuned
  cost_straight = 10
  cost_curve    = 14

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
    calls_open = 0
    calls_closed = 0
    calls_pop = 0
  }

  // adds node c to closed list
  function add_to_close(c)
  {
    closed_list[ coord3d_to_key(c) ] <- 1
    calls_closed++
  }

  function test_and_close(c)
  {
    local key = coord3d_to_key(c)
    if (key in closed_list) {
      return false
    }
    else {
      closed_list[ key ] <- 1
      calls_closed++
      return true
    }
  }

  function is_closed(c)
  {
    local key = coord3d_to_key(c)
    return (key in closed_list)
  }

  // add node c to open list with give weight
  function add_to_open(c, weight)
  {
    local i = nodes.len()
    nodes.append(c)
    heap.insert(weight, i)
    calls_open++
  }

  function search()
  {
    // compute bounding box of targets
    compute_bounding_box()

    local current_node = null
    while (!heap.is_empty()) {
      calls_pop++

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

    print("Calls: pop = " + calls_pop + ", open = " + calls_open + ", close = " + calls_closed)
  }

  /**
   * Computes bounding box of all targets to speed up distance computation.
   */
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

  /**
   * Estimates distance to target.
   * Returns zero if and only if c is a target tile.
   */
  function estimate_distance(c)
  {
    local d = 0
    local curved = 0

    // distance to bounding box
    local dx = boundingbox.xmin - c.x
    if (dx <= 0) dx = c.x - boundingbox.xmax;
    if (dx > 0) d += dx; else dx = 0;

    local dy = boundingbox.ymin - c.y
    if (dy <= 0) dy = c.y - boundingbox.ymax
    if (dy > 0) d += dy; else dy = 0;

    if (d > 0) {
      // cost to bounding box
      return cost_straight * d + (dx*dy > 0 ? cost_curve - cost_straight : 0);
    }
    else {
      local t = targets[0]
      d = abs(t.x-c.x) + abs(t.y-c.y)

      // inside bounding box
      for(local i=1; i < targets.len(); i++) {
        local t = targets[i]
        local dx = abs(t.x-c.x)
        local dy = abs(t.y-c.y)
        d = min(d, cost_straight * (dx+dy) + (dx*dy > 0 ? cost_curve - cost_straight : 0))
      }
    }
    return d
  }
}

/**
 * Class to search a route along existing ways.
 */
class astar_route_finder extends astar
{
  wt = wt_all

  constructor(wt_)
  {
    base.constructor()
    wt = wt_
    if ( [wt_all, wt_invalid, wt_water, wt_air].find(wt) ) {
      throw("Using this waytype is going to be inefficient. Use at own risk.")
    }
    cost_curve = cost_straight
  }

  function process_node(cnode)
  {
    local from = tile_x(cnode.x, cnode.y, cnode.z)
    local back = dir.backward(cnode.dir)
    // allowed directions
    local dirs = from.get_way_dirs_masked(wt)

    for(local d = 1; d<16; d*=2) {
      // do not go backwards, only along existing ways
      if ( d == back  ||  ( (dirs & d) == 0) ) {
        continue
      }

      local to = from.get_neighbour(wt, d)
      if (to) {
        if (!is_closed(to)) {
          // estimate moving cost
          local move = cnode.is_straight_move(d)  ?  cost_straight  :  cost_curve
          local dist   = estimate_distance(to)
          local cost   = cnode.cost + move
          local weight = cost //+ dist
          local node = ab_node(to, cnode, cost, dist, d)

          add_to_open(node, weight)
        }
      }
    }
  }

  // start and end have to be arrays of objects with 3d-coordinates
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
      add_to_open(ab_node(s, null, 1, dist+1, 0, 0), dist+1)
    }

    search()

    if (route.len() > 0) {
      return { start = route[route.len()-1], end = route[0], routes = route }
    }
    print("No route found")
    return { err =  "No route" }
  }
}

/**
 * Class to search a route along existing ways.
 */
class astar_route_finder extends astar
{
  wt = wt_all

  constructor(wt_)
  {
    base.constructor()
    wt = wt_
    if ( [wt_all, wt_invalid, wt_water, wt_air].find(wt) ) {
      throw("Using this waytype is going to be inefficient. Use at own risk.")
    }
    cost_curve = cost_straight
  }

  function process_node(cnode)
  {
    local from = tile_x(cnode.x, cnode.y, cnode.z)
    local back = dir.backward(cnode.dir)
    // allowed directions
    local dirs = from.get_way_dirs_masked(wt)

    for(local d = 1; d<16; d*=2) {
      // do not go backwards, only along existing ways
      if ( d == back  ||  ( (dirs & d) == 0) ) {
        continue
      }

      local to = from.get_neighbour(wt, d)
      if (to) {
        if (!is_closed(to)) {
          // estimate moving cost
          local move = cnode.is_straight_move(d)  ?  cost_straight  :  cost_curve
          local dist   = estimate_distance(to)
          local cost   = cnode.cost + move
          local weight = cost //+ dist
          local node = ab_node(to, cnode, cost, dist, d)

          add_to_open(node, weight)
        }
      }
    }
  }

  // start and end have to be arrays of objects with 3d-coordinates
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
      add_to_open(ab_node(s, null, 1, dist+1, 0, 0), dist+1)
    }

    search()

    if (route.len() > 0) {
      return { start = route.top(), end = route[0], routes = route }
    }
    print("No route found")
    return { err =  "No route" }
  }
}

class ab_node extends ::astar_node
{
  dir = 0   // direction to reach this node
  flag = 0  // flag internal to the route searcher
  constructor(c, p, co, d, di, fl=0)
  {
    base.constructor(c, p, co, d)
    dir  = di
    flag = fl
  }
}

/**
 * Helper class to find bridges and spots to place them.
 */
class pontifex
{
  player = null
  bridge = null

  constructor(pl, way)
  {
    // print messages box
    // 1 = erreg
    // 2 = list bridges
    local print_message_box = 0
    local wt_name = ["", "road", "rail", "water"]

    if ( print_message_box > 1 ) {
      gui.add_message_at(pl, "____________ Search bridge ___________", world.get_time())
    }
    player = pl
    local list = bridge_desc_x.get_available_bridges(way.get_waytype())
    local len = list.len()
    local way_speed = way.get_topspeed()
    local bridge_min_len = 5

    if (len>0) {
      bridge = list[0]
        if ( print_message_box == 2 ) {
          gui.add_message_at(pl, " ***** way : " + wt_name[way.get_waytype()], world.get_time())
          gui.add_message_at(pl, " ***** bridge : " + bridge.get_name(), world.get_time())
          gui.add_message_at(pl, " ***** get_max_length : " + bridge.get_max_length(), world.get_time())
        }

      for(local i=1; i<len; i++) {
        local b = list[i]
        if ( print_message_box == 2 ) {
          gui.add_message_at(pl, " ***** way : " + wt_name[way.get_waytype()], world.get_time())
          gui.add_message_at(pl, " ***** bridge : " + b.get_name(), world.get_time())
          gui.add_message_at(pl, " ***** get_max_length : " + b.get_max_length(), world.get_time())
        }
        if ( b.get_max_length() > bridge_min_len || b.get_max_length() == 0 ) {
          if (bridge.get_topspeed() < way_speed) {
            if (b.get_topspeed() > bridge.get_topspeed()) {
              bridge = b
            }
          }
          else {
            if (way_speed < b.get_topspeed() && b.get_topspeed() < bridge.get_topspeed()) {
              bridge = b
            }
          }
        }
      }
    }
    if ( print_message_box > 1 ) {
      gui.add_message_at(pl, " *** bridge found : " + bridge.get_name() + " way : " + wt_name[way.get_waytype()], world.get_time())
      gui.add_message_at(our_player, "--------- Search bridge end ----------", world.get_time())
    }
  }

  function find_end(pos, dir, min_length)
  {
    return bridge_planner_x.find_end(player, pos, dir, bridge, min_length)
  }
}

/**
 * Class to search a route and to build a connection (i.e. roads).
 * Builds bridges. But not tunnels (not implemented).
 */
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
          local move = cnode.is_straight_move(d)  ?  cost_straight  :  cost_curve
          local dist   = estimate_distance(to)
          // is there already a road?
          if (!to.has_way(wt_road)) {
            move += 8
          }

          local cost   = cnode.cost + move
          local weight = cost + dist
          local node = ab_node(to, cnode, cost, dist, d)

          add_to_open(node, weight)
        }
        // try bridges
        else if (bridger  &&  d == cnode.dir  &&  cnode.flag != 1) {
          local len = 1
          local max_len = bridger.bridge.get_max_length()

          do {
            local to = bridger.find_end(from, d, len)
            if (to.x < 0  ||  is_closed(to)) {
              break
            }
            local bridge_len = abs(from.x-to.x) + abs(from.y-to.y)

            // long bridges bad
            local bridge_factor = 3

            if ( bridge_len > 20 ) {
              bridge_factor = 4
            }/* else if ( bridge_len > 8 ) {
              bridge_factor = 4
            }*/
            local move = bridge_len * cost_straight  * bridge_factor  /*extra bridge penalty */;
            // set distance to 1 if at a target tile
            local dist = max(estimate_distance(to), 1)

            local cost   = cnode.cost + move
            local weight = cost + dist
            local node = ab_node(to, cnode, cost, dist, d, 1 /*bridge*/)

            add_to_open(node, weight)

            len = bridge_len + 1
          } while (len <= max_len)
        }
      }
    }
  }

  function search_route(start, end, build_route = 1)
  {

    if ( start.len() == 0 || end.len() == 0 ) {
      if ( print_message_box > 0 ) {
        gui.add_message_at(our_player, " *** invalid tile : start or end ", world.get_time())
      }
      return { err =  "No route" }
    }

    prepare_search()
    foreach (e in end) {
      targets.append(e);
    }
    compute_bounding_box()

    foreach (s in start)
    {
      local dist = estimate_distance(s)
      add_to_open(ab_node(s, null, 1, dist+1, 0, 0), dist+1)
    }

    search()

    local bridge_tiles = 0
    local count_tree = 0

    if (route.len() > 0) {
      remove_field( route[0] )

      // do not try to build in tunnels
      local is_tunnel_0 = tile_x(route[0].x, route[0].y, route[0].z).find_object(mo_tunnel)
      local is_tunnel_1 = is_tunnel_0

      local last_treeway_tile = null

      for (local i = 1; i<route.len(); i++) {
        // remove any fields on our routes (only start & end currently)
        remove_field( route[i] )

        // check for tunnel
        is_tunnel_0 = is_tunnel_1
        is_tunnel_1 = tile_x(route[i].x, route[i].y, route[i].z).find_object(mo_tunnel)

        if (is_tunnel_0 && is_tunnel_1) {
          continue
        }

        local err
        // build
        if (route[i-1].flag == 0) {
          /*if ( way.get_waytype() == wt_road ) {
            if ( build_route == 1 ) {
              err = command_x.build_road(our_player, route[i-1], route[i], way, true, true)
              if (err) {
                //gui.add_message_at(our_player, "Failed to build " + way.get_name() + " from " + coord_to_string(route[i-1]) + " to " + coord_to_string(route[i]) +"\n" + err, route[i])
                remove_wayline(route, (i - 1), way.get_waytype())
              }

            }

          } else {*/
            if ( build_route == 1 ) {
              // test way is available
              if ( !way.is_available(world.get_time()) ) {
                way = find_object("way", way.get_waytype(), way.get_topspeed())
              }

              local t = tile_x(route[i].x, route[i].y, route[i].z)
              local d = t.get_way_dirs(way.get_waytype())
              local test_exists_way = t.find_object(mo_way)

              local check_build_tile = true
              if ( i > 1 && i < (route.len()-1) ) {
                local tx_0 = tile_x(route[i-1].x, route[i-1].y, route[i-1].z)
                local tx_1 = tile_x(route[i+1].x, route[i+1].y, route[i+1].z)
                if ( tx_0.find_object(mo_way) != null && tx_1.find_object(mo_way) != null ) {
                  //gui.add_message_at(our_player, " check tx_0 and tx_1 ", t)
                  if ( test_exists_way == null ) {
                    local ty = route[i]
                    local cnv_count = tx_0.find_object(mo_way).get_convoys_passed()[0] + tx_0.find_object(mo_way).get_convoys_passed()[1]

                    if ( last_treeway_tile != null && cnv_count == 0 ) {
                      ty = route[last_treeway_tile]
                    }
                    err = test_select_way(tx_1, tx_0, ty, way.get_waytype())
                    //gui.add_message_at(our_player, " check tx_0 and tx_1 : test_select_way " + err, t)
                    if ( err ) {
                      check_build_tile = false
                    }
                    err = null
                  }
                } else if ( test_exists_way != null && test_exists_way.get_waytype() == way.get_waytype() ) {
                  check_build_tile = false
                }
                if ( tx_0.find_object(mo_signal) != null ) {
                  check_build_tile = false

                }
              }

              if ( test_exists_way != null && test_exists_way.get_owner() != our_player.nr ) { //&& last_treeway_tile != null
                //gui.add_message_at(our_player, "test_exists_way " + test_exists_way + " last_treeway_tile " + last_treeway_tile + " test_exists_way.get_waytype() " + test_exists_way.get_waytype() + " !t.is_bridge() " + !t.is_bridge() + " t.get_slope() " + t.get_slope(), t)

                  test_exists_way = null


              }

              if ( t.is_bridge() ) {
                //gui.add_message_at(our_player, " t.is_bridge() " + t.is_bridge(), t)
                last_treeway_tile = null
              }

              if ( i > 2 && test_exists_way != null && last_treeway_tile != null && test_exists_way.get_waytype() == wt_rail && t.get_slope() == 0 ) {
                //gui.add_message_at(our_player, " (624) ", t)
                err = test_select_way(route[i], route[last_treeway_tile], route[i-1], way.get_waytype())
                if ( err ) {
                  last_treeway_tile = null
                } else {
                  test_exists_way = null
                  last_treeway_tile = null
                }
                err = null
              }
              /*if ( way.get_waytype() == wt_rail && !t.is_bridge() && t.get_slope == 0 ) {
                t = tile_x(route[i-1].x, route[i-1].y, route[i-1].z)
                d = t.get_way_dirs(way.get_waytype())
                if ( dir.is_threeway(d) ) {
                  last_treeway_tile = i - 1
                } else {
                  last_treeway_tile = null
                  test_exists_way = null
                }

              }*/
              if ( test_exists_way != null && ( i < 2 || test_exists_way.get_waytype() == wt_road ) ) {
                test_exists_way = null
              }

              local build_tile = false
              if ( settings.get_pay_for_total_distance_mode == 2 && test_exists_way == null && check_build_tile ) {
                err = command_x.build_way(our_player, route[i-1], route[i], way, true)
                build_tile = true
              } else if ( test_exists_way == null && check_build_tile ) {
                err = command_x.build_way(our_player, route[i-1], route[i], way, false)
                build_tile = true
              }
              if (err) {
                //gui.add_message_at(our_player, "Failed to build " + way.get_name() + " from " + coord_to_string(route[i-1]) + " to " + coord_to_string(route[i]) +"\n" + err, route[i])
                // remove way
                // route[0] to route[i]
                //err = command_x.remove_way(our_player, route[0], route[i])
                remove_wayline(route, (i - 1), way.get_waytype())
              } else {
                t = tile_x(route[i-1].x, route[i-1].y, route[i-1].z)
                d = t.get_way_dirs(way.get_waytype())
                //gui.add_message_at(our_player, " (666) dir.is_threeway(d) " + dir.is_threeway(d), t)
                if ( dir.is_threeway(d) && way.get_waytype() == wt_rail && build_tile ) {
                  last_treeway_tile = i - 1
                }
              }
            } else if ( build_route == 0 ) {
              if ( tile_x(route[i].x, route[i].y, route[i].z).find_object(mo_tree) != null ) {
                count_tree++
              }
            }
          //}
        }
        else if (route[i-1].flag == 1) {
          // plan build bridge

          local b_tiles = 0

          //
            if ( route[i-1].x == route[i].x ) {
              if ( route[i-1].y > route[i].y ) {
                b_tiles = (route[i-1].y - route[i].y + 1)
                bridge_tiles += b_tiles
              } else {
                b_tiles = (route[i].y - route[i-1].y + 1)
                bridge_tiles += b_tiles
              }
            } else if ( route[i-1].y == route[i].y ) {
              if ( route[i-1].x > route[i].x ) {
                b_tiles = (route[i-1].x - route[i].x + 1)
                bridge_tiles += b_tiles
              } else {
                b_tiles = (route[i].x - route[i-1].x + 1)
                bridge_tiles += b_tiles
              }
            }


          if ( build_route == 1 ) {
            // check ground under bridge
            // check_ground() return true build bridge
            // check_ground() return false no build bridge

            local build_bridge = true
            // check whether the ground can be adjusted and no bridge is necessary
            // bridge len <= 4 tiles
            if ( b_tiles < 8 ) {
              build_bridge = check_ground(tile_x(route[i-1].x, route[i-1].y, route[i-1].z), tile_x(route[i].x, route[i].y, route[i].z), way)
              //gui.add_message_at(our_player, "check_ground(pos_s, pos_e) --- " + build_bridge, route[i-1])
            }

            if ( build_bridge ) {
              err = command_x.build_bridge(our_player, route[i-1], route[i], bridger.bridge)
              if (err) {
                // check whether bridge exists
                sleep()
                local arf = astar_route_finder(wt_road)
                local res_bridge = arf.search_route([route[i-1]], [route[i]])

                if ("routes" in res_bridge  &&  res_bridge.routes.len() == abs(route[i-1].x-route[i].x)+abs(route[i-1].y-route[i].y)+1) {
                  // there is a bridge, continue
                  err = null
                  gui.add_message_at(our_player, "Failed to build bridge from " + coord_to_string(route[i-1]) + " to " + coord_to_string(route[i]) +"\n" + err, route[i])
                } else {
                  remove_wayline(route, (i - 1), way.get_waytype())
                  // remove bridge tiles build by not build bridge

                }
              }
            }

          } else if ( build_route == 0 ) {
          }

        }
        if (err) {
          return { err =  err }
        }
      }
      return { start = route.top(), end = route[0], routes = route, bridge_lens = bridge_tiles, bridge_obj = bridger.bridge, tiles_tree = count_tree }
    }
    print("No route found")
    return { err =  "No route" }
  }
}

/*
 *
 *
 */
function test_select_way(start, end, t_end, wt) {
  //gui.add_message_at(our_player, "start " + coord3d_to_string(start) + " end " + coord3d_to_string(end) + " t_end " + coord3d_to_string(t_end), start)
  local asf = astar_route_finder(wt_rail)
  local wayline = asf.search_route([start], [end])
  if ( "err" in wayline ) {
    //gui.add_message_at(our_player, "no route from " + coord3d_to_string(start) + " to " + coord3d_to_string(end) , start)
  } else {
    //gui.add_message_at(our_player, "exists route from " + coord3d_to_string(start) + " to " + coord3d_to_string(end) , start)
    local toolr = command_x(tool_remove_way)
    toolr.work(our_player, t_end, end, "" + wt)
    return true
  }
  return false
}

/*
 * check ground under bridges for terraform and remove bridge
 *
 * bridges ramp - ramp
 *
 * tiles = array -> way tile - bridge tiles - way tile
 */
function replace_bridge_to_land(tiles) {
  // test_tile_is_empty(tile)

  if ( (tiles[1].is_bridge() && tiles[1].y == tiles[0].y ) ) {
    if ( tiles[1].x > tiles[0].x ) {
      // 31-1 : 3-13
      local tiles_ground = []
      for ( local i = 2; i < tiles.len()-2; i++ ) {
        tiles_ground = square_x(t_tile[i].x, t_tile[i].y).get_ground_tile()
      }

      local terraform_grid_tiles = []
      local terraform_action = null // 0 = down ; 1 = up

      if ( tiles_ground[0].get_slope() == 31 && tiles_ground[1].get_slope() == 1 ) {
        local tile_a1 = square_x(tiles_ground[0].x, tiles_ground[0].y-1).get_ground_tile()
        local tile_a1 = square_x(tiles_ground[1].x, tiles_ground[1].y-1).get_ground_tile()
        local tile_b1 = square_x(tiles_ground[0].x, tiles_ground[0].y+1).get_ground_tile()
        local tile_b2 = square_x(tiles_ground[1].x, tiles_ground[1].y+1).get_ground_tile()


      }



    } else {
    // 13-3 : 1-31



    }
  }




}

/*
 * check ground under bridges
 *
 *
 */
function check_ground(pos_s, pos_e, way) {
  // 0 = off
  // 1 = terraform
  // 2 = ground bridge
  // 3 =
  local print_message_box = 0
  //gui.add_message_at(our_player, "check_ground(pos_s, pos_e) --- " + coord_to_string(pos_s) + " - " + coord_to_string(pos_e), pos_s)

  local check_x = 0
  local check_y = 0
  local f_count = 0
  local t_tile = []
  if ( pos_s.x == pos_e.x ) {
    check_x = 0
    check_y = 1
    if ( pos_s.y > pos_e.y ) {
      f_count = pos_s.y - pos_e.y - 1
      local t = tile_x(pos_e.x + check_x, pos_e.y + check_y, pos_e.z)
      t_tile.append(t)
    } else {
      f_count = pos_e.y - pos_s.y - 1
      local t = tile_x(pos_s.x + check_x, pos_s.y + check_y, pos_s.z)
      t_tile.append(t)
    }
    //gui.add_message_at(our_player, "check_ground(pos_s, pos_e) --- pos_s.x == pos_e.x -> " + coord_to_string(t_tile[0]), t_tile[0])
  } else if ( pos_s.y == pos_e.y ) {
    check_y = 0
    check_x = 1
    if ( pos_s.x > pos_e.x ) {
      f_count = pos_s.x - pos_e.x - 1
      local t = tile_x(pos_e.x + check_x, pos_e.y + check_y, pos_e.z)
      t_tile.append(t)
    } else {
      f_count = pos_e.x - pos_s.x - 1
      local t = tile_x(pos_s.x + check_x, pos_s.y + check_y, pos_s.z)
      t_tile.append(t)
    }
    //gui.add_message_at(our_player, "check_ground(pos_s, pos_e) --- pos_s.y == pos_e.y -> " + coord_to_string(t_tile[0]), t_tile[0])
  }




  for ( local i = 1; i < f_count; i++ ) {
    local t = tile_x(t_tile[i-1].x + check_x, t_tile[i-1].y + check_y, t_tile[i-1].z)
    t_tile.append(t)
  }

  local start_end_slope = 0
  if ( tile_x(pos_s.x, pos_s.y, pos_s.z).get_slope() == 0 && tile_x(pos_e.x, pos_e.y, pos_e.z).get_slope() == 0 ) {
    start_end_slope = 1
  } else if ( tile_x(pos_s.x, pos_s.y, pos_s.z).get_slope() > 0 && tile_x(pos_e.x, pos_e.y, pos_e.z).get_slope() == 0 ) {
    start_end_slope = 1
  } else if ( tile_x(pos_s.x, pos_s.y, pos_s.z).get_slope() == 0 && tile_x(pos_e.x, pos_e.y, pos_e.z).get_slope() > 0 ) {
    start_end_slope = 1
  }

  if (print_message_box == 2) gui.add_message_at(our_player, "(800) start_end_slope " + start_end_slope, world.get_time())

  local z = null
  local z1 = null
  local terraform_tiles = []
  local terraform_grid_tiles = []
  local grid_coord = []
  local err = null

  if ( start_end_slope == 1 && pos_s.z == pos_e.z && f_count > 1 ) {
    for ( local i = 0; i < f_count; i++ ) {
      // find z coord
      z = square_x(t_tile[i].x, t_tile[i].y).get_ground_tile()
      if ( (i + 1) == f_count ) {
        z1 = square_x(t_tile[i-1].x, t_tile[i-1].y).get_ground_tile()
      } else {
        z1 = square_x(t_tile[i+1].x, t_tile[i+1].y).get_ground_tile()
      }

      if (print_message_box == 2) gui.add_message_at(our_player, "check_ground bridge - tile_tree = " + tile_tree + " || tile_groundobj = " + tile_groundobj + " tile_moving_object = " + tile_moving_object, z)

      if ( !z.is_ground() ) {
        // tile is water
        return true
      } else if ( !test_tile_is_empty(z) ) {
        // tiles not free -> build bridge
        //gui.add_message_at(our_player, "check_ground bridge - !z.is_empty() = " + !z.is_empty() + " || !z.is_ground() = " + !z.is_ground() + " tile = " + coord_to_string(z), z)
        return true
      } else if ( ((pos_s.z-2) == z.z || (pos_s.z-1) == z.z) && z.get_slope() > 0 ) {
        // tiles free no build bridges -> terraform
        terraform_tiles.append(z)

        if (print_message_box == 2) gui.add_message_at(our_player, "(832) check_ground bridge - terraform_tiles.append(z) " + coord3d_to_string(z), z)
        //gui.add_message_at(our_player, "(832) z.get_slope() " + z.get_slope(), z)
        // EW slope 37-39 : 13-31 : 31-1 : 3-13
        // NS slope 13-39 : 31-37
        local z_slope_nr = z.get_slope()
        local z1_slope_nr = z1.get_slope()
        local grid_tile = null

        if ( ((z_slope_nr == 39 || z_slope_nr == 31) && z1_slope_nr == 37) || (z_slope_nr == 37 && (z1_slope_nr == 39 || z1_slope_nr == 31)) ) {
          if ( z1_slope_nr == 39 ) {
            grid_tile = tile_x(z1.x, z1.y+1, z1.z)
          } else if ( z_slope_nr == 39 ) {
            grid_tile = tile_x(z.x, z.y+1, z.z)
          }
          if ( z1_slope_nr == 31 ) {
            grid_tile = tile_x(z1.x+1, z1.y, z1.z)
          } else if ( z_slope_nr == 31 ) {
            grid_tile = tile_x(z.x+1, z.y, z.z)
          }
        }
        if ( (z_slope_nr == 13 && (z1_slope_nr == 31 || z1_slope_nr == 39)) || ((z_slope_nr == 31 || z_slope_nr == 39) && z1_slope_nr == 13) ) {
          if ( z1_slope_nr == 13 ) {
            grid_tile = tile_x(z1.x, z1.y, z1.z)
          } else {
            grid_tile = tile_x(z.x, z.y, z.z)
          }
        }

        if ( (z_slope_nr == 1 && z1_slope_nr == 31) || (z_slope_nr == 31 && z1_slope_nr == 1) ) {
          if ( z1_slope_nr == 1 ) {
            grid_tile = tile_x(z1.x, z1.y, z1.z)
          } else {
            grid_tile = tile_x(z.x, z.y, z.z)
          }
        }

        if (print_message_box == 3) gui.add_message_at(our_player, "(862) grid_coord.find(coord3d_to_string(grid_tile)) " + grid_coord.find(coord3d_to_string(grid_tile)), grid_tile)
        if ( grid_tile != null && grid_coord.find(coord3d_to_string(grid_tile)) == null ) {
          terraform_grid_tiles.append(grid_tile)
          grid_coord.append(coord3d_to_string(grid_tile))
          if (print_message_box == 3) gui.add_message_at(our_player, "(862) check_ground bridge - terraform_grid_tiles.append(grid_tile) " + coord3d_to_string(grid_tile), grid_tile)
        }

/*
        if ( (z_slope_nr == 3 && z1_slope_nr == 9) || (z_slope_nr == 9 && z1_slope_nr == 3) ) {
          if ( z1_slope_nr == 9 ) {
            terraform_grid_tiles.append(tile_x(z1.x+1, z1.y, z1.z))
          } else {
            terraform_grid_tiles.append(tile_x(z.x+1, z.y, z.z))
          }
        }
*/
      }
    }

    local terraform_action = 1 // 0 = down ; 1 = up

    if ( terraform_tiles.len() > 0 && terraform_tiles.len() < 7 ) {
      local terraform_field = false
      if ( terraform_grid_tiles.len() > 0 ) {
        if (print_message_box == 3) gui.add_message_at(our_player, "(873) terraform_grid_tiles.len() " + terraform_grid_tiles.len(), world.get_time())
        for ( local i = 0; i < terraform_grid_tiles.len(); i++ ) {
          if ( pos_s.z > terraform_grid_tiles[i].z ) {
            err = command_x.grid_raise(our_player, coord3d(terraform_grid_tiles[i].x, terraform_grid_tiles[i].y, terraform_grid_tiles[i].z))
          } else {
            err = command_x.grid_lower(our_player, coord3d(terraform_grid_tiles[i].x, terraform_grid_tiles[i].y, terraform_grid_tiles[i].z))
          }


          if ( err != null ) {
            terraform_field = true
          }
        }

      }

      if ( terraform_field ) {
        for ( local i = 0; i < terraform_tiles.len(); i++ ) {
          local f = square_x(terraform_tiles[i].x, terraform_tiles[i].y).get_ground_tile()
          if ( f.z == pos_s.z ) { continue }
          do {
            f = square_x(terraform_tiles[i].x, terraform_tiles[i].y).get_ground_tile()
            err = command_x.set_slope(our_player, f, 82)

            if ( err != null ) { return false }
            f = square_x(terraform_tiles[i].x, terraform_tiles[i].y).get_ground_tile()
          } while(f.z < pos_s.z )
        }

      }

      err = command_x.build_way(our_player, tile_x(pos_s.x, pos_s.y, pos_s.z), tile_x(pos_e.x, pos_e.y, pos_e.z), way, true)
      if ( err != null ) { return true }
      return false

    } else {
      return true
    }

  } else if ( start_end_slope == 1 && ((pos_s.z-1) == pos_e.z || (pos_s.z+1) == pos_e.z) && f_count < 5 ) {
    for ( local i = 0; i < f_count; i++ ) {
      // find z coord
      z = square_x(t_tile[i].x, t_tile[i].y).get_ground_tile()

      if (print_message_box == 2) gui.add_message_at(our_player, "(990) check_ground bridge - " + coord_to_string(z), z)

      if ( !z.is_ground() ) {
        // tile is water
        return true
      } else if ( !test_tile_is_empty(z) ) {
        // tiles not free -> build bridge
        if (print_message_box == 2) gui.add_message_at(our_player, "(997) check_ground bridge - !z.is_empty() = " + !z.is_empty() + " || !z.is_ground() = " + !z.is_ground() + " tile = " + coord_to_string(z), z)
        return true
      } else if ( ((pos_s.z+1) == z.z || (pos_s.z-1) == z.z || (pos_s.z) == z.z) && z.get_slope() > 0 ) {
        // tiles free no build bridges -> terraform
        terraform_tiles.append(z)
        if (print_message_box == 2) gui.add_message_at(our_player, "(1002) check_ground bridge - terraform_tiles.append(z) " + coord_to_string(z), z)
      }
    }

    if (print_message_box == 1) {
      local f = null
      for ( local i = 0; i < terraform_tiles.len(); i++ ) {
        gui.add_message_at(our_player, "(1009) ---=> terraform_tiles[" + i + "] tile : " + coord3d_to_string(terraform_tiles[i]), world.get_time())
      }
    }
    /*
      slope up 83
      slope down 82
      slope ramp double high up high 1
        n 4
        s 36
        e 28
        w 12
      slope ramp double high up high 2
        n 8
        s 72
        e 56
        w 24
    */
    if ( ((pos_s.z-1) == pos_e.z || (pos_s.z+1) == pos_e.z) && terraform_tiles.len() > 0 ) {
      if ( pos_s.is_bridge() ) {
        err = remove_tile_to_empty(pos_s, way.get_waytype(), 0)
      }

      if ( terraform_tiles[0].z == pos_s.z && pos_s.z > pos_e.z && terraform_tiles.len() > 1 ) {
        terraform_tiles.reverse()
      } else if ( terraform_tiles[0].z < pos_s.z && pos_s.z > pos_e.z && terraform_tiles.len() > 1 ) {
        terraform_tiles.reverse()
      } else if ( terraform_tiles[0].z == pos_s.z && pos_s.z < pos_e.z && pos_s.x > pos_e.x && terraform_tiles.len() > 1 ) {
        terraform_tiles.reverse()
      }

      local slope_id = 0
      if ( pos_s.x == pos_e.x && pos_s.y < terraform_tiles[0].y ) {
        if (print_message_box == 1) gui.add_message_at(our_player, " ---=> (897) tile : " + coord3d_to_string(terraform_tiles[0]), world.get_time())
        if ( terraform_tiles[0].z == pos_s.z ) {
          slope_id = 4
        } else {
          slope_id = 36
        }
      } else if ( pos_s.x == pos_e.x && pos_s.y > terraform_tiles[0].y ) {
        if (print_message_box == 1) gui.add_message_at(our_player, " ---=> (900) tile : " + coord3d_to_string(terraform_tiles[0]), world.get_time())
        if ( terraform_tiles[0].z == pos_s.z ) {
          slope_id = 36
        } else {
          slope_id = 4
        }
      } else if ( pos_s.y == pos_e.y && pos_s.x < terraform_tiles[0].x ) {
        if (print_message_box == 1) gui.add_message_at(our_player, " ---=> (903) tile : " + coord3d_to_string(terraform_tiles[0]), world.get_time())
        if ( terraform_tiles[0].z == pos_s.z ) {
          slope_id = 12
        } else {
          slope_id = 28
        }
      } else if ( pos_s.y == pos_e.y && pos_s.x > terraform_tiles[0].x ) {
        if (print_message_box == 1) gui.add_message_at(our_player, " ---=> (906) tile : " + coord3d_to_string(terraform_tiles[0]), world.get_time())
        if ( terraform_tiles[0].z == pos_s.z ) {
          slope_id = 28
        } else {
          slope_id = 12
        }
      }

      if ( slope_id > 0 ) {
        err = command_x.set_slope(our_player, terraform_tiles[0], slope_id)
        if ( terraform_tiles.len() > 1 ) {
          err = command_x.set_slope(our_player, terraform_tiles[1], 82)
        }
      }

      err = command_x.build_way(our_player, tile_x(pos_s.x, pos_s.y, pos_s.z), tile_x(pos_e.x, pos_e.y, pos_e.z), way, true)
      if ( err == null ) {
        return false
      }

    //} else if ( (pos_s.z+1) == pos_e.z && terraform_tiles.len() > 0 ) {

    }
    return true
  }else {
    return true
  }
}

/**
 *  check tile end of station
 *
 *  direction = 1, 2, 4, 8
 *  count
 *  s_tile    = station tile : tile_x
 *
 *  return tile_x or null
 */
function check_tile_end_of_station(direction, count, s_tile) {

  local tx = 0
  local ty = 0
  if ( direction == 1 || direction == 4 || direction == 5 ) {
    ty = count
  } else if ( direction == 2 || direction == 8 || direction == 10 ) {
    tx = count
  }

  local t_square = null
  switch(direction) {
    case 1:
      t_square = square_x(s_tile.x, s_tile.y + ty)
      break
    case 2:
      t_square = square_x(s_tile.x - tx, s_tile.y)
      break
    case 4:
      t_square = square_x(s_tile.x, s_tile.y - ty)
      break
    case 8:
      t_square = square_x(s_tile.x + tx, s_tile.y)
      break
    case 5:
      t_square = square_x(s_tile.x, s_tile.y - ty)
      break
    case 10:
      t_square = square_x(s_tile.x + tx, s_tile.y)
      break
  }
  local tile_coord = coord3d_to_string(s_tile) // debug
  local t_ground = null
  // tile out of map
  if ( !world.is_coord_valid(t_square) ) {
    return null
  }
  t_ground = t_square.get_ground_tile()
  if ( test_tile_is_empty(t_ground) && t_ground.z > (s_tile.z - 2) && t_ground.z < (s_tile.z + 2) ) { //t_ground.z > s_tile.z
    return t_ground
  }

  return null
}

/**
 * terraform tile
 *
 * tile       = tile_x
 * ref_hight  = target height
 *
 */
function terraform_tile(tile, ref_hight) {

  // messages set 2
  local print_message_box = 0
  local pl = our_player

  if ( print_message_box > 0 ) {
    if ( debug ) ::debug.set_pause_on_error(true)
    gui.add_message_at(our_player, " ---=> terraform_tile(tile, ref_hight) tile : " + coord3d_to_string(tile) + " target hight : " + ref_hight, world.get_time())
  }

  if ( test_tile_is_empty(tile) && ( tile.get_slope() > 0 || tile.z != ref_hight ) ) {
    local err = null
        if ( print_message_box == 2 ) {
           gui.add_message_at(our_player, " ---=> terraform", world.get_time())
           gui.add_message_at(our_player, " ---=> tile z " + tile.z + " to ref_hight " + ref_hight, world.get_time())
        }

        if ( tile.z < ref_hight && tile.z >= (ref_hight - 2) ) {
        // terraform up
          if ( print_message_box == 2 ) {
            gui.add_message_at(our_player, " ---=> tile up to flat ", world.get_time())
          }
          do {
            err = command_x.set_slope(our_player, tile, 82 )
            if ( err != null ) { break }
            //z = square_x(tile.x, tile.y).get_ground_tile()
          } while(tile.z < ref_hight )

        } else if ( tile.z >= ref_hight || tile.z <= (ref_hight + 1) ) {
           // terraform down
          if ( print_message_box == 2 ) {
            gui.add_message_at(our_player, " ---=> tile down to flat ", world.get_time())
          }
          do {
            err = command_x.set_slope(pl, tile, 83 )
            if ( err != null ) { break }
            //z = square_x(fields[i].x, fields[i].y).get_ground_tile()
          } while(tile.z > ref_hight )
          // replace water to land
          if ( tile.is_water() ) { command_x.change_climate_at(our_player, tile, cl_temperate) }

        }
        if ( err ) {
          return false
        }
        return true
  } else if ( test_tile_is_empty(tile) && ( tile.get_slope() == 0 || tile.z == ref_hight ) ) {
    return true
  }

  return false
}


/**
 *
 *
 */
function set_marker(pos) {
  local m_tile = null
  if (type(pos) == "array") {
    m_tile = pos[0]
  } else {
    m_tile = pos
  }

  local tile = square_x(m_tile.x, m_tile.y).get_ground_tile()
  local tool = command_x(tool_set_marker)
  tool.work(our_player, tile)
}

/**
 * Helper class to remove a field at a factory.
 * Used if no empty spot is available to place a station.
 */
function remove_field(pos)
{
  local tile = square_x(pos.x, pos.y).get_ground_tile()
  local tool = command_x(tool_remover)
  while(tile.find_object(mo_field)) {
    tool.work(our_player, pos)
  }
}

/**
 * remove way line
 * route  = way line
 * pos    = last field to build
 * wt     = waytype
 */
function remove_wayline(route, pos, wt, st_len = null) {
  local tool = command_x(tool_remover)
  local toolr = command_x(tool_remove_way)
  //while(tile.find_object(mo_field)) {
  //  tool.work(our_player, pos)
  //}
  local l = 0
  if ( st_len == null ) {
    st_len = 6
  }

  if ( debug ) ::debug.set_pause_on_error(true)

  local new_route_s = null
  local new_route_e = null

  local i = pos
  local test = 0

  local tile_treeway = false

  local way_cnv_count = 0

  for ( i; i >= 0; i-- ) {
    l++
    local tile = square_x(route[i].x, route[i].y).get_ground_tile()
    local next_tile = null
    if ( i > 0 ) {
      next_tile = square_x(route[i-1].x, route[i-1].y).get_ground_tile()
    }

    local no_bridge = false
    if ( i > 0 && ( abs(tile.x-next_tile.x) > 1 || abs(tile.y-next_tile.y) > 1 )) {
      if ( tile.find_object(mo_bridge) == null ) {
        no_bridge = true
        gui.add_message_at(our_player, "#1304# remove way by not build bridge " + coord3d_to_string(tile), next_tile)
      }
    }

    local t_field = tile.find_object(mo_way)
    if ( t_field == null ) { continue }
    local cnv_count = t_field.get_convoys_passed()[0] + t_field.get_convoys_passed()[1]
    // test field has way
    if ( t_field != null && t_field.get_waytype() == wt ) {
      // test direction next tile
      // break by direction 7, 11, 13, 14, 15

      if ( i > 0 ) {
        if (dir.is_threeway(next_tile.get_way_dirs(wt))  ||  t_field.get_owner().nr == 1) {
          test = 1
          tile_treeway = dir.is_threeway(next_tile.get_way_dirs(wt))
          gui.add_message_at(our_player, "(1320) dir.is_threeway(next_tile.get_way_dirs(wt) " + dir.is_threeway(next_tile.get_way_dirs(wt)), next_tile)
          //::debug.pause()
        }
      }

      if ( tile.find_object(mo_building) != null ) {
        // no remove station
        if ( l < st_len ) { continue }
        test = 1
      } else if ( wt == wt_road ) {
          local test_way = tile.find_object(mo_way) //.get_desc()
          //local tile_coord = coord3d_to_string(tile)
          if ( test_way.get_owner().nr == our_player_nr ) {
            // remove player road from tile
            // not remove public player road from tile
            if ( (t_field.get_convoys_passed()[0] + t_field.get_convoys_passed()[1]) == 0 ) {
              tool.work(our_player, tile)
            }
          } else {
            // break public way ( road )
            test = 1
          }
      } else { // if ( test == 0 ) {
        if ( l < st_len ) { way_cnv_count = t_field.get_convoys_passed()[0] + t_field.get_convoys_passed()[1] }
        // remove way from tile
        if ( cnv_count == way_cnv_count ) {
          //::debug.pause()
          tool.work(our_player, tile)

        }
      }
    /*} else if ( wt == wt_rail && dir.is_threeway(tile.get_way_dirs(wt)) ) {
      gui.add_message_at(our_player, "dir.is_threeway(next_tile.get_way_dirs(wt) " + dir.is_threeway(next_tile.get_way_dirs(wt)), next_tile)
      test = 1
      */
      // no remove tile
    } else if ( tile.find_object(mo_crossing) != null ) {
      // test crossing and remove
      local t_field = tile.get_way(wt)
      way_cnv_count = t_field.get_convoys_passed()[0] + t_field.get_convoys_passed()[1]
      //::debug.pause()

      if ( way_cnv_count == 0 ) {
        toolr.work(our_player, tile, tile, "" + wt)
      }
      //tool.work(our_player, tile)
    }
    // break by direction 7, 11, 13, 14, 15 or owner public player next tile
    if ( test == 1 ) {
      //::debug.pause()
      new_route_s = i - 1
      i--
      break
    }
  }

  if ( test == 1 ) {
    //test = 0
    for ( local j = 0; j < i; j++ ) {
      test = 1
      local tile = square_x(route[j].x, route[j].y).get_ground_tile()
      local next_tile = null
      if ( j < i ) {
        next_tile = square_x(route[j+1].x, route[j+1].y).get_ground_tile()
      }
      local t_field = tile.find_object(mo_way)
      // test field has way
      if ( t_field != null && t_field.get_waytype() == wt ) {
        // test direction: if in  [7, 11, 13, 14, 15] then dir.is_threeway will find this
        //local tile_treeway = dir.is_threeway(next_tile.get_way_dirs(wt))

        if ( tile.find_object(mo_building) != null ) {
          // no remove station
          if ( j < st_len ) { continue }
          test += 1
        } else if ( wt == wt_road ) {
          //local tile_coord = coord3d_to_string(tile)
          if ( t_field.get_owner().nr == our_player_nr ) {
            // remove player road from tile
            // not remove public player road from tile
            if ( (t_field.get_convoys_passed()[0] + t_field.get_convoys_passed()[1]) == 0 ) {
              toolr.work(our_player, tile, tile, "" + wt_road)
            }
          } else {
            // break public way ( road )
            test += 1
          }
        /*} else if ( wt == wt_rail && dir.is_threeway(tile.get_way_dirs(wt)) ) {
          test += 1
          ::debug.pause()*/
          // no remove tile
        } else {
          // remove way from tile
          local cnv_count = t_field.get_convoys_passed()[0] + t_field.get_convoys_passed()[1]
          if ( cnv_count == way_cnv_count && !tile_treeway ) {
            toolr.work(our_player, tile, tile, "" + wt_rail)
          } else {

          }
        }
      }

      if ( tile.is_crossing() ) {
        //::debug.pause()
        // test crossing and remove
        local t_field = tile.get_way(wt)
        way_cnv_count = t_field.get_convoys_passed()[0] + t_field.get_convoys_passed()[1]
        //gui.add_message_at(our_player, "(1133) crossing way_cnv_count " + way_cnv_count, tile)
        //::debug.pause()
        if ( way_cnv_count == 0 ) {
          toolr.work(our_player, tile, tile, "" + wt)
        }
        //tool.work(our_player, tile)
      }
      // break by direction 7, 11, 13, 14, 15
      if ( test >= 2 ) {
        //::debug.pause()
        new_route_e = j + 1
        break
      }
    }
  }

  // check tile pos to empty
  local tile = square_x(route[pos].x, route[pos].y).get_ground_tile()
  if ( tile.find_object(mo_way) != null && wt == wt_rail && test == 0 ) {
    tool.work(our_player, tile)
  }


  local route_status = ""
  local new_route = []
  if ( new_route_s != null && new_route_e != null ) {
    new_route = route.slice(new_route_e, new_route_s)
    route_status = coord_to_string(new_route[0]) + " to " + coord_to_string(new_route[new_route.len()-1])

  }

  if ( test == 0 ) {
    //gui.add_message_at(our_player, "removed way from " + coord_to_string(route[pos]) + " to " + coord_to_string(route[0]), route[0])
  } else {
    //gui.add_message_at(our_player, "removed way not all " + route_status, route[0])
    //optimize_way_line(new_route, wt)
  }

}

/**
 * remove station / tiles remove until it is empty
 * fields = field list
 * wt     = waytype
 */
function remove_tile_to_empty(tiles, wt, t_array = 1) {
  local tool = command_x(tool_remover)
  local toolr = command_x(tool_remove_way)

  if ( t_array == 1 ) {
    // remove array tiles
    for ( local i = 0; i < tiles.len(); i++ ) {
      local tile_remove = 1

      local tiles_r = square_x(tiles[i].x, tiles[i].y).get_ground_tile()
      local test_way = tiles_r.find_object(mo_way) //.get_desc()
      local tile_coord = coord3d_to_string(tiles_r)
      if ( test_way != null ) {
        if ( test_way.get_owner().nr != our_player_nr ) {
          tile_remove = 0
        }
      }

      if ( tile_remove == 1 ) {
        //gui.add_message_at(our_player, "remove tile " + coord3d_to_string(tiles[i]), tiles[i])
        if ( tiles_r.is_crossing() ) {
          //::debug.pause()
          // test crossing and remove
          toolr.work(our_player, tiles_r, tiles_r, "" + wt)
          //tool.work(our_player, tile)
        } else {
          while(true){
            tool.work(our_player, tiles_r)
            if (tiles_r.is_empty())
              break
          }
        //}
        }
      }
    }
  } else if ( t_array == 0 ) {
    // remove one tile
    local tile_remove = 1
    local tiles_r = tile_x(tiles.x, tiles.y, tiles.z)
    local test_way = tiles_r.has_way(wt) //.get_desc()
    //gui.add_message_at(our_player, "test way tile " + tiles_r.has_way(wt), tiles)
    //gui.add_message_at(our_player, "crossing_tile " + tiles_r.is_crossing(), tiles)
    //gui.add_message_at(our_player, "tiles_r.find_object(mo_way).get_owner().nr " + tiles_r.find_object(mo_way).get_owner().nr, tiles)
    if ( test_way && tiles_r.find_object(mo_way).get_owner().nr != our_player_nr && !tiles_r.is_crossing() ) {
      tile_remove = 0
    }
    if ( tile_remove == 1 ) {
      //gui.add_message_at(our_player, "crossing_tile " + tiles_r.is_crossing(), tiles)
        if ( tiles_r.is_crossing() ) {
          //::debug.pause()
          // test crossing and remove
          toolr.work(our_player, tiles_r, tiles_r, "" + wt)
          //tool.work(our_player, tile)
        } else {
          while(true){
          tool.work(our_player, tiles_r)
          if (tiles_r.is_empty())
            break
          }
        }
      return true
    } else {
      return false
    }
  }

}

/**
 * function for check station lenght
 *
 * pl             = player
 * starts_field   = tile station from plan_simple_connection
 * st_lenght      = stations fields count
 * wt             = waytype
 * select_station = station object
 * build          = 0 -> test ; 1 -> build
 * combined_halt  = true -> yes ; false -> no
 *
 * returns false (something failed) or array of station tiles (success)
 * in case of success, the value of starts_field maybe changed
 *
 */
function check_station(pl, starts_field, st_lenght, wt, select_station, build = 1, combined_halt = false) {

    // print messages box
    // 1
    // 2
    local print_message_box = 0

    if ( print_message_box == 2 ) {
      gui.add_message_at(pl, " --- start field : " + coord3d_to_string(starts_field) + "  # station lenght : " + st_lenght, world.get_time())
    }

    local st_build = false
    local err = null

    local t = tile_x(starts_field.x, starts_field.y, starts_field.z)
    local d = t.get_way_dirs(wt)
    d = dir.backward(d) // now d points outside of the built route
    // otherwise stations on very short tracks overlap

    if ( print_message_box == 2 ) {
      gui.add_message_at(pl, " --- field test : " + coord3d_to_string(starts_field), world.get_time())
      gui.add_message_at(pl, " ------ get_way_dirs : " + d, world.get_time())
    }

    // test directions
    local d_quer    = (d == dir.east || d == dir.west) ? dir.north : dir.east;
    local test_dirs = [d, d_quer, dir.backward(d_quer) ]


    // first check direction d and backwards
    // then both orthogonal directions
    for(local i = 0; i<test_dirs.len(); i++)
    {
      local b_tile = [] // station fields array
      local step = 1 // if positive go in direction d, otherwise go backwards
      local step_end = 0 // (positive) steps to the end of the station

      local current_d = test_dirs[i]
      local dc = dir.to_coord(current_d)

      if (i == 0) {
        // append starting tile if searching in direction d
        b_tile.append(t)
      }

      while (b_tile.len() < st_lenght) {

        local b1_tile = tile_x(starts_field.x + step * dc.x, starts_field.y + step * dc.y, starts_field.z)
        if ( print_message_box == 2 ) {
          gui.add_message_at(pl, " ---> test : " + coord3d_to_string(b1_tile), world.get_time())
          gui.add_message_at(pl, " ---=> dir.double(d) : " + dir.double(current_d), world.get_time())
        }

        if ( test_field(pl, b1_tile, wt, dir.double(current_d), starts_field.z) &&  test_tile_is_empty(b1_tile)) {
          if ( print_message_box == 2 ) {
            gui.add_message_at(pl, " ---=> add tile : " + coord3d_to_string(b1_tile), world.get_time())
          }
          b_tile.append(b1_tile)

          // increase step: if positive add +1, if negative add -1
          step = step>0 ? step+1 : step-1
          // also increase steps to end of station
          if (step>0) {
            step_end ++;
          }
        }
        else {
          if (step < 0) {
            // not enough space, fail
            break
          }
          if (i>0) {
            // do not search for backward directions if orthogonal to d
            break
          }
          // search in forward direction failed, now go backward
          step = -1
        }
      }

      // build station
      if ( b_tile.len() == st_lenght && build == 1) {
        // this will build station, missing ways, do terraform
        st_build = expand_station(pl, b_tile, wt, select_station, starts_field, combined_halt)
      }
      else if ( b_tile.len() == st_lenght && build == 0 ) {
        st_build = true
      }

      // correct first tile of station
      // (this will correct c_start/c_end if these are used in the call to this method)
      if (st_build  &&  step_end > 0 && build == 1) {
        starts_field.x += step_end*dc.x
        starts_field.y += step_end*dc.y
        //gui.add_message_at(pl, " ---> first tile of station reset : " + coord3d_to_string(starts_field), starts_field)
      }
      if (st_build) {
        break // leave for loop to test directions
      }
    }

    if ( st_build == false ) {
      // move station
      if ( print_message_box > 0 ) {
        gui.add_message_at(pl, " *#* ERROR => expand station failed", world.get_time())
        gui.add_message_at(pl, " --- field test : " + coord3d_to_string(starts_field), starts_field)
        gui.add_message_at(pl, " ------ get_way_dirs : " + d, world.get_time())
      }
      //::debug.pause()
    }

    return st_build
}

/**
 * test fields station
 * pl         = player
 * t_tile     = field to test
 * wt         = waytype
 * rotate     = northsouth ( 5 ) or eastwest ( 10 )
 * ref_hight  = z from start field
 * way_exists = 0 -> field expand new station
 *              1 -> field has way for expand exists station
 */
function test_field(pl, t_tile, wt, rotate, ref_hight, way_exists = 0) {

  local print_message_box = 0
  local err = null
  local z = null

  // tile out of map
  if ( !world.is_coord_valid(t_tile) ) {
    return false
  }

  // find z coord
  z = square_x(t_tile.x, t_tile.y).get_ground_tile()

  if ( test_tile_is_empty(t_tile) && t_tile.get_slope() == 0 && ref_hight == z.z && way_exists == 0 ) {
    // tile is empty and is flat
    if ( t_tile.is_ground() ) {
      if ( print_message_box == 2 ) {
        gui.add_message_at(pl, " ---=> tile is empty and is flat " + coord3d_to_string(t_tile), t_tile)
      }
      return true
    }
  } else if ( t_tile.has_way(wt) && !t_tile.has_two_ways() && dir.double(t_tile.get_way_dirs(wt)) == rotate && t_tile.get_slope() == 0 && !t_tile.is_bridge() ) {
    // tile has single way and is flat - no bridge ramp
    if ( print_message_box == 2 ) {
      gui.add_message_at(pl, " ---=> tile has single way and is flat " + coord3d_to_string(t_tile), t_tile)
      gui.add_message_at(pl, " ---=> dir.double(t_tile.get_way_dirs(wt)) == rotate : " + dir.double(t_tile.get_way_dirs(wt)) + " == " + rotate, t_tile)
    }
    return true
  } else if ( t_tile.has_way(wt) && !t_tile.has_two_ways() && dir.double(t_tile.get_way_dirs(wt)) == rotate && t_tile.get_slope() > 0 && t_tile.is_bridge() ) {
    // tile has single way and has bridge start
    if ( print_message_box == 2 ) {
      gui.add_message_at(pl, " ---=> tile has single way and is bridge " + coord3d_to_string(t_tile), t_tile)
    }
    return true
  } else if ( test_tile_is_empty(t_tile) && ( t_tile.get_slope() > 0 || ref_hight != z.z ) && way_exists == 0 ) {
    // terraform
    // return true and terraform befor build station
    if ( print_message_box == 2 ) {
      gui.add_message_at(pl, " ---=> tile terraform befor build station " + coord3d_to_string(t_tile), t_tile)
    }
    return true
  }

  return false
}


/**
 * test tile is empty
 * removed objects for empty tiles: tree, ground_object, moving_object
 *
 */
function test_tile_is_empty(tile) {
  local tile_tree = tile.find_object(mo_tree)
  local tile_groundobj = tile.find_object(mo_groundobj)
  local tile_moving_object = tile.find_object(mo_moving_object)

  //gui.add_message_at(our_player, " ---=> test_tile " + coord3d_to_string(tile) + " | is_empty " + tile.is_empty() + " | tile_tree " + tile_tree + " | tile_groundobj " + tile_groundobj + " | tile_moving_object " + tile_moving_object, tile)

  if ( tile.is_empty() ) {
    return true
  } else if ( tile_tree != null || tile_groundobj != null || tile_moving_object != null ) {
    return true
  }

  return false
}

/**
 * function expand station()
 *
 * pl             = player
 * fields         = array fields
 * wt             = waytype
 * select_station = station object
 * start_fld      = c_start or c_end
 * combined_halt  = true -> yes ; false -> no
 */
function expand_station(pl, fields, wt, select_station, start_fld, combined_halt) {

  local start_field = tile_x(start_fld.x, start_fld.y, start_fld.z)

  // 0 = off
  // 1 = extension build
  // 2 = messages
  // 3 = check dock/harbour
  local print_message_box = 0

  local ref_hight = start_field.z
  local err = null
  local combined_station = false

  // extension field for connect station to factory/dock
  local r = tile_x(start_field.x, start_field.y, start_field.z)
  local d = r.get_way_dirs(wt)
  local extension_tile = null
  switch(d) {
    case 1:
      extension_tile = square_x(start_field.x, start_field.y + 1)
      break
    case 2:
      extension_tile = square_x(start_field.x - 1, start_field.y)
      break
    case 4:
      extension_tile = square_x(start_field.x, start_field.y - 1)
      break
    case 8:
      extension_tile = square_x(start_field.x + 1, start_field.y)
      break
  }
  //gui.add_message_at(pl, "(1496) ---=> extension_tile " + extension_tile, world.get_time())


  if ( extension_tile != null && tile_x(extension_tile.x, extension_tile.y, extension_tile.get_ground_tile().z).is_water() ) {
    extension_tile = null
  }

  local t = fields.len()

  if ( t > 0 ) {
    // terrafom
    local i = 1
    if ( fields[0].x != start_field.x || fields[0].y != start_field.y ) {
      i = 0
    }
    for ( i; i < t; i++ ) {
      local z = square_x(fields[i].x, fields[i].y).get_ground_tile()
      local f = tile_x(fields[i].x, fields[i].y, z.z)

      if ( test_tile_is_empty(f) && ( f.get_slope() > 0 || ref_hight != z.z ) ) {

        if ( print_message_box == 2 ) {
           gui.add_message_at(pl, " ---=> terraform", world.get_time())
           gui.add_message_at(pl, " ---=> tile z " + z.z + " start tile z " + ref_hight, world.get_time())
        }

        if ( z.z < ref_hight && z.z >= (ref_hight - 2) ) {
        // terraform up
          if ( print_message_box == 2 ) {
            gui.add_message_at(pl, " ---=> tile up to flat ", world.get_time())
          }
          do {
            err = command_x.set_slope(pl, tile_x(f.x, f.y, z.z), 82 )
            if ( err != null ) { break }
            z = square_x(fields[i].x, fields[i].y).get_ground_tile()
          } while(z.z < ref_hight )

        } else if ( z.z >= ref_hight || z.z <= (ref_hight + 1) ) {
           // terraform down
          if ( print_message_box == 2 ) {
            gui.add_message_at(pl, " ---=> tile down to flat ", world.get_time())
          }
          do {
            err = command_x.set_slope(pl, tile_x(f.x, f.y, z.z), 83 )
            if ( err != null ) { break }
            z = square_x(fields[i].x, fields[i].y).get_ground_tile()
          } while(z.z > ref_hight )
          // replace water to land
          if ( z.is_water() ) { command_x.change_climate_at(our_player, z, cl_temperate) }

        }
        if ( err ) {
          return false
        }
      }
    }

    // build way to tiles
    for ( local i = 1; i < t; i++ ) {

      if ( test_tile_is_empty(fields[i]) && fields[i].get_slope() == 0 ) {
        err = null
        // empty then build way

        err = command_x.build_way(pl, fields[i-1], fields[i], planned_way, true)
        if ( err != null ) {
          gui.add_message_at(pl, " ---=> not build way tile " + coord3d_to_string(fields[i-1]) + " to " + coord3d_to_string(fields[i]) + " - err :" + err, fields[i])
          remove_tile_to_empty(fields, wt)
          return false
        }
      }
    }

    // check harbour/dock
    local st_dock = search_station(start_field, wt, 1)
    if ( !equal_coord3d( fields[0], start_field) ) {
      local extension = find_extension(wt)
      if ( extension ) {
        if ( print_message_box == 2 ) {
          gui.add_message_at(our_player, "-*---> selectet extension: " + extension.get_name(), world.get_time())
        }
        if ( st_dock && extension_tile != null ) {
          if ( print_message_box == 2 ) {
            gui.add_message_at(our_player, "-*---> dock/harbour found at : " + coord3d_to_string(st_dock[0]), st_dock[0])
          }
          //gui.add_message_at(pl, "(1596) ---=> abs(fields[0].x - st_dock[0].x) " + abs(fields[0].x - st_dock[0].x), fields[0])
          //gui.add_message_at(pl, "(1596) ---=> abs(fields[0].y - st_dock[0].y) " + abs(fields[0].y - st_dock[0].y), fields[0])
          if ( abs(fields[0].x - st_dock[0].x) > 1 || abs(fields[0].y - st_dock[0].y) > 1 ) {
            local tile = tile_x(extension_tile.x, extension_tile.y, extension_tile.get_ground_tile().z)
            if ( test_tile_is_empty(tile) ) {
              if ( print_message_box == 2 ) {
                gui.add_message_at(our_player, "-*---> build extension at : " + coord3d_to_string(tile), world.get_time())
              }
              err = command_x.build_station(pl, tile, extension)
            }
          }

        } else {
          err = command_x.build_station(pl, start_field, select_station)
        }
      }
    }
    if ( st_dock ) { //&& st_dock[0].find_object(mo_building).get_waytype() == wt_water
      local st_dock_wt = st_dock[0].find_object(mo_building).get_desc().get_waytype()

      if ( print_message_box == 3 ) {
        gui.add_message_at(pl, "(1593) ---=> st_dock " + st_dock, st_dock[0])
        local b = tile_x(st_dock[0].x, st_dock[0].y, st_dock[0].z).find_object(mo_building)
        gui.add_message_at(pl, "(1593) ---=> b " + b, st_dock[0])
        gui.add_message_at(pl, "(1593) ---=> b.get_desc().get_waytype() " + b.get_desc().get_waytype(), st_dock[0])
        gui.add_message_at(pl, "(1593) ---=> b.get_desc().get_name() " + b.get_desc().get_name(), st_dock[0])
        ::debug.pause()
      }
      if ( st_dock_wt == wt_water ) {
        combined_station = true
      }
    }

    // station not build to start_field
/*    local build_connection = 0
    if ( combined_station && !equal_coord3d( fields[0], start_field) ) {
        err = command_x.build_station(pl, start_field, select_station)
        build_connection = 1
        // build connect tile to dock
    }*/


    if ( err == null ) {
      // build station to tile
      for ( local i = 0; i < t; i++ ) {
        local z = square_x(fields[i].x, fields[i].y).get_ground_tile()
        local f = tile_x(fields[i].x, fields[i].y, z.z)
        if ( tile_x(fields[i].x, fields[i].y, fields[i].z).is_bridge() && f.get_slope() > 0 ) {
          // bridge start field -> build to ground
          fields[i].z -= 1
        }
        err = command_x.build_station(pl, fields[i], select_station)
        if ( err ) {
          if ( print_message_box > 0 ) {
            gui.add_message_at(pl, "(1615) ---=> not build station tile at " + coord3d_to_string(fields[i]), fields[i])
          }
          if ( equal_coord3d( fields[0], start_field) ) {
            fields.remove(0)
          }
          remove_tile_to_empty(fields, wt_rail)
          return false
        }
        if ( print_message_box == 2 ) {
          gui.add_message_at(pl, " ---=> build station tile at " + coord3d_to_string(fields[i]), fields[i])
        }
      }
    }

    // station not build to start_field
    if ( !equal_coord3d( fields[0], start_field) ) {
      if ( start_field.find_object(mo_building) ) {
        if ( print_message_box == 2 ) {
          gui.add_message_at(pl, "(1756) ---=> combined_station " + combined_station, fields[0])
        }
        // remove connect tile to dock
        local tool = command_x(tool_remover)
        tool.work(our_player, start_field)
      }
      // connect way to station
      err = command_x.build_way(pl, start_field, fields[0], planned_way, true)
    }


    // check station connect factory
    local st = halt_x.get_halt(fields[0], pl)
    local s_tiles = []
/*
        gui.add_message_at(our_player, "(1927) -*---> halt_x.get_halt(fields[0], pl) : " + st, fields[0])
        gui.add_message_at(our_player, "(1927) -*---> combined_station : " + combined_station, fields[0])
        gui.add_message_at(our_player, "(1927) -*---> combined_halt : " + combined_halt, fields[0])
        gui.add_message_at(our_player, "(1927) -*---> extension_tile : " + extension_tile, fields[0])
        gui.add_message_at(our_player, "(1927) -*---> st.get_factory_list().len() : " + st.get_factory_list().len(), fields[0])
*/
    if ( st != null && st.get_factory_list().len() == 0 && combined_halt == false ) {
      local fl_st = st.get_factory_list()
      if ( combined_station == false && fl_st.len() == 0 && extension_tile != null ) {
        local tile = tile_x(extension_tile.x, extension_tile.y, extension_tile.get_ground_tile().z)
        if ( print_message_box == 1 ) {
          gui.add_message_at(our_player, "-*---> build extension at : " + coord3d_to_string(tile), tile)
        }
        local extension = find_extension(wt)
        local new_tile = 0
        if ( test_tile_is_empty(tile) ) {
          new_tile = 1

        } else if ( tile.has_way(wt_road) && !tile.has_two_ways() ) {
          local tiles = [1, 2, 4, 5, 8, 10]
          local test = 0
          // test direction from road
          for ( local i = 0; i < tiles.len(); i++ ) {
            if ( tile.get_way_dirs(wt_road) == tiles[i] ) {
              test++
            }
          }
          if ( test == 1 ) {
            local stations_list = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x("Passagiere"))
            err = command_x.build_station(pl, tile, stations_list[0])
            if ( err ) {
              gui.add_message_at(pl, " -##-=> WARNING not connect factory: " + coord3d_to_string(start_field), start_field)
              new_tile = 1
            }

          } else {
            new_tile = 1
          }

        } else {
          new_tile = 1
        }

        if ( new_tile == 1 ) {
          // to do connection factory other rotations
          if ( print_message_box == 1 ) {
            gui.add_message_at(pl, "check connect factory : build extension", start_field)
          }
          //::debug.pause()
            s_tiles.append(fields[0])
            s_tiles.append(start_field)
          local tool = command_x(tool_remover)
          local test_tiles = []
          local factory_connect = 0
          if ( start_field.get_way_dirs(wt_rail) == 3 ) {
            if ( start_field.x == fields[0].x ) {
              s_tiles.append(square_x(start_field.x+1, start_field.y).get_ground_tile())

              test_tiles.append(square_x(start_field.x-1, start_field.y).get_ground_tile())
              test_tiles.append(square_x(start_field.x-1, start_field.y+1).get_ground_tile())
              test_tiles.append(square_x(start_field.x, start_field.y+1).get_ground_tile())

              factory_connect = build_extensions_connect_factory(pl, start_field, fields[0], test_tiles, extension)

            } else if ( start_field.y == fields[0].y ) {
              s_tiles.append(square_x(start_field.x, start_field.y-1).get_ground_tile())

              test_tiles.append(square_x(start_field.x, start_field.y+1).get_ground_tile())
              test_tiles.append(square_x(start_field.x-1, start_field.y+1).get_ground_tile())
              test_tiles.append(square_x(start_field.x-1, start_field.y).get_ground_tile())

              factory_connect = build_extensions_connect_factory(pl, start_field, fields[0], test_tiles, extension)

            }

          } else if ( start_field.get_way_dirs(wt_rail) == 6 ) {
            if ( start_field.x == fields[0].x ) {
              s_tiles.append(square_x(start_field.x-1, start_field.y).get_ground_tile())

              test_tiles.append(square_x(start_field.x-1, start_field.y).get_ground_tile())
              test_tiles.append(square_x(start_field.x-1, start_field.y-1).get_ground_tile())
              test_tiles.append(square_x(start_field.x, start_field.y-1).get_ground_tile())

              factory_connect = build_extensions_connect_factory(pl, start_field, fields[0], test_tiles, extension)

            } else if ( start_field.y == fields[0].y ) {
              s_tiles.append(square_x(start_field.x, start_field.y+1).get_ground_tile())

              test_tiles.append(square_x(start_field.x, start_field.y-1).get_ground_tile())
              test_tiles.append(square_x(start_field.x-1, start_field.y-1).get_ground_tile())
              test_tiles.append(square_x(start_field.x-1, start_field.y).get_ground_tile())

              factory_connect = build_extensions_connect_factory(pl, start_field, fields[0], test_tiles, extension)

            }

          } else if ( start_field.get_way_dirs(wt_rail) == 9 ) {
            if ( start_field.x == fields[0].x ) {
              s_tiles.append(square_x(start_field.x-1, start_field.y).get_ground_tile())

              test_tiles.append(square_x(start_field.x+1, start_field.y).get_ground_tile())
              test_tiles.append(square_x(start_field.x+1, start_field.y+1).get_ground_tile())
              test_tiles.append(square_x(start_field.x, start_field.y+1).get_ground_tile())

              factory_connect = build_extensions_connect_factory(pl, start_field, fields[0], test_tiles, extension)

            } else if ( start_field.y == fields[0].y ) {
              s_tiles.append(square_x(start_field.x, start_field.y-1).get_ground_tile())

              test_tiles.append(square_x(start_field.x, start_field.y+1).get_ground_tile())
              test_tiles.append(square_x(start_field.x+1, start_field.y+1).get_ground_tile())
              test_tiles.append(square_x(start_field.x+1, start_field.y).get_ground_tile())

              factory_connect = build_extensions_connect_factory(pl, start_field, fields[0], test_tiles, extension)

            }

          } else if ( start_field.get_way_dirs(wt_rail) == 12 ) {
            if ( start_field.x == fields[0].x ) {
              s_tiles.append(square_x(start_field.x-1, start_field.y).get_ground_tile())

              test_tiles.append(square_x(start_field.x+1, start_field.y).get_ground_tile())
              test_tiles.append(square_x(start_field.x+1, start_field.y-1).get_ground_tile())
              test_tiles.append(square_x(start_field.x, start_field.y-1).get_ground_tile())

              factory_connect = build_extensions_connect_factory(pl, start_field, fields[0], test_tiles, extension)

            } else if ( start_field.y == fields[0].y ) {
              s_tiles.append(square_x(start_field.x, start_field.y+1).get_ground_tile())

              test_tiles.append(square_x(start_field.x, start_field.y-1).get_ground_tile())
              test_tiles.append(square_x(start_field.x+1, start_field.y-1).get_ground_tile())
              test_tiles.append(square_x(start_field.x+1, start_field.y).get_ground_tile())

              factory_connect = build_extensions_connect_factory(pl, start_field, fields[0], test_tiles, extension)

            }

          }
          /*
          if ( tiles.len() == 3 ) {
            for ( local x = 0; x < 3; x++ ) {
              if ( halt_x.get_halt(tiles[x], pl) ) {
                fields.append(tiles[x])
              }
            }

          }*/
          /*if ( start_field.x < fields[0].x && start_field.y < fields[0].y ) {
            tile = square_x(start_field.x-1, start_field.y).get_ground_tile()
            if ( tile.is_empty ) {
              gui.add_message_at(pl, " --=> new extensions field: " + coord3d_to_string(tile), tile)
              s_tiles.append(fields[0])
              s_tiles.append(start_field)
              if ( start_field.get_way_dirs(wt_rail) == 3 ) {
                s_tiles.append(square_x(start_field.x, start_field.y-1).get_ground_tile())
              } else if ( start_field.get_way_dirs(wt_rail) == 6 ) {
                s_tiles.append(square_x(start_field.x, start_field.y+1).get_ground_tile())
              }
              rebuild = 1
            }
            fields.append(tile)
          } else if ( start_field.x > fields[0].x && start_field.y > fields[0].y ) {
            tile = square_x(start_field.x+1, start_field.y).get_ground_tile()
            if ( tile.is_empty ) {
              gui.add_message_at(pl, " --=> new extensions field: " + coord3d_to_string(tile), tile)
              s_tiles.append(fields[0])
              s_tiles.append(start_field)
              if ( start_field.get_way_dirs(wt_rail) == 9 ) {
                s_tiles.append(square_x(start_field.x, start_field.y-1).get_ground_tile())
              } else if ( start_field.get_way_dirs(wt_rail) == 12 ) {
                s_tiles.append(square_x(start_field.x+1, start_field.y).get_ground_tile())
              }
              rebuild = 1
            }
            fields.append(tile)

          } else {*/
          if ( factory_connect == 0 ) {
            local tile = square_x(fields[0].x, fields[0].y).get_ground_tile()
            if ( tile.find_object(mo_building) != null ) {
              local waytypes = test_halt_waytypes(tile)
              if ( waytypes > 1 || start_field.find_object(mo_building) != null ) {
                if ( print_message_box > 0 ) {
                  gui.add_message_at(pl, " -#-=> combined station " + coord3d_to_string(start_field), start_field)
                }
              } else {
                gui.add_message_at(pl, " -###-=> WARNING not connect factory: " + coord3d_to_string(start_field), start_field)
              }
            }
          }



        }
      }

    } else if ( combined_halt ) {
      // build combined station
    }

    if ( start_field.is_empty() ) {
      // rebuild way
      err = command_x.build_way(pl, s_tiles[0], s_tiles[1], planned_way, true)
      err = command_x.build_way(pl, s_tiles[1], s_tiles[2], planned_way, true)
    }

    return fields
  }
}

/*
 *  function build_extensions_connect_factory()
 *
 *  pl          = player
 *  st_field    = start field
 *  hlt_field   = halt field
 *  tiles[]     = test tiles for extensions to connect factory
 *  extension   = extensions object
 *
 *  return
 *    1 = connect factory
 *    0 = not connect factory
 */
function build_extensions_connect_factory(pl, st_field, hlt_field, tiles, extension) {
  local factory_connect = 0
  // check station connect factory
  local st = halt_x.get_halt(hlt_field, pl)
  local fl_st = null
  local err = null

  local tool = command_x(tool_remover)

  //gui.add_message_at(pl, " -##-=> " + coord3d_to_string(tiles[0]) + " tiles[0].is_empty() " + tiles[0].is_empty() + " - tiles[0].is_water() " + tiles[0].is_water() + " - tiles[0].has_ways() " + tiles[0].has_ways() + " - tiles[0].has_way(wt_water) " + tiles[0].has_way(wt_water), tiles[0])
  //gui.add_message_at(pl, " -##-=> " + coord3d_to_string(tiles[1]) + " tiles[1].is_empty() " + tiles[1].is_empty() + " - tiles[1].is_water() " + tiles[1].is_water(), tiles[1])
  //gui.add_message_at(pl, " -##-=> " + coord3d_to_string(tiles[2]) + " tiles[2].is_empty() " + tiles[2].is_empty() + " - tiles[2].is_water() " + tiles[2].is_water(), tiles[2])

  //gui.add_message_at(pl, " -##-=> " + coord3d_to_string(tiles[0]) + " tiles[0].is_empty() " + tiles[0].is_empty() + " - tiles[0].is_water() " + tiles[0].is_water() + " - tiles[0].has_ways() " + tiles[0].has_ways() + " - tiles[0].has_way(wt_water) " + tiles[0].has_way(wt_water), tiles[0])
  //gui.add_message_at(pl, " -##-=> " + coord3d_to_string(tiles[1]) + " tiles[1].is_empty() " + tiles[1].is_empty() + " - tiles[1].is_water() " + tiles[1].is_water() + " - tiles[1].has_ways() " + tiles[1].has_ways(), tiles[1])
  //gui.add_message_at(pl, " -##-=> " + coord3d_to_string(tiles[2]) + " tiles[2].is_empty() " + tiles[2].is_empty() + " - tiles[2].is_water() " + tiles[2].is_water() + " - tiles[2].has_ways() " + tiles[2].has_ways(), tiles[2])

  // test 1 -> rebuild 0
  for (local i = 0; i < tiles.len(); i++ ) {
    if ( test_tile_is_empty(tiles[i]) && tiles[i].is_ground() ) {
      if ( i > 0 ) {
        tool.work(pl, st_field)
        command_x.build_station(pl, st_field, extension)
      }
      err = command_x.build_station(pl, tiles[i], extension)
      if ( err != null ) {
        gui.add_message_at(pl, " -##-=> build err tile " + i + " " + err, tiles[i])
      }
      fl_st = st.get_factory_list()
      if ( fl_st.len() > 0 ) {
        factory_connect = 1
        if ( i > 0 ) {
          tool.work(pl, st_field)
        }
        break
      }
    } else if ( tiles[i].has_way(wt_water) ) {

      //gui.add_message_at(pl, " -##-=> tiles["+i+"].get_way_dirs(wt_water) " + tiles[i].get_way_dirs(wt_water) + " - tiles["+i+"].find_object(mo_way).get_owner().nr " + tiles[i].find_object(mo_way).get_owner().nr, tiles[i])

      if ( (tiles[i].get_way_dirs(wt_water) == 5 || tiles[i].get_way_dirs(wt_water) == 10 ) && tiles[i].find_object(mo_way).get_owner().nr == 16 ) {

        local station = find_station(wt_water)
        if ( station != false ) {
          err = command_x.build_station(pl, tiles[i], station)
          if ( err != null ) {
            gui.add_message_at(pl, " -##-=> build err tile " + i + " " + err, tiles[i])
          }
          fl_st = st.get_factory_list()
          if ( fl_st.len() > 0 ) {
            factory_connect = 1
            break
          }
        }
      }
    } else if ( (tiles[i].get_way_dirs(wt_road) == 5 || tiles[i].get_way_dirs(wt_road) == 10 ) && (tiles[i].find_object(mo_way).get_owner().nr == our_player.nr || tiles[i].find_object(mo_way).get_owner().nr == 1) ) {

        //gui.add_message_at(pl, " -##-=> build road station tile " + coord3d_to_string(tiles[i]), tiles[i])
        //gui.add_message_at(pl, " -##-=> halt_x.get_halt(st_field, pl) " + halt_x.get_halt(st_field, pl), tiles[i])

        local restor_way = 0

        local station = find_station(wt_road)
        if ( station != false ) {
          if ( halt_x.get_halt(st_field, pl) == null ) {
            tool.work(pl, st_field)
            command_x.build_station(pl, st_field, extension)
            restor_way = 1
          }
          err = command_x.build_station(pl, tiles[i], station)
          if ( err != null ) {
            gui.add_message_at(pl, " -##-=> build err tile " + i + " " + err, tiles[i])
          }
          fl_st = st.get_factory_list()
          if ( fl_st.len() > 0 ) {
            factory_connect = 1
            if (restor_way == 1) {
              tool.work(pl, st_field)
            }
            break
          }
        }
    }

  }

  return factory_connect
}

/*
 *  tiles       = field array
 *  station_obj = building_desc_x.station
 */
function build_station(tiles, station_obj) {

}

/**
  * find signal tool
  *
  * sig_type  = signal type (is_signal, is_presignal ... )
  * wt        = waytype
  */
function find_signal(sig_type, wt) {

  local list = sign_desc_x.get_available_signs(wt)
  local obj_sign = null
  foreach(o in list) {
    if (o.is_signal() && sig_type == "is_signal") {
      obj_sign = o
    }
    if ( o.is_traffic_light() && sig_type == "traffic_light" ) {
      obj_sign = o
    }
  }

  return obj_sign

}

/*
 *
 *
 */
function find_station(wt) {
  local list = building_desc_x.get_available_stations(building_desc_x.station, wt, good_desc_x(freight))

  if ( list.len() > 0 ) {
    return list[0]
  } else {
    return false
  }
}

/**
  * find object tool
  *
  * obj   = object type ( bridge, tunnel, way, catenary )
  * wt    = waytype
  * speed = speed
  */
function find_object(obj, wt, speed) {

  local list = []
  switch(obj) {
    case "bridge":
      list = bridge_desc_x.get_available_bridges(wt)
      break
    case "tunnel":
      list = tunnel_desc_x.get_available_tunnels(wt)
      break
    case "way":
      list = way_desc_x.get_available_ways(wt, st_flat)
      break
    case "catenary":
      local li = wayobj_desc_x.get_available_wayobjs(wt)
      for (local j=0; j<li.len(); j++) {
        if ( li[j].is_overhead_line() ) {
          /*
            pak128.german check catenary in name
          */
          if ( get_set_name() == "pak128.german" ) {
            local name = li[j].get_name().tolower()
            if ( name.find("catenary") != null ) {
              list.append(li[j])
            }
          } else {
            list.append(li[j])
          }
        }
      }
      break
  }


  local len = list.len()

  // sort objects by speed
  {
    local obj_list = []
    for(local i=0; i<len; i++) {
      obj_list.append(list[i].get_topspeed())
    }
    obj_list.sort()

    local sort_obj_list = []

    for(local i=0; i<len; i++) {
      //gui.add_message_at(our_player,i + " obj " + obj_list[i] , world.get_time())
      for (local j=0; j<len; j++) {
        if ( obj_list[i] == list[j].get_topspeed() ) {
          sort_obj_list.append(list[j])
        }
      }
    }
    list.clear()
    list = sort_obj_list.slice(0)
    /*for(local i=0; i<len; i++) {
      gui.add_message_at(our_player,i + " obj " + list[i].get_name() + " speed " + list[i].get_topspeed(), world.get_time())
    }*/
  }

  local max_speed = 160
  local min_len = 5

  local obj_desc = null

  if (len>0) {
    obj_desc = list[0]
    //gui.add_message_at(our_player,"0  obj_desc " + obj_desc.get_name(), world.get_time())

      for(local i=1; i<len; i++) {
        local b = list[i]
        local o = 1
        if ( obj == "bridge" && b.get_max_length() < min_len ) {
          o = 0
        }

        if ( (obj == "way" || obj == "tunnel") && !obj_desc.is_available(world.get_time()) ) {
          o = 0
        }

        if ( obj == "catenary" && !obj_desc.is_available(world.get_time()) && obj_desc.is_overhead_line() ) {
          o = 0
        }

        if ( o == 1 ) {
          if (obj_desc.get_topspeed() <= speed) {
            if ( b.get_topspeed() > obj_desc.get_topspeed() && b.get_topspeed() <= speed ) {
              obj_desc = b
              if ( obj_desc.get_topspeed() == speed ) { break }
            } else {
              if ( obj_desc.get_topspeed() >= speed ) { break }
              obj_desc = b
              //gui.add_message_at(our_player, i + " break obj_desc " + obj_desc.get_name(), world.get_time())
              break
            }
          }
        }
      }
  }

  return obj_desc
}


/**
  * find extension building tool
  *
  *
  */
function find_extension(wt, tile_size = 1) {

  local print_message_box = 0

  local select_extension = null

  // extension building from waytype for selected good
  local extension_list = building_desc_x.get_available_stations(building_desc_x.station_extension, wt, good_desc_x(freight))

  if ( print_message_box == 2 ) {
    gui.add_message_at(our_player, "extension_list " + extension_list.len(), world.get_time())
  }

  if ( extension_list.len() > 0 ) {
    foreach(extension in extension_list) {
      local ok = (select_extension == null)

      if ( print_message_box == 2 ) {
        gui.add_message_at(our_player, "extension " + extension.get_name() + " size " + extension.get_size(0), world.get_time())
      }

      if ( !ok && coord_to_string(extension.get_size(0)) == "1,1" ) {
        if ( select_extension.get_capacity() > extension.get_capacity() ) {
          select_extension = extension
        }
      } else if ( coord_to_string(extension.get_size(0)) == "1,1" ) {
        select_extension = extension
      }
    }
  }

  if ( select_extension == null ) {
    extension_list.clear()
    // not find extension from waytype for selected good
    // search post extension from all waytypes
    extension_list = building_desc_x.get_available_stations(building_desc_x.station_extension, wt_all, good_desc_x(freight))

    if ( print_message_box == 2 ) {
      gui.add_message_at(our_player, "extension_list " + extension_list.len(), world.get_time())
    }

    if ( extension_list.len() > 0 ) {
      foreach(extension in extension_list) {
        local ok = (select_extension == null)

        if ( print_message_box == 2 ) {
          gui.add_message_at(our_player, "extension " + extension.get_name(), world.get_time())
        }

        if ( !ok && coord_to_string(extension.get_size(0)) == "1,1" ) {
          if ( select_extension.get_capacity() > extension.get_capacity() ) {
            select_extension = extension
          }
        } else if ( coord_to_string(extension.get_size(0)) == "1,1" ) {
          select_extension = extension
        }
      }
    }

  }

  if ( select_extension == null ) {
    extension_list.clear()
    // not find extension from waytype for selected good
    // search post extension from all waytypes
    extension_list = building_desc_x.get_available_stations(building_desc_x.station_extension, wt_all, good_desc_x("post"))

    if ( print_message_box == 2 ) {
      gui.add_message_at(our_player, "extension_list " + extension_list.len(), world.get_time())
    }

    if ( extension_list.len() > 0 ) {
      foreach(extension in extension_list) {
        local ok = (select_extension == null)

        if ( print_message_box == 2 ) {
          gui.add_message_at(our_player, "extension " + extension.get_name(), world.get_time())
        }

        if ( !ok && coord_to_string(extension.get_size(0)) == "1,1" ) {
          if ( select_extension.get_capacity() > extension.get_capacity() ) {
            select_extension = extension
          }
        } else if ( coord_to_string(extension.get_size(0)) == "1,1" ) {
          select_extension = extension
        }
      }
    }

  }

  if ( print_message_box == 2 ) {
    gui.add_message_at(our_player, "select_extension " + select_extension, world.get_time())
  }

  return select_extension

}

/**
 * search existing depot on range to station
 *
 *  field_pos = start field
 *  wt        = waytype
 *  range     = search range
 *
 */
function search_depot(field_pos, wt, range = 10) {

  local list_exists_depot = depot_x.get_depot_list(our_player, wt)

  if ( list_exists_depot ) {
    local tile_min = [field_pos.x - range, field_pos.y - range]
    local tile_max = [field_pos.x + range, field_pos.y + range]
    local depot_found = null

    foreach(key in list_exists_depot) {

      if ( key.x >= tile_min[0] && key.y >= tile_min[1] && key.x <= tile_max[0] && key.y <= tile_max[1] ) {
        depot_found = tile_x(key.x, key.y, key.z)
        return depot_found
      }
    }
  }
  return false
}

/**
 * search existing station on range to field
 *
 *  field_pos = start field
 *  wt        = waytype
 *  range     = search range
 *
 */
function search_station(field_pos, wt, range) {

    if ( field_pos == null || type(field_pos) == "array" ) { return false }

    local tile_min = [field_pos.x - range, field_pos.y - range]
    local tile_max = [field_pos.x + range, field_pos.y + range]
    local station_found = false

    foreach(halt in halt_list_x()) {
        if (halt.get_owner().nr == our_player_nr) {  // && halt.get_type == wt
          local tile = halt.get_tile_list()
          if ( tile[0].x >= tile_min[0] && tile[0].y >= tile_min[1] && tile[0].x <= tile_max[0] && tile[0].y <= tile_max[1] ) {
            station_found = tile
            break
          }

        }
    }

    return station_found
}


/**
 * build double way for more convoys to line
 *
 *
 */
function build_double_track(start_field, wt, station_len) {

  // 1
  // 2 - terraform
  // 21 - terraform grid
  // 3 - double track diagonal
  local print_message_box = 0

  if ( print_message_box > 0 ) {
    gui.add_message_at(our_player, " ### build_double_track ### " + coord3d_to_string(start_field), start_field)
    gui.add_message_at(our_player, " ### station_len ### " + station_len, start_field)
  }

  //local way_list = way_desc_x.get_available_ways(wt, st_flat)
  local way_obj = start_field.find_object(mo_way).get_desc() //way_list[0]
  if ( !way_obj.is_available(world.get_time()) ) {
    way_obj = find_object("way", wt, way_obj.get_topspeed())
  }

  local b_player = our_player //way_obj.get_owner()

  local d = start_field.get_way_dirs(wt)

  local tiles = []
  local tiles_build_l = []
  local tiles_build_r = []
  local t = 0
  local way_len = station_len
  if ( way_len < 8 ) {
    way_len = 8
  }
  local diagonal_st = 0
  local way_len_d = 0
  if ( d == 6 || d == 9 ) {
    way_len++
    way_len_d = 1
    diagonal_st = d
  } else if ( d == 3 || d == 12  ) {
    way_len++
    way_len_d = 1
    diagonal_st = d
  }
  local diagonal_x = 0
  local diagonal_y = 0

  local signal = []

  // check way
  // 8 fields straight
  // 11 fields diagonal

  for ( local i = 0; i < (way_len + way_len_d); i++ ) {

    if ( d == 5 ) {
      // build from n to s
      // ns - r
      local ref_ground = square_x(start_field.x, start_field.y + i).get_ground_tile()
      local t = tile_x(start_field.x + 1, start_field.y + i, start_field.z)
      if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(start_field.x, start_field.y + i).get_ground_tile().z == ref_ground.z
        tiles_build_r.append(tile_x(start_field.x + 1, start_field.y + i, ref_ground.z))
      }
      // ns - l
      t = tile_x(start_field.x - 1, start_field.y + i, start_field.z)
      if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(start_field.x, start_field.y + i).get_ground_tile().z == ref_ground.z
        tiles_build_l.append(tile_x(start_field.x - 1, start_field.y + i, ref_ground.z))
      }

      tiles.append(ref_ground)

    } else if ( d == 10 ) {
      //gui.add_message_at(our_player, "tile r " + coord3d_to_string(tile_x(start_field.x + i, start_field.y + 1, start_field.z)) + " empty " + tile_x(start_field.x + i, start_field.y + 1, start_field.z).is_empty(), start_field)
      //gui.add_message_at(our_player, " -- tile l " + coord3d_to_string(tile_x(start_field.x + i, start_field.y - 1, start_field.z)) + " to empty " + tile_x(start_field.x - i, start_field.y + 1, start_field.z).is_empty(), start_field)

      // ew - r
      // build from w to e
      local ref_ground = square_x(start_field.x + i, start_field.y).get_ground_tile()
      local t = tile_x(start_field.x + i, start_field.y + 1, start_field.z)
      if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(start_field.x + i, start_field.y).get_ground_tile().z == ref_ground.z
        tiles_build_r.append(tile_x(start_field.x + i, start_field.y + 1, ref_ground.z))
      }
      // ew - l
      t = tile_x(start_field.x + i, start_field.y - 1, start_field.z)
      if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(start_field.x + i, start_field.y).get_ground_tile().z == ref_ground.z
        tiles_build_l.append(tile_x(start_field.x + i, start_field.y - 1, ref_ground.z))
      }
      tiles.append(ref_ground)
    } else if ( d == 6 || d == 9 ) {
      // build from ne to sw
      // nw - r
      if ( i > 0 ) {
        if ( tiles[i-1].get_way_dirs(wt) == 6 ) {
          diagonal_y++
        }
        if ( tiles[i-1].get_way_dirs(wt) == 9 ) {
          diagonal_x++
        }
      }

      local ref_ground = square_x(start_field.x - diagonal_x, start_field.y + diagonal_y).get_ground_tile()
      if ( print_message_box == 2 ) {
        gui.add_message_at(b_player, "6/9 ref_ground " + coord3d_to_string(ref_ground), ref_ground)
      }

      local t = null
      if ( tiles_build_l.len() == 0 && ref_ground.get_way_dirs(wt) == 9 ) {
        t = tile_x(ref_ground.x, ref_ground.y + 1, ref_ground.z)
        if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(ref_ground.x, ref_ground.y + 1).get_ground_tile().z == ref_ground.z
          tiles_build_l.append(tile_x(ref_ground.x, ref_ground.y + 1, ref_ground.z))
        }
        t = tile_x(ref_ground.x, ref_ground.y + 2, ref_ground.z)
        if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(ref_ground.x, ref_ground.y + 2).get_ground_tile().z == ref_ground.z
          tiles_build_l.append(tile_x(ref_ground.x, ref_ground.y + 2, ref_ground.z))
        }
      } else if ( tiles_build_l.len() > way_len-2 && tiles_build_l.len() <= way_len ) {
        // no build tile
      } else if ( tiles_build_l.len() < way_len-2 ) {
        t = tile_x(ref_ground.x, ref_ground.y + 2, ref_ground.z)
        if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(ref_ground.x, ref_ground.y + 2).get_ground_tile().z == ref_ground.z
          tiles_build_l.append(tile_x(ref_ground.x, ref_ground.y + 2, ref_ground.z))
        }
      }

      if ( tiles_build_r.len() == 0 && ref_ground.get_way_dirs(wt) == 6 ) {
        t = tile_x(ref_ground.x - 1, ref_ground.y, ref_ground.z)
        if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(ref_ground.x - 1, ref_ground.y).get_ground_tile().z == ref_ground.z
          tiles_build_r.append(tile_x(ref_ground.x - 1, ref_ground.y, ref_ground.z))
        }
        t = tile_x(ref_ground.x - 2, ref_ground.y, ref_ground.z)
        if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(ref_ground.x - 2, ref_ground.y).get_ground_tile().z == ref_ground.z
          tiles_build_r.append(tile_x(ref_ground.x - 2, ref_ground.y, ref_ground.z))
        }
      }else if ( tiles_build_r.len() > way_len-2 && i <= way_len ) {
        // no build tile
      } else if ( tiles_build_r.len() < way_len-2 ) {
        t = tile_x(ref_ground.x - 2, ref_ground.y, ref_ground.z)
        if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(ref_ground.x - 2, ref_ground.y).get_ground_tile().z == ref_ground.z
          tiles_build_r.append(tile_x(ref_ground.x - 2, ref_ground.y, ref_ground.z))
        }
      }
      tiles.append(ref_ground)

      if ( print_message_box == 30 && i < way_len-2 ) {
        gui.add_message_at(b_player, "tiles_build_r[" + i + "] " + coord3d_to_string(tiles_build_r[i]), world.get_time())
        gui.add_message_at(b_player, "tiles_build_l[" + i + "] " + coord3d_to_string(tiles_build_l[i]), world.get_time())
      }
    } else if ( d == 3 || d == 12 ) {
      // build from nw to se
      // nw - r
      if ( i > 0 ) {
        if ( tiles[i-1].get_way_dirs(wt) == 3 ) {
          diagonal_x++
        }
        if ( tiles[i-1].get_way_dirs(wt) == 12 ) {
          diagonal_y++
        }
      }

      local ref_ground = square_x(start_field.x + diagonal_x, start_field.y + diagonal_y).get_ground_tile()
      if ( print_message_box == 2 ) {
        gui.add_message_at(b_player, "3/12 ref_ground " + coord3d_to_string(ref_ground), world.get_time())
        gui.add_message_at(b_player, "tiles_build_l.len() " + tiles_build_l.len() + " ref_ground.get_way_dirs(wt)" + ref_ground.get_way_dirs(wt), world.get_time())
      }

      local t = null
      if ( tiles_build_l.len() == 0 && ref_ground.get_way_dirs(wt) == 3 ) {
        //gui.add_message_at(b_player, "test tile " + coord3d_to_string(tile_x(ref_ground.x, ref_ground.y + 1, ref_ground.z)), world.get_time())
        t = tile_x(ref_ground.x, ref_ground.y + 1, ref_ground.z)
        if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(ref_ground.x, ref_ground.y + 1).get_ground_tile().z == ref_ground.z
          tiles_build_l.append(tile_x(ref_ground.x, ref_ground.y + 1, ref_ground.z))
          //gui.add_message_at(b_player, "add tile " + coord3d_to_string(tile_x(ref_ground.x, ref_ground.y + 1, ref_ground.z)), world.get_time())
        }
        //gui.add_message_at(b_player, "test tile " + coord3d_to_string(tile_x(ref_ground.x, ref_ground.y + 2, ref_ground.z)), world.get_time())
        t = tile_x(ref_ground.x, ref_ground.y + 2, ref_ground.z)
        if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(ref_ground.x, ref_ground.y + 2).get_ground_tile().z == ref_ground.z
          tiles_build_l.append(tile_x(ref_ground.x, ref_ground.y + 2, ref_ground.z))
          //gui.add_message_at(b_player, "add tile " + coord3d_to_string(tile_x(ref_ground.x, ref_ground.y + 2, ref_ground.z)), world.get_time())
        }
      } else if ( tiles_build_l.len() > way_len-2 && tiles_build_l.len() <= way_len ) {
        // no build tile
      } else if ( tiles_build_l.len() < way_len-2 ) {
        //gui.add_message_at(b_player, "test tile " + coord3d_to_string(tile_x(ref_ground.x, ref_ground.y + 2, ref_ground.z)), world.get_time())
        t = tile_x(ref_ground.x, ref_ground.y + 2, ref_ground.z)
        if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(ref_ground.x, ref_ground.y + 2).get_ground_tile().z == ref_ground.z
          tiles_build_l.append(tile_x(ref_ground.x, ref_ground.y + 2, ref_ground.z))
          //gui.add_message_at(b_player, "add tile " + coord3d_to_string(tile_x(ref_ground.x, ref_ground.y + 2, ref_ground.z)), world.get_time())
        }
      }

      if ( tiles_build_r.len() == 0 && ref_ground.get_way_dirs(wt) == 12 ) {
        t = tile_x(ref_ground.x + 1, ref_ground.y, ref_ground.z)
        if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(ref_ground.x + 1, ref_ground.y).get_ground_tile().z == ref_ground.z
          tiles_build_r.append(tile_x(ref_ground.x + 1, ref_ground.y, ref_ground.z))
        }
        t = tile_x(ref_ground.x + 2, ref_ground.y, ref_ground.z)
        if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(ref_ground.x + 2, ref_ground.y).get_ground_tile().z == ref_ground.z
          tiles_build_r.append(tile_x(ref_ground.x + 2, ref_ground.y, ref_ground.z))
        }
      }else if ( tiles_build_r.len() > way_len-2 && i <= way_len ) {
        // no build tile
      } else if ( tiles_build_r.len() < way_len-2 ) {
        t = tile_x(ref_ground.x + 2, ref_ground.y, start_field.z)
        if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(ref_ground.x + 2, ref_ground.y).get_ground_tile().z == ref_ground.z
          tiles_build_r.append(tile_x(ref_ground.x + 2, ref_ground.y, ref_ground.z))
        }
      }
      tiles.append(ref_ground)

      if ( print_message_box == 4 && i < way_len-2 ) {
          gui.add_message_at(b_player, " 12 tiles_build_r[" + i + "] " + coord3d_to_string(tiles_build_r[i]), world.get_time())
          gui.add_message_at(b_player, " 3 tiles_build_l[" + i + "] " + coord3d_to_string(tiles_build_l[i]), world.get_time())
      }
    }
  }

  if ( print_message_box == 1 ) {
    gui.add_message_at(b_player, "  tiles_build_r " + tiles_build_r.len(), tiles_build_r[0])
    gui.add_message_at(b_player, "  tiles_build_l " + tiles_build_l.len(), world.get_time())
  }

  // test fields flat
  local tl = 0
  local tr = 0
  if ( ( ( tiles_build_r.len() == way_len || tiles_build_l.len() == way_len ) && diagonal_st == 0 ) || ( ( tiles_build_r.len() == (way_len-2) || tiles_build_l.len() == (way_len-2) ) && diagonal_st > 0 ) ) {
    if ( diagonal_st == 0 ) {
      // test straight way
      //gui.add_message_at(b_player, " -- slope test straight way ", world.get_time())

      if ( tiles_build_r.len() == way_len ) {
        for ( local i = 0; i < way_len; i++ ) {
          if ( tiles_build_r[i].get_slope() == 0 ) {
            tr++
          }
        }
      }
      if ( tiles_build_l.len() == way_len ) {
        for ( local i = 0; i < way_len; i++ ) {
          if ( tiles_build_l[i].get_slope() == 0 ) {
            tl++
          }
        }
      }
    } else if ( diagonal_st == 6 || diagonal_st == 9 || diagonal_st == 3 || diagonal_st == 12 ) {
      // test diagonal way
      //gui.add_message_at(b_player, " -- slope test diagonal way ", world.get_time())

      if ( tiles_build_r.len() == way_len - 2 ) {
        for ( local i = 0; i < way_len - 2; i++ ) {
          //gui.add_message_at(b_player, " -- tiles_build_r[" + i + "] " + coord3d_to_string(tiles_build_r[i]), world.get_time())
          if ( tiles_build_r[i].get_slope() == 0 ) {
            tr++
          }
        }
      }

      if ( tiles_build_l.len() == way_len - 2 ) {
        for ( local i = 0; i < way_len - 2; i++ ) {
          //gui.add_message_at(b_player, " -- tiles_build_l[" + i + "] " + coord3d_to_string(tiles_build_l[i]), world.get_time())
          if ( tiles_build_l[i].get_slope() == 0 ) {
            tl++
          }
        }
      }

    }

    //gui.add_message_at(our_player, "  tiles r get_slope() = 0 " + tr, tiles_build_r[0])
    //gui.add_message_at(our_player, "  tiles l get_slope() = 0 " + tl, tiles_build_l[0])
    //gui.add_message_at(our_player, "  way_len " + way_len, world.get_time())

    local tiles_build = null
    local err = null
    local terraform = 0

    if ( ( tr == way_len &&  diagonal_st == 0 ) || ( tr == way_len - 2 &&  diagonal_st > 0 ) ) {
      if ( print_message_box == 1 ) {
        gui.add_message_at(b_player, "build flat right ", start_field)
      }
      tiles_build = tiles_build_r
      tr = way_len
      tl = 0
    }
    else if ( ( tl == way_len &&  diagonal_st == 0 ) || ( tl == way_len - 2 &&  diagonal_st > 0 ) ) {
      if ( print_message_box == 1 ) {
        gui.add_message_at(b_player, "build flat left ", start_field)
      }
      tiles_build = tiles_build_l
      tl = way_len
      tr = 0
    }
    else if ( (( tr < tiles_build_r.len() && tiles_build_r.len() == way_len && diagonal_st == 0 ) || ( tr < tiles_build_r.len() && tiles_build_r.len() == way_len - 2 &&  diagonal_st > 0 )) && tr >= tl ) {
      // check terraform tr
      if ( print_message_box == 1 ) {
        gui.add_message_at(b_player, "build right check terraform", world.get_time())
      }
      tiles_build = tiles_build_r
      tr = way_len
      tl = 0
      terraform = 1
    }
    else if ( ( tl < tiles_build_l.len() && tiles_build_l.len() == way_len && diagonal_st == 0 ) || ( tl < tiles_build_l.len() && tiles_build_l.len() == way_len - 2 &&  diagonal_st > 0 ) ) {
      // check terraform tl
      if ( print_message_box == 1 ) {
        gui.add_message_at(b_player, "build left check terraform", world.get_time())
      }
      tiles_build = tiles_build_l
      tl = way_len
      tr = 0
      terraform = 1
    }

    if ( terraform == 1 ) {
      // change terraform

      local g = terraform_way(tiles, tiles_build, tr, tl, way_len, d)
      tr = g[0]
      tl = g[1]
      if ( way_len > g[2] ) {
        tiles_build.remove(tiles_build.len()-1)
        tiles.remove(tiles.len()-1)
        way_len = g[2]
      }
    }

    // set build left or right
    /*if ( tr == way_len && tl == way_len ) {

    }
    else */
    if ( tr == way_len ) {
      tiles_build = tiles_build_r
    }
    else if ( tl == way_len ) {
      tiles_build = tiles_build_l
    }

    // set diagonal start direction correct
    if ( diagonal_st > 0 ) {
      // tr = ribi 6 /
      // tl = ribi 9 /
      if ( ( tiles[0].get_way_dirs(wt) == 9 && tr == way_len ) || ( tiles[0].get_way_dirs(wt) == 6 && tl == way_len ) || ( tiles[0].get_way_dirs(wt) == 3 && tr == way_len ) || ( tiles[0].get_way_dirs(wt) == 12 && tl == way_len ) ) {
        //gui.add_message_at(b_player, "remove first tile from tiles[] ", world.get_time())
        local n = tiles.slice(1)
        tiles.clear()
        tiles = n
      } else if ( tiles.len() > way_len ) {
        //gui.add_message_at(b_player, "remove last tile from tiles[] ", world.get_time())
        local n = tiles.slice(0, way_len)
        tiles.clear()
        tiles = n
      }


      diagonal_st = tiles[0].get_way_dirs(wt)

    }

    if ( tr == way_len ) {
      if ( print_message_box == 3 ) {
        gui.add_message_at(b_player, "build flat right ", start_field)
      }
      if ( settings.get_drive_on_left() ) {
        if ( d == 10 ) {
          signal = [{coor=coord3d(tiles_build[1].x, tiles_build[1].y, tiles_build[1].z), ribi=8}, {coor=coord3d(tiles[tiles.len()-2].x, tiles[tiles.len()-2].y, tiles[tiles.len()-2].z), ribi=2}]
          //gui.add_message_at(b_player, "settings.get_drive_on_left() signals 10 tr " + coord3d_to_string(tiles_build[1]) + " & " + coord3d_to_string(tiles[6]), world.get_time())

        } else if ( d == 5 ) {
          signal = [{coor=coord3d(tiles_build[tiles.len()-2].x, tiles_build[tiles.len()-2].y, tiles_build[tiles.len()-2].z), ribi=4}, {coor=coord3d(tiles[1].x, tiles[1].y, tiles[1].z), ribi=1}]
          //gui.add_message_at(b_player, "settings.get_drive_on_left() signals 5 tr " + coord3d_to_string(tiles_build[6]) + " & " + coord3d_to_string(tiles[1]), world.get_time())

        } else if ( diagonal_st == 6 ) {
          // ribi 6 to 6
          signal = [{coor=coord3d(tiles_build[0].x, tiles_build[0].y, tiles_build[0].z), ribi=2}, {coor=coord3d(tiles[way_len - 3].x, tiles[way_len - 3].y, tiles[way_len - 3].z), ribi=8}]
          //gui.add_message_at(b_player, "settings.get_drive_on_left() signals diagonal tr " + coord3d_to_string(tiles_build[0]) + " & " + coord3d_to_string(tiles[way_len - 2]), world.get_time())

        } else if ( diagonal_st == 12 ) {
          signal = [{coor=coord3d(tiles_build[way_len - 3].x, tiles_build[way_len - 3].y, tiles_build[way_len - 3].z), ribi=4}, {coor=coord3d(tiles[1].x, tiles[1].y, tiles[1].z), ribi=1}]
          //gui.add_message_at(b_player, "settings.get_drive_on_left() signals diagonal tr " + coord3d_to_string(tiles_build[way_len - 3]) + " & " + coord3d_to_string(tiles[1]), world.get_time())

        }
      }
      else {
        if ( d == 10 ) {
          signal = [{coor=coord3d(tiles_build[way_len - 2].x, tiles_build[way_len - 2].y, tiles_build[way_len - 2].z), ribi=2}, {coor=coord3d(tiles[1].x, tiles[1].y, tiles[1].z), ribi=8}]
          //gui.add_message_at(b_player, "signals 10 tr " + coord3d_to_string(tiles_build[6]) + " & " + coord3d_to_string(tiles[1]), world.get_time())

        } else if ( d == 5 ) {
          signal = [{coor=coord3d(tiles_build[1].x, tiles_build[1].y, tiles_build[1].z), ribi=1}, {coor=coord3d(tiles[way_len - 2].x, tiles[way_len - 2].y, tiles[way_len - 2].z), ribi=4}]
          //gui.add_message_at(b_player, "signals 5 tr " + coord3d_to_string(tiles_build[1]) + " & " + coord3d_to_string(tiles[6]), world.get_time())

        } else if ( diagonal_st == 6 ) {
          // ribi 6 to 6
          signal = [{coor=coord3d(tiles_build[way_len - 3].x, tiles_build[way_len - 3].y, tiles_build[way_len - 3].z), ribi=4}, {coor=coord3d(tiles[1].x, tiles[1].y, tiles[1].z), ribi=1}]
          //gui.add_message_at(b_player, "signals diagonal tr " + coord3d_to_string(tiles_build[way_len - 3]) + " & " + coord3d_to_string(tiles[1]), world.get_time())
        } else if ( diagonal_st == 12 ) {
          // ribi 12 to 12
          signal = [{coor=coord3d(tiles_build[0].x, tiles_build[0].y, tiles_build[0].z), ribi=8}, {coor=coord3d(tiles[way_len - 2].x, tiles[way_len - 2].y, tiles[way_len - 2].z), ribi=2}]
          //gui.add_message_at(b_player, "signals diagonal tr " + coord3d_to_string(tiles_build[0]) + " & " + coord3d_to_string(tiles[way_len - 2]), world.get_time())

        }
      }
    }
    else if ( tl == way_len ) {
      if ( print_message_box == 3 ) {
        gui.add_message_at(b_player, "build flat left ", start_field)
      }
      if ( settings.get_drive_on_left() ) {
        if (  d == 10 ) {
          signal = [{coor=coord3d(tiles_build[way_len - 2].x, tiles_build[way_len - 2].y, tiles_build[way_len - 2].z), ribi=2}, {coor=coord3d(tiles[1].x, tiles[1].y, tiles[1].z), ribi=8}]
          //gui.add_message_at(b_player, "settings.get_drive_on_left() signals 10 tl " + coord3d_to_string(tiles_build[6]) + " & " + coord3d_to_string(tiles[1]), world.get_time())

        } else if ( d == 5 ) {
          signal = [{coor=coord3d(tiles_build[1].x, tiles_build[1].y, tiles_build[1].z), ribi=1}, {coor=coord3d(tiles[way_len - 2].x, tiles[way_len - 2].y, tiles[way_len - 2].z), ribi=4}]
          //gui.add_message_at(b_player, "settings.get_drive_on_left() signals 5 tl " + coord3d_to_string(tiles_build[1]) + " & " + coord3d_to_string(tiles[6]), world.get_time())
        } else if ( diagonal_st == 9 ) {
          // ribi 9 to 9
          signal = [{coor=coord3d(tiles_build[way_len - 3].x, tiles_build[way_len - 3].y, tiles_build[way_len - 3].z), ribi=8}, {coor=coord3d(tiles[1].x, tiles[1].y, tiles[1].z), ribi=2}]
          //gui.add_message_at(b_player, "settings.get_drive_on_left() signals diagonal tl " + coord3d_to_string(tiles_build[way_len - 3]) + " & " + coord3d_to_string(tiles[2]), world.get_time())
        } else if ( diagonal_st == 3 ) {
          // ribi 3 to 3
          signal = [{coor=coord3d(tiles_build[0].x, tiles_build[0].y, tiles_build[0].z), ribi=1}, {coor=coord3d(tiles[way_len - 2].x, tiles[way_len - 2].y, tiles[way_len - 2].z), ribi=4}]
          //gui.add_message_at(b_player, "settings.get_drive_on_left() signals diagonal tl " + coord3d_to_string(tiles_build[0]) + " & " + coord3d_to_string(tiles[way_len - 2]), world.get_time())

        }
      }
      else {
        if (  d == 10 ) {
          signal = [{coor=coord3d(tiles_build[1].x, tiles_build[1].y, tiles_build[1].z), ribi=8}, {coor=coord3d(tiles[6].x, tiles[6].y, tiles[6].z), ribi=2}]
          //gui.add_message_at(b_player, "signals 10 tl " + coord3d_to_string(tiles_build[1]) + " & " + coord3d_to_string(tiles[6]), world.get_time())

        } else if ( d == 5 ) {
          signal = [{coor=coord3d(tiles_build[way_len - 1].x, tiles_build[way_len - 1].y, tiles_build[way_len - 1].z), ribi=4}, {coor=coord3d(tiles[1].x, tiles[1].y, tiles[1].z), ribi=1}]
          //gui.add_message_at(b_player, "signals 5 tl " + coord3d_to_string(tiles_build[6]) + " & " + coord3d_to_string(tiles[1]), world.get_time())
        } else if ( diagonal_st == 9 ) {
          // ribi 9 to 9
          signal = [{coor=coord3d(tiles_build[0].x, tiles_build[0].y, tiles_build[0].z), ribi=1}, {coor=coord3d(tiles[way_len - 2].x, tiles[way_len - 2].y, tiles[way_len - 2].z), ribi=4}]
          //gui.add_message_at(b_player, "signals diagonal tl " + coord3d_to_string(tiles_build[0]) + " & " + coord3d_to_string(tiles[way_len - 1]), world.get_time())
        } else if ( diagonal_st == 3 ) {
          // ribi 3 to 3
          signal = [{coor=coord3d(tiles_build[way_len - 3].x, tiles_build[way_len - 3].y, tiles_build[way_len - 3].z), ribi=2}, {coor=coord3d(tiles[1].x, tiles[1].y, tiles[1].z), ribi=8}]
          //gui.add_message_at(b_player, "signals diagonal tl " + coord3d_to_string(tiles_build[way_len - 3]) + " & " + coord3d_to_string(tiles[1]), world.get_time())

        }
      }
    }

    if ( tiles_build == null ) {
      //gui.add_message_at(b_player, " ERROR no double way found " + coord3d_to_string(tiles[0]), start_field)
      return false
    }


    // build way
    local err = null
    local sig_field = 0
    local sig_tile_new = null
    if ( start_field.get_way_dirs(wt) == 5 || start_field.get_way_dirs(wt) == 10 ) {
      if ( print_message_box == 1 ) {
        gui.add_message_at(b_player, "5/10 build tiles[0] " + coord3d_to_string(tiles[0]) + " to tiles_build[0] " + coord3d_to_string(tiles_build[0]), start_field)
      }
      // connect diagonal way
      local b_tile = null
      local tile_a = null
      local tile_b = null
      local tile_check = 0
      if ( tl == way_len ) {
        if ( start_field.get_way_dirs(wt) == 5 ) {
          tile_a = tile_x(tiles[0].x, tiles[0].y-1, tiles[0].z)
          tile_b = tile_x(tiles[0].x-1, tiles[0].y-1, tiles[0].z)
          if ( tile_a.get_way_dirs(wt) == 12 && (tile_b.get_way_dirs(wt) == 3 || (tile_b.get_way_dirs(wt) == 10 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0)) ) {
            b_tile = tile_b
          }
          tile_check = 1
          //gui.add_message_at(b_player, "(2765) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)
        } else if ( start_field.get_way_dirs(wt) == 10 ) {
          tile_a = tile_x(tiles[0].x-1, tiles[0].y, tiles[0].z)
          tile_b = tile_x(tiles[0].x-1, tiles[0].y-1, tiles[0].z)
          if ( (tile_a.get_way_dirs(wt) == 12 || tile_a.get_way_dirs(wt) == 3) && (tile_b.get_way_dirs(wt) == 3 || tile_b.get_way_dirs(wt) == 12 || (tile_b.get_way_dirs(wt) == 5 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0)) ) {
            b_tile = tile_b
          }
          tile_check = 2
          //gui.add_message_at(b_player, "(2773) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)
        }

      } else if ( tr == way_len ) {
        if ( start_field.get_way_dirs(wt) == 5 ) {
          tile_a = tile_x(tiles[0].x, tiles[0].y-1, tiles[0].z)
          tile_b = tile_x(tiles[0].x+1, tiles[0].y-1, tiles[0].z)
          if ( tile_a.get_way_dirs(wt) == 6 && (tile_b.get_way_dirs(wt) == 9 || (tile_b.get_way_dirs(wt) == 10 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0)) ) {
            b_tile = tile_b
            sig_tile_new = "tr5"
          }
          tile_check = 3
          //gui.add_message_at(b_player, "(2785) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)

        } else if ( start_field.get_way_dirs(wt) == 10 ) {
          tile_a = tile_x(tiles[0].x-1, tiles[0].y, tiles[0].z)
          tile_b = tile_x(tiles[0].x-1, tiles[0].y+1, tiles[0].z)
          if ( tile_a.get_way_dirs(wt) == 6 && (tile_b.get_way_dirs(wt) == 9 || (tile_b.get_way_dirs(wt) == 5 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0)) ) {
            b_tile = tile_b
            sig_tile_new = "tr10t"
          }
          tile_check = 4
          //gui.add_message_at(b_player, "(2795) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)
        }

      }
      if ( b_tile == null ) {
        err = command_x.build_way(b_player, tiles[0], tiles_build[0], way_obj, true)
      } else {
        err = command_x.build_way(b_player, b_tile, tiles_build[0], way_obj, true)
      }

      if ( err == null ) {
        if ( print_message_box == 1 ) {
          gui.add_message_at(b_player, "  build tiles_build[0] " + coord3d_to_string(tiles_build[0]) + " to tiles_build[" + (way_len - 1) + "] " + coord3d_to_string(tiles_build[way_len - 1]), start_field)
        }
        err = command_x.build_way(b_player, tiles_build[0], tiles_build[way_len - 1], way_obj, true)
        if ( err != null ) {
          gui.add_message_at(b_player, " ERROR => build tile " + coord3d_to_string(tiles[0]) + " to tile " + coord3d_to_string(tiles_build[way_len - 1]), tiles[0])
          err = null
        }
        if ( print_message_box == 1 ) {
          gui.add_message_at(b_player, "  build tiles_build[" + (way_len - 1) + "] " + coord3d_to_string(tiles_build[way_len - 1]) + " to tiles[" + (way_len - 1) + "] " + coord3d_to_string(tiles[way_len - 1]), start_field)
        }
        // connect diagonal way
        b_tile = null
        if ( tile_check == 1 ) {

            tile_a = tile_x(tiles[way_len - 1].x, tiles[way_len - 1].y+1, tiles[way_len - 1].z)
            tile_b = tile_x(tiles[way_len - 1].x-1, tiles[way_len - 1].y+1, tiles[way_len - 1].z)
            if ( tile_a.get_way_dirs(wt) == 9 && (tile_b.get_way_dirs(wt) == 6 || tile_b.get_way_dirs(wt) == 10 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0) ) {
              b_tile = tile_b
              sig_tile_new = "tl5"
            }
            //gui.add_message_at(b_player, "(2827) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)

        } else if ( tile_check == 2 ) {
            tile_a = tile_x(tiles[way_len - 1].x+1, tiles[way_len - 1].y, tiles[way_len - 1].z)
            tile_b = tile_x(tiles[way_len - 1].x+1, tiles[way_len - 1].y-1, tiles[way_len - 1].z)
            if ( tile_a.get_way_dirs(wt) == 6 && (tile_b.get_way_dirs(wt) == 9 || tile_b.get_way_dirs(wt) == 10 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0) ) {
              b_tile = tile_b
              sig_tile_new = "tl5"
            }
            //gui.add_message_at(b_player, "(2836) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)
        } else if ( tile_check == 3 ) {
            tile_a = tile_x(tiles[way_len - 1].x, tiles[way_len - 1].y+1, tiles[way_len - 1].z)
            tile_b = tile_x(tiles[way_len - 1].x+1, tiles[way_len - 1].y+1, tiles[way_len - 1].z)
            if ( tile_a.get_way_dirs(wt) == 3 && (tile_b.get_way_dirs(wt) == 12 || (tile_b.get_way_dirs(wt) == 5 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0)) ) {
              b_tile = tile_b
              sig_tile_new = "tr5t"
            }
            //gui.add_message_at(b_player, "(2844) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)

        } else if ( tile_check == 4 ) {
            tile_a = tile_x(tiles[way_len - 1].x+1, tiles[way_len - 1].y, tiles[way_len - 1].z)
            tile_b = tile_x(tiles[way_len - 1].x+1, tiles[way_len - 1].y+1, tiles[way_len - 1].z)
            if ( tile_a.get_way_dirs(wt) == 12 && (tile_b.get_way_dirs(wt) == 3 || (tile_b.get_way_dirs(wt) == 5 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0)) ) {
              b_tile = tile_b
              sig_tile_new = "tr10"
            }
            //gui.add_message_at(b_player, "(2853) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)

        }

        if ( b_tile == null ) {
          err = command_x.build_way(b_player, tiles_build[way_len - 1], tiles[way_len - 1], way_obj, true)

          /*if ( settings.get_drive_on_left() ) {
            local st = tile_x(signal[0].coor.x, signal[0].coor.y, signal[0].coor.z).get_way_dirs(wt)
            if ( st == 11 || st == 13 || st == 7 || st == 14 ) {
              sig_field = 1
            }
          } else {*/
            local st = tile_x(signal[1].coor.x, signal[1].coor.y, signal[1].coor.z).get_way_dirs(wt)
            if ( st == 11 || st == 13 || st == 7 || st == 14 ) {
              sig_field = 1
            }

          //}

        } else {
          err = command_x.build_way(b_player, tiles_build[way_len - 1], b_tile, way_obj, true)
        }

        if ( err != null ) {
          gui.add_message_at(b_player, " ERROR => build tile " + coord3d_to_string(tiles_build[way_len - 1]) + " to tile " + coord3d_to_string(tiles[way_len - 1]), tiles[0])
          err = null
          // remove last tile
          local tool = command_x(tool_remover)
          tool.work(our_player, tiles_build[way_len - 1])
          // build
          err = command_x.build_way(b_player, tiles_build[way_len - 2], tiles[way_len - 2], way_obj, true)
          /*if ( settings.get_drive_on_left() ) {
            local st = tile_x(signal[0].coor.x, signal[0].coor.y, signal[0].coor.z).get_way_dirs(wt)
            if ( st == 11 || st == 13 || st == 7 || st == 14 ) {
              sig_field = 1
            }
          } else {*/
            local st = tile_x(signal[1].coor.x, signal[1].coor.y, signal[1].coor.z).get_way_dirs(wt)
            if ( st == 11 || st == 13 || st == 7 || st == 14 ) {
              sig_field = 1
            }
            /*if ( signal[0].coor.x == tiles_build[way_len - 2].x && signal[0].coor.y == tiles_build[way_len - 2].y && signal[0].coor.z == tiles_build[way_len - 2].z ) {
              // 4
              signal[0].ribi = 8
            }*/
          //}
        }
      } else {
        return false
      }
    }
    else if ( tiles[0].get_way_dirs(wt) == 6 ) {
      if ( print_message_box == 1 ) {
        gui.add_message_at(b_player, "6 build tiles[0] " + coord3d_to_string(tiles[0]) + " to tiles_build[1] " + coord3d_to_string(tiles_build[1]), start_field)
      }
      err = command_x.build_way(b_player, tiles[0], tiles_build[1], way_obj, true)
      if ( err == null ) {
        if ( print_message_box == 1 ) {
          gui.add_message_at(b_player, "  build tiles_build[1] " + coord3d_to_string(tiles_build[1]) + " to tiles_build[" + (way_len - 3) + "] " + coord3d_to_string(tiles_build[way_len - 3]), start_field)
        }
        err = command_x.build_way(b_player, tiles_build[1], tiles_build[way_len - 3], way_obj, true)
        if ( err != null ) {
          gui.add_message_at(b_player, " ERROR => build tile " + coord3d_to_string(tiles_build[1]) + " to tile " + coord3d_to_string(tiles_build[way_len - 3]), tiles[0])
          err = null
        }
        if ( print_message_box == 1 ) {
          gui.add_message_at(b_player, "  build tiles_build[" + (way_len - 3) + "] " + coord3d_to_string(tiles_build[way_len - 3]) + " to tiles[" + (way_len - 1) + "] " + coord3d_to_string(tiles[way_len - 1]), start_field)
        }
        err = command_x.build_way(b_player, tiles_build[way_len - 3], tiles[way_len - 1], way_obj, true)
        if ( err != null ) {
          gui.add_message_at(b_player, " ERROR => build tile " + coord3d_to_string(tiles_build[way_len - 3]) + " to tile " + coord3d_to_string(tiles[way_len]), tiles[0])
          err = null
        }

      } else {
        return false
      }
    }
    else if ( tiles[0].get_way_dirs(wt) == 9 ) {
      if ( print_message_box == 1 ) {
        gui.add_message_at(b_player, "9 build tiles[0] " + coord3d_to_string(tiles[0]) + " to tiles_build[1] " + coord3d_to_string(tiles_build[1]), start_field)
      }
      err = command_x.build_way(b_player, tiles[0], tiles_build[1], way_obj, true)
      if ( err == null ) {
        if ( print_message_box == 1 ) {
          gui.add_message_at(b_player, "  build tiles_build[1] " + coord3d_to_string(tiles_build[1]) + " to tiles_build[" + (way_len - 4) + "] " + coord3d_to_string(tiles_build[way_len - 4]), start_field)
        }
        err = command_x.build_way(b_player, tiles_build[1], tiles_build[way_len - 4], way_obj, true)
        if ( err != null ) {
          gui.add_message_at(b_player, " ERROR => build tile " + coord3d_to_string(tiles_build[1]) + " to tile " + coord3d_to_string(tiles_build[way_len - 4]), tiles[0])
          err = null
        }
        if ( print_message_box == 1 ) {
          gui.add_message_at(b_player, "  build tiles_build[" + (way_len - 4) + "] " + coord3d_to_string(tiles_build[way_len - 4]) + " to tiles[" + (way_len - 1) + "] " + coord3d_to_string(tiles[way_len - 1]), start_field)
        }
        err = command_x.build_way(b_player, tiles_build[way_len - 4], tiles[way_len - 1], way_obj, true)
        if ( err != null ) {
          gui.add_message_at(b_player, " ERROR => build tile " + coord3d_to_string(tiles_build[way_len - 4]) + " to tile " + coord3d_to_string(tiles[way_len - 1]), tiles[0])
          err = null
        }

      }
    }
    else if ( tiles[0].get_way_dirs(wt) == 3 ) {
      if ( print_message_box == 1 ) {
        gui.add_message_at(b_player, "3 build tiles[0] " + coord3d_to_string(tiles[0]) + " to tiles_build[1] " + coord3d_to_string(tiles_build[1]), start_field)
      }
      err = command_x.build_way(b_player, tiles[0], tiles_build[1], way_obj, true)
      if ( err == null ) {
        if ( print_message_box == 1 ) {
          gui.add_message_at(b_player, "  build tiles_build[1] " + coord3d_to_string(tiles_build[1]) + " to tiles_build[" + (way_len - 4) + "] " + coord3d_to_string(tiles_build[way_len - 4]), start_field)
        }
        err = command_x.build_way(b_player, tiles_build[1], tiles_build[way_len - 4], way_obj, true)
        if ( err != null ) {
          gui.add_message_at(b_player, " ERROR => build tile " + coord3d_to_string(tiles_build[1]) + " to tile " + coord3d_to_string(tiles_build[way_len - 4]), tiles[0])
          err = null
        }
        if ( print_message_box == 1 ) {
          gui.add_message_at(b_player, "  build tiles_build[" + (way_len - 4) + "] " + coord3d_to_string(tiles_build[way_len - 4]) + " to tiles[" + (way_len - 1) + "] " + coord3d_to_string(tiles[way_len - 1]), start_field)
        }
        err = command_x.build_way(b_player, tiles_build[way_len - 4], tiles[way_len - 1], way_obj, true)
        if ( err != null ) {
          gui.add_message_at(b_player, " ERROR => build tile " + coord3d_to_string(tiles_build[way_len - 4]) + " to tile " + coord3d_to_string(tiles[way_len - 1]), tiles[0])
          err = null
        }

      }
    }
    else if ( tiles[0].get_way_dirs(wt) == 12 ) {
      if ( print_message_box == 1 ) {
        gui.add_message_at(b_player, "12 build tiles[0] " + coord3d_to_string(tiles[0]) + " to tiles_build[1] " + coord3d_to_string(tiles_build[1]), start_field)
      }
      err = command_x.build_way(b_player, tiles[0], tiles_build[1], way_obj, true)
      if ( err == null ) {
        if ( print_message_box == 1 ) {
          gui.add_message_at(b_player, "  build tiles_build[1] " + coord3d_to_string(tiles_build[1]) + " to tiles_build[" + (way_len - 3) + "] " + coord3d_to_string(tiles_build[way_len - 3]), start_field)
        }
        err = command_x.build_way(b_player, tiles_build[1], tiles_build[way_len - 3], way_obj, true)
        if ( err != null ) {
          gui.add_message_at(b_player, " ERROR => build tile " + coord3d_to_string(tiles_build[1]) + " to tile " + coord3d_to_string(tiles_build[way_len - 3]), tiles[0])
          err = null
        }
        local t = 1
        if ( tiles[way_len - t].get_way_dirs(wt) == 3 ) {
          t = 0
        }
        if ( print_message_box == 1 ) {
          gui.add_message_at(b_player, "  build tiles_build[" + (way_len - 3) + "] " + coord3d_to_string(tiles_build[way_len - 3]) + " to tiles[" + (way_len - 1) + "] " + coord3d_to_string(tiles[way_len - 1]), start_field)
        }
        err = command_x.build_way(b_player, tiles_build[way_len - 3], tiles[way_len - 1], way_obj, true)
        if ( err != null ) {
          gui.add_message_at(b_player, " ERROR => build tile " + coord3d_to_string(tiles_build[way_len - 3]) + " to tile " + coord3d_to_string(tiles[way_len - 1]), tiles[0])
          err = null
        }

      }
    }

    // build signals
    if ( diagonal_st == 0 ) {
        // build signals
        // select signal tool
        local obj_sign = find_signal("is_signal", wt)

        local sig_1 = tile_x(signal[0].coor.x, signal[0].coor.y, signal[0].coor.z)
        local sig_2 = tile_x(signal[1].coor.x, signal[1].coor.y, signal[1].coor.z)
        if ( sig_1.find_object(mo_way) != null && sig_2.find_object(mo_way) != null ) {
          local test = 0
          /*local tiles = [3, 5, 6, 7, 9, 10, 11, 12]
          for ( local i = 0; i < tiles.len(); i++ ) {
            if ( sig_1.get_way_dirs(wt) == tiles[i] ) {
              test++
            }
            if ( sig_2.get_way_dirs(wt) == tiles[i] ) {
              test++
            }
          }
          if ( test == 2 ) {*/
            for ( local j=0; j < signal.len(); j++ ) {

              if ( print_message_box == 1 ) {
                gui.add_message_at(b_player, "signal to tile " + coord3d_to_string(tile_x(signal[j].coor.x, signal[j].coor.y, signal[j].coor.z)), start_field)
              }

              local fx = 0
              local fy = 0
              if ( j == 1 && d == 5 ) { //&& signal[j].coor.y == tiles[tiles.len()-2].y
                fy = sig_field
              } else if ( j == 1 && d == 10 ) { //&& signal[j].coor.x == tiles[tiles.len()-2].x
                fx = sig_field
              }

              local s_ribi = signal[j].ribi
              test = tile_x(signal[j].coor.x-fx, signal[j].coor.y-fy, signal[j].coor.z).get_way_dirs(wt)
              //gui.add_message_at(b_player, "signal set to ribi " + s_ribi, world.get_time())
              if ( print_message_box == 3 ) {
                gui.add_message_at(b_player, "signal to tile " + coord3d_to_string(tile_x(signal[j].coor.x, signal[j].coor.y, signal[j].coor.z)), world.get_time())
              }

              local signal_build_tile = tile_x(signal[j].coor.x-fx, signal[j].coor.y-fy, signal[j].coor.z)

              if ( settings.get_drive_on_left() ) {


              } else {
                // set s_ribi new by double track lenght 7
                if ( test == 9 && s_ribi == 2 ) { s_ribi = 1 }
                if ( test == 12 && s_ribi == 1 ) { s_ribi = 8 }
                if ( test == 3 && s_ribi == 4 ) { s_ribi = 2 }
                if ( test == 6 && s_ribi == 8 ) { s_ribi = 4 }
                if ( print_message_box == 1 && signal[j].ribi != s_ribi ) {
                  gui.add_message_at(b_player, coord3d_to_string(tile_x(signal[j].coor.x-fx, signal[j].coor.y-fy, signal[j].coor.z)) + " signal set to ribi new " + s_ribi, world.get_time())
                }

                //gui.add_message_at(b_player, "sig_tile_new " + sig_tile_new, world.get_time())
                if ( (sig_tile_new == "tr10" && j == 0) || (sig_tile_new == "tl5" && j == 0) ) {
                  signal_build_tile = tiles_build[way_len - 1]
                //} else if ( sig_tile_new == "tr5s" ) {

                } else if ( (sig_tile_new == "tl10" && j == 0) || (sig_tile_new == "tr5" && j == 0) ) {
                  signal_build_tile = tiles_build[0]
                } else if ( (sig_tile_new == "tr10t" && j == 1) || (sig_tile_new == "tr5t" && j == 1) ) {
                  signal_build_tile = tiles[0]
                }
              }

              // check ribi
              test = signal_build_tile.get_way_dirs(wt)
              if ( test == 3 && s_ribi == 4 ) { s_ribi = 2 }


              while(true){
                local err = command_x.build_sign_at(b_player, signal_build_tile, obj_sign)
                local ribi = signal_build_tile.get_way_dirs_masked(wt)
                if (ribi == s_ribi)
                  break
              }

            }

          //}
        }

      if ( sig_tile_new != null ) {
        //::debug.pause()

      }

    }
    else if ( diagonal_st == 6 || diagonal_st == 9 || diagonal_st == 3 || diagonal_st == 12 ) {
        // build signals
        local list = sign_desc_x.get_available_signs(wt)
        local obj_sign = null
        foreach(o in list) {
          if ( print_message_box == 2 ) {
            gui.add_message_at(b_player, "signals " + o.get_name(), start_field)
          }
          if (o.is_signal()) {
            obj_sign = o
            break
          }
        }

        local sig_1 = tile_x(signal[0].coor.x, signal[0].coor.y, signal[0].coor.z)
        local sig_2 = tile_x(signal[1].coor.x, signal[1].coor.y, signal[1].coor.z)
        if ( sig_1.find_object(mo_way) != null && sig_2.find_object(mo_way) != null ) {
          local tiles = [3, 5, 6, 9, 10, 12]
          local test = 0
          for ( local i = 0; i < tiles.len(); i++ ) {
            if ( sig_1.get_way_dirs(wt) == tiles[i] ) {
              test++
            }
            if ( sig_2.get_way_dirs(wt) == tiles[i] ) {
              test++
            }
          }
          if ( test == 2 ) {

            for ( local j=0; j < signal.len(); j++ ) {

              if ( print_message_box == 1 ) {
                gui.add_message_at(b_player, "signal to tile " + coord3d_to_string(tile_x(signal[j].coor.x, signal[j].coor.y, signal[j].coor.z)), world.get_time())
              }

              while(true){
                local err = command_x.build_sign_at(b_player, tile_x(signal[j].coor.x, signal[j].coor.y, signal[j].coor.z), obj_sign)
                local ribi = tile_x(signal[j].coor.x, signal[j].coor.y, signal[j].coor.z).get_way_dirs_masked(wt)
                if (ribi == signal[j].ribi)
                  break
              }

            }

          }
        }
    }

    print_message_box = 0
    return true
  }
}

/*
 *  start = start field line
 *  end   = end field line
 *  wt    = waytype
 *  l     = stations distance
 *  c     = count of double ways
 *  c=0 -> no build double ways - return route array
 */
function check_way_line(start, end, wt, l, c, r_line) {
  /*
   * 1 =
   * 2 = straight
   * 3 = diagonal
   * 4 =
   */

  if ( debug ) ::debug.set_pause_on_error(true)
  //debug.pause

  local print_message_box = 0
  local print_message = 0
  local message_text = []

  if ( print_message_box > 0 ) {
    gui.add_message_at(our_player, " ### check_way_line ### " + coord3d_to_string(start), start)
  }

  local nexttile = [] //[tile_x(start.x, start.y, start.z)]
  local station_len = 0

  //if (wt == wt_rail) {
    local asf = astar_route_finder(wt)
    local result = asf.search_route([start], [end])
    // result is contains routes-array or error message
    // route is backward from end to start

    if ("err" in result) {
      //gui.add_message_at(our_player, " ### no route found: " + result.err, start)
      return nexttile
    }
    else {
      //gui.add_message_at(our_player, " ### route found: length =  " +  result.routes.len(), start)
      // route found, mark tiles
      local marked = {}
      local a = 0
      foreach(node in result.routes) {
        local tile = tile_x(node.x, node.y, node.z)
        nexttile.append(tile)

        // count station len start/end
        if ( tile.find_object(mo_building) != null && a == 0 ) {
          station_len++
        } else {
          if ( a == 0 ) {
            station_len += 2
          }
          a = 1
        }
        /*
        try {
          tile.mark();
          marked.append(tile)
        } catch(ev) {}*/
      }
      //::debug.pause()
      sleep()
      // unmark if game is unpaused again
      /*
      foreach(tile in marked) {
        tile.unmark();
      }
      sleep()*/
    }
  //}

  // optimize way line befor build double ways
  //optimize_way_line(nexttile, wt)

/*
  gui.add_message_at(our_player, "end line " + coord3d_to_string(end), end)
  gui.add_message_at(our_player, "length " + l, world.get_time())
  gui.add_message_at(our_player, "count " + c, world.get_time())
*/
  nexttile.reverse()

  if ( c == 0 ) {
    // return way line
    if ( l == 0 ) {
      // return way line len
      return nexttile.len()
    } else {
      // return way array
      return nexttile
    }
  }

  local d = 0

  // count double ways
  local start_fields = []
  local start_fields_id = []
  local as = 0
  local fc = 0

  local dfcl = 0
  local dfcr = 0

  local dc = 0
  local di = 0
  local r = 0
  local s = []

  local l_split = 25
  if ( station_len < 8 ) {
    station_len = 8
  } else if ( station_len > 8 ) {
    l_split = station_len + 1
  }

  if ( c > 0 ) {
    // distance double ways
    //local as = (l / (c + 1)).tointeger()
    as = (l / l_split).tointeger()
    //as = as - ( c * 16 )
    if ( print_message_box > 0 ) {
      gui.add_message_at(our_player, c + " double way search", world.get_time())
      message_text.append("l_split " + l_split + " as " + as + " l " + l + " c " + c)
    }

      for (local i = 0; i < as; i++ ) {
        if ( i == 0 ) {
          //if ( c == 1 ) {
          //  s.append(15)
          //} else {
            s.append(l_split - (l_split * 0.3).tointeger() )
          //}
        } else {
          s.append(s[i-1]+(l_split * 0.7).tointeger())
        }
        message_text.append("  s[" + (i) + "] " + s[i])
      }
  } else {
    s.append(1)
  }

  local sign = 0
  local reset = 0

  local stl = 0
  local str = 0
  local dst = 0
  local i = 8

  while ( i < (nexttile.len() - 1 - station_len) ) {
    i++
    // check to signal
    local sig = nexttile[i-1].find_object(mo_signal)
    //gui.add_message_at(our_player, "find_object(mo_signal) " + sig, nexttile[i-1])
    if ( sig != null && c > 0 ) {
      sign++
      if ( sign == c ) {
        //gui.add_message_at(our_player, c + " double way found, no build ", world.get_time())
        return true
      }
      print_message = 0
      //gui.add_message_at(our_player, "signals " + sign, world.get_time())
    }

    di = dc
    dc = d

    /*if ( i < station_len ) {
      st_tile = nexttile[i-1].find_object(mo_building)
    }*/

    // len from double track
    local way_len = 8
    if ( station_len > 8 ) {
      way_len = station_len
      if ( d == 6 || d == 9 || d == 3 || d == 12) {
        way_len = station_len + 1
      }
    } else {
      if ( d == 6 || d == 9 || d == 3 || d == 12) {
        way_len = station_len + 1
      }
    }

    d = nexttile[i].get_way_dirs(wt)

    // diagonal start ribi
    if ( ( dc == 5 && d == 6 ) || ( dc == 5 && d == 9 ) ) {
      dst = 5
    } else if ( ( dc == 10 && d == 6 ) || ( dc == 10 && d == 9 ) ) {
      dst = 10
    } else if ( ( dc == 5 && d == 3 ) || ( dc == 5 && d == 12 ) ) {
      dst = 5
    } else if ( ( dc == 10 && d == 3 ) || ( dc == 10 && d == 12 ) ) {
      dst = 10
    } else if ( d == 5 || d == 10 ) {
      dst = 0
    }

    local t = nexttile[i]
    local st = 0 // st == 1 no field for double way
    if ( dst == 0 && fc == 0 && ( t.get_slope() > 0 || t.is_bridge() || t.has_two_ways() ) ) {
      // check slope, bridge and crossing to start field for double way
      if ( i >= s[r] && print_message_box == 2 ) {
        gui.add_message_at(our_player, " ### check first tile " + coord3d_to_string(t), t)
      }
      st = 1
    } else if ( t.is_bridge() || t.has_two_ways() ) {
      // check bridge and crossing
      if ( i >= s[r] && print_message_box == 2 ) {
        gui.add_message_at(our_player, " ### check bridge " + coord3d_to_string(t), t)
      }
      st = 1
    } else if ( t.get_way_dirs(wt) == 10 && t.find_object(mo_building) == null ) {
      local check_tile_str = tile_x(t.x, t.y + 1, t.z)
      local check_tile_stl = tile_x(t.x, t.y - 1, t.z)
      // check left & right ground and empty
      if ( world.is_coord_valid(check_tile_str) ) {
        // field right empty and ground
        if ( tile_x(t.x, t.y + 1, t.z).is_ground() && tile_x(t.x, t.y + 1, t.z).is_empty() ) {
          str++
        } else {
          str = 0
        }
      } else {
        str = 0
      }
      if ( world.is_coord_valid(check_tile_stl) ) {
        // field left empty and ground
        if ( tile_x(t.x, t.y - 1, t.z).is_ground() && tile_x(t.x, t.y - 1, t.z).is_empty() ) {
          stl++
        } else {
          stl = 0
        }
      } else {
        stl = 0
      }
      // end diagonal way
      dst = 0
      dfcl = 0
      dfcr = 0
    } else if ( t.get_way_dirs(wt) == 5 && t.find_object(mo_building) == null ) {
      // check left & right out of map and ground and empty
      local check_tile_str = tile_x(t.x + 1, t.y, t.z)
      local check_tile_stl = tile_x(t.x - 1, t.y, t.z)

      if ( world.is_coord_valid(check_tile_str) ) {
        // field right empty and ground
        if ( check_tile_str.is_ground() && check_tile_str.is_empty() ) {
          str++
        } else {
          str = 0
        }
      } else {
        str = 0
      }
      if ( world.is_coord_valid(check_tile_stl) ) {
        // field left empty and ground
        if ( check_tile_stl.is_ground() && check_tile_stl.is_empty() ) {
          stl++
        } else {
          stl = 0
        }
      } else {
        stl = 0
      }
      // end diagonal way
      dst = 0
      dfcl = 0
      dfcr = 0
    } else if ( dst > 0 && t.get_way_dirs(wt) == 6 || t.get_way_dirs(wt) == 9 ) {
      if ( print_message_box == 3 && i >= s[0] && i < (s[0] + way_len) && str == 0 && stl == 0 ) {
        gui.add_message_at(our_player, " # test 6/9 " + coord3d_to_string(t), t)
        gui.add_message_at(our_player, " diagonal start dst " + dst, world.get_time())
        gui.add_message_at(our_player, " dc " + dc + " dfcl " + dfcl + " dfcr " + dfcr + " - stl " + stl + " - str " + str, world.get_time())
      }
      if ( dst == 5 || dst == 10 ) {
        local check_tile_strs = null
        local check_tile_str = null
        local check_tile_stls = null
        local check_tile_stl = null
        if ( nexttile[i-2].x < nexttile[i-1].x || nexttile[i-2].y > nexttile[i-1].y ) {
          check_tile_strs = tile_x(t.x, t.y - 1, t.z)
          check_tile_str = tile_x(t.x, t.y - 2, t.z)
          check_tile_stls = tile_x(t.x + 1, t.y, t.z)
          check_tile_stl = tile_x(t.x + 2, t.y, t.z)
          //gui.add_message_at(our_player, " diagonal test A " + " i " + i + " - tile check_tile_str " + coord3d_to_string(check_tile_str), world.get_time())
        } else {
          check_tile_strs = tile_x(t.x - 1, t.y, t.z)
          check_tile_str = tile_x(t.x - 2, t.y, t.z)
          check_tile_stls = tile_x(t.x, t.y + 1, t.z)
          check_tile_stl = tile_x(t.x, t.y + 2, t.z)
          //gui.add_message_at(our_player, " diagonal test B " + " i " + i + " - tile check_tile_str " + coord3d_to_string(check_tile_str), world.get_time())
        }

        local tile_on_map_r = 0
        local tile_on_map_l = 0
        if ( world.is_coord_valid(check_tile_strs) && world.is_coord_valid(check_tile_str) ) {
          // tile is in range map
          tile_on_map_r = 1
        } else {
          // tile is out of range map
          str = 0
        }

        if ( world.is_coord_valid(check_tile_stls) && world.is_coord_valid(check_tile_stl) ) {
          // tile is in range map
          tile_on_map_l = 1
        } else {
          // tile is out of range map
          stl = 0
        }

        if ( ( dc == 6 && t.get_way_dirs(wt) == 9 ) || ( dc == 9 && t.get_way_dirs(wt) == 6 ) ) {
          if ( t.get_way_dirs(wt) == 6 && str == 0 ) {
            if ( print_message_box == 3 && i >= s[0] && i < (s[0] + way_len) ) {
              gui.add_message_at(our_player, " - check start ribi 6 " + coord3d_to_string(t), t)
            }
            if ( tile_on_map_r == 1 ) {
              if ( check_tile_strs.is_ground() && check_tile_strs.is_empty() ) { //&& tile_x(t.x, t.y - 2, t.z).is_ground() && tile_x(t.x, t.y - 2, t.z).is_empty()
                str++
              } else {
                str = 0
              }
            } else {
              //gui.add_message_at(our_player, " ERROR test tile out of map " + coord3d_to_string(check_tile_strs) + " / " + coord3d_to_string(check_tile_str), t)
            }
          } else if ( t.get_way_dirs(wt) == 9 && stl == 0 ) {
            if ( print_message_box == 3 && i >= s[0] && i < (s[0] + way_len) ) {
              gui.add_message_at(our_player, " - check start ribi 9 " + coord3d_to_string(t), t)
            }
            if ( tile_on_map_l == 1 ) {
              if ( check_tile_stls.is_ground() && check_tile_stls.is_empty() ) { //&& tile_x(t.x, t.y + 2, t.z).is_ground() && tile_x(t.x, t.y + 2, t.z).is_empty()
                stl++
              } else {
                stl = 0
              }
            } else {
              //gui.add_message_at(our_player, " ERROR test tile out of map " + coord3d_to_string(check_tile_stls) + " / " + coord3d_to_string(check_tile_stl), t)
            }
          }

          if ( t.get_way_dirs(wt) == 6 || t.get_way_dirs(wt) == 9 ) {
            if ( print_message_box == 3 && i >= s[0] && i < (s[0] + way_len) ) {
              gui.add_message_at(our_player, " - check ribi 6/9 l/r " + coord3d_to_string(t) + " d " + t.get_way_dirs(wt), t)
            }
            if ( tile_on_map_r == 1 ) {
              if ( str > 0 && str <= way_len - 3 ) {
                if ( check_tile_str.is_ground() && check_tile_str.is_empty() ) {
                  //gui.add_message_at(player_x(1), " + str " + str, world.get_time())
                  str++
                } else {
                  str = 0
                }

              } else if ( str > way_len - 3 && str <= way_len ) {
                //gui.add_message_at(player_x(1), " ++ str " + str, world.get_time())
                str++
              }
            } else {
              //gui.add_message_at(our_player, "(3320) ERROR test tile out of map ", t)
            }

            if ( tile_on_map_l == 1 ) {
              if ( stl > 0 && stl <= way_len - 3 ) {
                if ( check_tile_stl.is_ground() && check_tile_stl.is_empty() ) {
                  //gui.add_message_at(player_x(1), " + stl " + stl, world.get_time())
                  stl++
                } else {
                  stl = 0
                }
              } else if ( stl > way_len - 3 && stl <= way_len ) {
                //gui.add_message_at(player_x(1), " ++ stl " + stl, world.get_time())
                stl++
              }
            } else {
              //gui.add_message_at(our_player, "(3336) ERROR test tile out of map ", t)
            }

          }
          if ( print_message_box == 2 && i >= s[0] && i < (s[0] + way_len + 1) ) {
            gui.add_message_at(our_player, " -- stl " + stl + " - str " + str + " way_len " + way_len, world.get_time())
          }
        }
      }
      // end straight way
      fc = 0
    } else if ( dst > 0 && t.get_way_dirs(wt) == 3 || t.get_way_dirs(wt) == 12 ) {
      if ( print_message_box == 4 && i >= s[0] && i < (s[0] + way_len) && str == 0 && stl == 0 ) { //
        gui.add_message_at(our_player, " # test 3/12 " + coord3d_to_string(t), t)
        gui.add_message_at(our_player, " diagonal start dst " + dst, world.get_time())
        gui.add_message_at(our_player, " dc " + dc + " dfcl " + dfcl + " dfcr " + dfcr + " - stl " + stl + " - str " + str, world.get_time())
      }
      if ( dst == 5 || dst == 10 ) {
        local check_tile_strs = null
        local check_tile_str = null
        local check_tile_stls = null
        local check_tile_stl = null
        if ( nexttile[i-2].x > nexttile[i-1].x || nexttile[i-2].y > nexttile[i-1].y ) {
          check_tile_strs = tile_x(t.x - 1, t.y, t.z)
          check_tile_str = tile_x(t.x - 2, t.y, t.z)
          check_tile_stls = tile_x(t.x, t.y + 1, t.z)
          check_tile_stl = tile_x(t.x, t.y + 2, t.z)
        } else {
          check_tile_strs = tile_x(t.x, t.y - 1, t.z)
          check_tile_str = tile_x(t.x, t.y - 2, t.z)
          check_tile_stls = tile_x(t.x + 1, t.y, t.z)
          check_tile_stl = tile_x(t.x + 2, t.y, t.z)
        }

        local tile_on_map_r = 0
        local tile_on_map_l = 0
        if ( world.is_coord_valid(check_tile_strs) && world.is_coord_valid(check_tile_str) ) {
          // tile is in range map
          tile_on_map_r = 1
        } else {
          // tile is out of range map
          str = 0
        }

        if ( world.is_coord_valid(check_tile_stls) && world.is_coord_valid(check_tile_stl) ) {
          // tile is in range map
          tile_on_map_l = 1
        } else {
          // tile is out of range map
          stl = 0
        }

        if ( ( dc == 3 && t.get_way_dirs(wt) == 12 ) || ( dc == 12 && t.get_way_dirs(wt) == 3 ) ) {
          if ( t.get_way_dirs(wt) == 3 && str == 0 ) {
            if ( print_message_box == 4 ) { //&& i >= s[0] && i < (s[0] + way_len )
              gui.add_message_at(our_player, " - check start ribi 3 " + coord3d_to_string(t), t)
            }
            if ( tile_on_map_r == 1 ) {
              if ( check_tile_strs.is_ground() && check_tile_strs.is_empty()  ) {
                str++
              } else {
                str = 0
              }
            } else {
              //gui.add_message_at(our_player, "(3400) ERROR test tile out of map ", t)
            }
          } else if ( t.get_way_dirs(wt) == 12 && stl == 0 ) {
            if ( print_message_box == 4 && i >= s[0] && i < (s[0] + way_len) ) { //
              gui.add_message_at(our_player, " - check start ribi 12 " + coord3d_to_string(t), t)
            }
            if ( tile_on_map_l == 1 ) {
              if ( check_tile_stls.is_ground() && check_tile_stls.is_empty() ) {
                stl++
              } else {
                stl = 0
              }
            } else {
              //gui.add_message_at(our_player, "(3414) ERROR test tile out of map ", t)
            }
          }

          if ( t.get_way_dirs(wt) == 3 || t.get_way_dirs(wt) == 12 ) {
            if ( print_message_box == 4 && i >= s[0] && i < (s[0] + way_len) ) {
              gui.add_message_at(our_player, " - check ribi 3/12 l/r " + coord3d_to_string(t) + " d " + t.get_way_dirs(wt), t)
            }
            if ( tile_on_map_r == 1 ) {
              if ( str > 0 && str <= way_len - 3 ) {
                if ( check_tile_str.is_ground() && check_tile_str.is_empty() ) {
                  //gui.add_message_at(player_x(1), " + str " + str, world.get_time())
                  str++
                } else {
                  str = 0
                }
              } else if ( str > way_len - 3 && str <= way_len ) {
                //gui.add_message_at(player_x(1), " ++ str " + str, world.get_time())
                str++
              }
            } else {
              //gui.add_message_at(our_player, "(3436) ERROR test tile out of map ", t)
            }

            if ( tile_on_map_l == 1 ) {
              if ( stl > 0 && stl <= way_len - 3 ) {
                if ( check_tile_stl.is_ground() && check_tile_stl.is_empty() ) {
                  //gui.add_message_at(player_x(1), " + stl " + stl, world.get_time())
                  stl++
                } else {
                  stl = 0
                }
              } else if ( stl > way_len - 3 && stl <= way_len ) {
                //gui.add_message_at(player_x(1), " ++ stl " + stl, world.get_time())
                stl++
              }
            } else {
              //gui.add_message_at(our_player, "(3453) ERROR test tile out of map ", t)
            }
          }
          if ( print_message_box == 4 && i >= s[0] && i < (s[0] + way_len + 1) ) {
            gui.add_message_at(our_player, " -- stl " + stl + " - str " + str + " way_len " + way_len, world.get_time())
          }

        }
      }
      // end straight way
      fc = 0
    }

    if ( i >= s[r] && print_message_box == 2 ) {
        gui.add_message_at(our_player, "*  st " + st + "  dst " + dst + "  stl " + stl + " str " + str + " fc " + fc + " * " + coord3d_to_string(t), t)
    }
    if ( t.get_slope() > 0 && nexttile[i-1].get_slope() > 0 ) {
      if ( i >= s[r] && print_message_box == 2 ) {
        gui.add_message_at(our_player, " last 2 tiles have slope - reset " + coord3d_to_string(t), t)
        ::debug.pause()
      }
      stl = 0
      str = 0
      fc = 0
    }

    if ( dst == 0 && dc == d && i >= s[r] && st == 0 ) {
      if ( print_message_box == 2 ) {
        gui.add_message_at(our_player, "  stl " + stl + " str " + str + " fc " + fc + " * " + coord3d_to_string(t), t)
      }
      if ( stl == fc + 1 || str == fc + 1 ) {
        fc++
        dfcr = 0
        dfcl = 0
      } else {
        fc = 0
        stl = 0
        str = 0
      }
    } else if ( dst > 0 && i >= s[r] && st == 0 ) {
      if ( print_message_box == 3 ) {
        gui.add_message_at(player_x(1), "  stl " + stl + " str " + str + " fc " + fc + " * " + coord3d_to_string(t), t)
        gui.add_message_at(player_x(1), "  dfcl " + dfcl + " dfcr " + dfcr, world.get_time())
      }
      if ( stl == dfcl + 2 ) {
        dfcl++
        fc = 0
      } else {
        dfcl = 0
        stl = 0
      }
      if ( str == dfcr + 2 ) {
        dfcr++
        fc = 0
      } else {
        dfcr = 0
        str = 0
      }
    } else {
      fc = 0
      stl = 0
      str = 0
    }

    if ( print_message_box == 4 ) {
      gui.add_message_at(player_x(0), "  fc " + fc + " s[" + r + "] " + s[r] + " i " + i + " - " + l + " dc " + dc + " di " + di + " d " + d + " * " + coord3d_to_string(t), world.get_time())
    }

    if ( i == s[r] ) {
      message_text.append(" s[" + r + "] " + s[r] + " i " + i + " is tile " + coord3d_to_string(t))
    }


    if ( i >= s[r] && ( fc >= way_len || dfcl >= way_len || dfcr >= way_len ) ) { // && start_fields.len() < c
      if ( ( nexttile[i-1].x > nexttile[i].x && fc > 0 && nexttile[i-1].y == nexttile[i].y ) || ( nexttile[i-1].y > nexttile[i].y && fc > 0 ) || ( nexttile[i-1].y > nexttile[i].y && fc == 0 ) || ( nexttile[i-1].x < nexttile[i].x && fc == 0 && nexttile[i-2].y > nexttile[i].y ) ) {
        /*
         *
         *
         */
        if ( nexttile[i].get_slope() == 0 ) {
          if ( print_message == 1 ) {
            gui.add_message_at(our_player, " (1) add nexttile[i] id = " + i + " " + coord3d_to_string(t), t)
          }
          //start_fields.append(nexttile[i])
          start_fields_id.append(i)
          stl = 0
          str = 0
          fc = 0
        } else if ( nexttile[i+1].get_slope() == 0 ) {
          // plan start tile has slope then next tile
          if ( print_message == 1 ) {
            gui.add_message_at(our_player, " (2) add nexttile[i+1] id = " + (i+1) + " " + coord3d_to_string(nexttile[i+1]), nexttile[i+1])
          }
          //start_fields.append(nexttile[i+1])
          start_fields_id.append(i-1)
          stl = 0
          str = 0
          fc = 0
        }
      } else if ( nexttile[i-1].x > nexttile[i].x && fc == 0 && nexttile[i-2].y > nexttile[i].y ) {
        /*
         *
         *
         */
        if ( nexttile[i].get_slope() == 0 ) {
          if ( print_message == 1 ) {
            gui.add_message_at(our_player, " (3) add nexttile[i] id = " + i + " " + coord3d_to_string(t), t)
          }
          //start_fields.append(nexttile[i])
          start_fields_id.append(i)
          stl = 0
          str = 0
          fc = 0
        }
      } else if ( nexttile[i-1].x < nexttile[i].x && fc > 0 && nexttile[i-2].y == nexttile[i].y ) {
        if ( nexttile[i-way_len].get_slope() == 0 && !nexttile[i-way_len].is_bridge() && nexttile[i-way_len].find_object(mo_crossing) == null ) {
          //start_fields.append(nexttile[i-way_len])
          start_fields_id.append(i-way_len)
          if ( print_message == 1 ) {
            gui.add_message_at(our_player, " (4) add nexttile[i-way_len] id = " + (i-way_len) + " " + coord3d_to_string(t), t)
          }
        } else {
          //start_fields.append(nexttile[i-way_len+1])
          start_fields_id.append(i-way_len+1)
          if ( print_message == 1 ) {
            gui.add_message_at(our_player, " (5) add nexttile[i-way_len+1] id = " + (i-way_len) + " " + coord3d_to_string(t), t)
          }
        }
          stl = 0
          str = 0
      } else {
          if ( ( nexttile[i-way_len].get_way_dirs(wt) == 3 || nexttile[i-way_len].get_way_dirs(wt) == 12 ) && nexttile[i-way_len].get_slope() == 0 && fc == 0 ) {
            //start_fields.append(nexttile[i-way_len])
            start_fields_id.append(i-way_len)
            stl = 0
            str = 0
            if ( print_message == 1 ) {
              gui.add_message_at(our_player, " (6) add nexttile[i-way_len] id = " + (i-way_len) + " " + coord3d_to_string(t), t)
            }
          } else if ( nexttile[i-way_len-1].get_slope() == 0 ) {
            //start_fields.append(nexttile[i-way_len+1])
            start_fields_id.append(i-way_len+1)
            stl = 0
            str = 0
            fc = 0
            if ( print_message == 1 ) {
              gui.add_message_at(our_player, " (7) add nexttile[i-way_len+1] id = " + (i-way_len+1) + " " + coord3d_to_string(t), t)
            }
          }
      }

      if ( print_message_box == 2 ) {
        gui.add_message_at(our_player, "  tile s[" + r + "] " + coord3d_to_string(start_fields[r]), start_fields[r])
        gui.add_message_at(our_player, "  i " + i + "  r " + r + "  s[" + r + "] " + s[r], world.get_time())
        gui.add_message_at(our_player, "* " + coord3d_to_string(t), world.get_time())
      }

      local dw_abs = 10
      if ( station_len > 8 ) {
        dw_abs = 1
      }
      if ( r < as ) {
        if ( r < s.len() - 1 ) {
          if ( i > s[r] ) {
            s[r] = i + dw_abs
          } else {
            s[r] = i
          }
        } else {
          if ( i > s[r] ) {
            s.append(i + dw_abs)
          } else {
            s.append(i)
          }
        }
        r++
      }

    }

    //gui.add_message_at(our_player, "  tile d " + d + " * " + coord3d_to_string(t), world.get_time())

  }

    if ( print_message == 1 && print_message_box > 0 ) {
      for ( local j = 0; j < message_text.len(); j++ ) {
        gui.add_message_at(our_player, message_text[j], world.get_time())
      }
    }

  if ( sign == c - 1 && c > 2 ) {
    //gui.add_message_at(our_player, (c-1) + " double way found, no build " + c, world.get_time())
    return true
  }

  if ( start_fields_id.len() <= c ) {
    for ( local i = 0; i < start_fields_id.len(); i++ ) {
      start_fields.append(nexttile[start_fields_id[i]])
    }
  } else {
    // start_fields selected
    if ( c == 2 ) {
      if ( start_fields_id.len() >= 5 ) {
        start_fields.append(nexttile[start_fields_id[1]])
        start_fields.append(nexttile[start_fields_id[start_fields_id.len()-2]])
      } else {
        start_fields.append(nexttile[start_fields_id[0]])
        start_fields.append(nexttile[start_fields_id[start_fields_id.len()-1]])
      }
    } else {
     local a = (l / (c + 1)).tointeger()
     //gui.add_message_at(our_player, " a " + a, world.get_time())

     local l1 = 0
     local l2 = 0

     for ( local i = 0; i < c; i++ ) {
      //gui.add_message_at(our_player, " (a * (i+1)) " + (a * (i+1)), world.get_time())
      for ( local j = 1; j < start_fields_id.len(); j++ ) {
        if ( start_fields_id[j-1] < (a * (i+1)) ) {
          //l1 = (a * c) - start_fields_id[j-1]
          if ( start_fields_id.len() < i && start_fields_id[j] < (a * (i+1)) && start_fields_id[j+1] > (a * (i+1)) ) {
            //l1 = j
            if ( ((a * (i+1)) - start_fields_id[j+1]) < (start_fields_id[j] - (a * (i+1))) ) {
              l1 = j+1

            } else {
              l1 = j

            }

          } else {
            if ( ((a * (i+1)) - start_fields_id[j-1]) < (start_fields_id[j] - (a * (i+1))) ) {
              l1 = j-1

            } else {
              l1 = j
              //break
            }
            //
          }

        }/* else {
          l1 = j-1
          break
        }*/
        if ( l1 > 0 && start_fields_id.len() > j && (l1 - l2) < (a-10)  ) {

        } else if ( l1 > 0 ) { break }
      }

      //if ( l1 > l2 ) {

      //} else {

        start_fields.append(nexttile[start_fields_id[l1]])
        //gui.add_message_at(our_player, " start_fields.append() " + start_fields_id[l1], nexttile[start_fields_id[l1]])
        l2 = l1
        l1 = 0
      //}
     }

    }
    //gui.add_message_at(our_player, " start_fields.len() " + start_fields.len(), world.get_time())
  }

  return start_fields

}

function optimize_way_line(route, wt, int_run, o_line) {

  // 0 = off
  // 1 = bridges
  // 2 = tunnel
  // 3 = crossing
  // 4 = terraform
  local print_message_box = 0

  if ( print_message_box > 5 ) { //wt == wt_road && wt == wt_rail &&
    gui.add_message_at(our_player, " optimize_way_line(route, wt) ", tile_x(route[0].x, route[0].y, route[0].z))
  }
  //::debug.pause()

  local speed = tile_x(route[0].x, route[0].y, route[0].z).find_object(mo_way).get_desc().get_topspeed()
  local bridge_obj = find_object("bridge", wt, speed)
  local tunnel_obj = find_object("tunnel", wt, speed)

  local count_build = 0

  local stations_awst = null
  local stations_awst_2 = []

  // count station len start
  local station_len = 0
  for ( local c = 0; c < 50; c++ ) {
    if ( route[c].find_object(mo_building) != null ) {
      station_len++
    } else {
      break
    }
  }
  if ( station_len == 1 ) {
    station_len = 3
  }

  //gui.add_message_at(our_player, " found bridge " + bridge_obj.get_name() + " tunnel " + tunnel_obj.get_name(), world.get_time())

  for ( local i = 1; i < (route.len() - station_len); i++ ) {
    local tile_1 = tile_x(route[i-1].x, route[i-1].y, route[i-1].z)
    local tile_2 = tile_x(route[i].x, route[i].y, route[i].z)
    local tile_3 = tile_x(route[i+1].x, route[i+1].y, route[i+1].z)
    local tile_4 = null
    local tile_1_d = tile_1.get_way_dirs(wt)
    local tile_2_d = tile_2.get_way_dirs(wt)
    local tile_3_d = tile_3.get_way_dirs(wt)
    local tile_4_d = 0

    local tile_2_speed = 0

    // START :: remove diagonal double ways without spacing
    if ( dir.is_threeway(tile_1_d)  &&  dir.is_threeway(tile_3_d) ) {
      tile_2_speed = tile_2.find_object(mo_way).get_desc().get_topspeed()
      local remove_tile = null
      switch(tile_2_d) {
        case 3:
          tile_4 = tile_x(tile_2.x+1, tile_2.y-1, tile_2.z)
          if ( tile_4.find_object(mo_way) != null ) {
            tile_4_d = tile_4.get_way_dirs(wt)
            if ( tile_2_speed >= tile_4.find_object(mo_way).get_desc().get_topspeed() && tile_4_d == 12 ) {
              remove_tile = tile_4
            } else if ( tile_4_d == 12 ) {
              remove_tile = tile_2
            }
          }
          break
        case 6:
          tile_4 = tile_x(tile_2.x+1, tile_2.y+1, tile_2.z)
          if ( tile_4.find_object(mo_way) != null ) {
            tile_4_d = tile_4.get_way_dirs(wt)
            if ( tile_2_speed >= tile_4.find_object(mo_way).get_desc().get_topspeed() && tile_4_d == 9 ) {
              remove_tile = tile_4
            } else if ( tile_4_d == 9 ) {
              remove_tile = tile_2
            }
          }
          break
        case 9:
          tile_4 = tile_x(tile_2.x-1, tile_2.y-1, tile_2.z)
          if ( tile_4.find_object(mo_way) != null ) {
            tile_4_d = tile_4.get_way_dirs(wt)
            if ( tile_2_speed >= tile_4.find_object(mo_way).get_desc().get_topspeed() && tile_4_d == 6 ) {
              remove_tile = tile_4
            } else if ( tile_4_d == 6 ) {
              remove_tile = tile_2
            }
          }
          break
        case 12:
          tile_4 = tile_x(tile_2.x-1, tile_2.y+1, tile_2.z)
          if ( tile_4.find_object(mo_way) != null ) {
            tile_4_d = tile_4.get_way_dirs(wt)
            if ( tile_2_speed >= tile_4.find_object(mo_way).get_desc().get_topspeed() && tile_4_d == 3 ) {
              remove_tile = tile_4
            } else if ( tile_4_d == 3 ) {
              remove_tile = tile_2
            }
          }
          break
      }


      if ( remove_tile != null ) {
        remove_tile_to_empty(remove_tile, wt, 0)
      }
    }
    // END :: remove diagonal double ways without spacing


    // START :: check station in line
    if ( tile_2.find_object(mo_building) != null && i > station_len && i < (route.len() - station_len) && wt == wt_rail && stations_awst == null ) {
      // station in line

      // set line.way_line_count
      o_line.way_line_count = 2
      local li = tile_2.get_halt().get_line_list()

      // todo line_x -> my_line_t
      local s_line = select_linkline(li[0])
      s_line.way_line_count = 2

      /*
      local entries = r_line.get_schedule().entries
      if ( entries.len() == 2 ) {
        local sched = schedule_x(wt_rail, [])
        sched.entries.append( schedule_entry_x(entries[0], 100, 0) );
        sched.entries.append( schedule_entry_x(t, 0, 0) );
        sched.entries.append( schedule_entry_x(entries[entries.len()-1], 0, 0) );
        r_line.change_schedule(our_player, sched);
      }*/
      local t = null
      local r = null
      for (local j = 1; j < station_len; j++ ) {
        t = route[i+j]
        //gui.add_message_at(our_player, "st_awst ", t)
        if (t.find_object(mo_building) == null) {
          r = j
          break
        }
      }
      if ( (t.x == tile_2.x && t.y < tile_2.y) || (t.y == tile_2.y && t.x < tile_2.x) ) {
        stations_awst = t
        r = i + 9
      } else if ( (t.x == tile_2.x && t.y > tile_2.y) || (t.y == tile_2.y && t.x > tile_2.x) ) {
        stations_awst = tile_1
        r = i - 9
      }

      for (local j = 1; j < 9; j++ ) {
        stations_awst_2.append(route[r+j])

      }

    }
    // END :: check station in line

    // START :: build bridges and tunnel - 2 to 4 tiles
    local build_bridge = 0
    local build_tunnel = 0
    local build_tile = null

    tile_4 = tile_x(route[i+2].x, route[i+2].y, route[i+2].z)
    tile_4_d = tile_4.get_way_dirs(wt)

    // tile 1 - 3 direction 5 or 10 -> way_d = 1
    // tile 1 - 4 direction 5 or 10 -> way_d = 2
    local way_d = 0
    if ( tile_1_d == 5 || tile_1_d == 10 ) {
      if ( tile_1_d == tile_2_d && tile_2_d == tile_3_d && tile_3_d == tile_4_d && tile_1.z == tile_4.z && tile_1.get_slope() > 0 && tile_4.get_slope() > 0 ) {
        if ( our_player.get_current_cash() > 1000000 ) {
          if ( tile_1.z == tile_2.z && tile_2.z == tile_3.z && tile_1.is_bridge() != true ) {
            build_bridge = 2
            build_tile = tile_4
          } else if ( tile_1.z < tile_2.z && tile_2.z == tile_3.z ) {
            build_tunnel = 3
            build_tile = tile_4
          }
        }
      } else if ( tile_1_d == tile_2_d &&  tile_2_d == tile_3_d && tile_1.z == tile_3.z && tile_1.get_slope() > 0 && tile_3.get_slope() > 0 ) {
        if ( our_player.get_current_cash() > 500000 ) {
          if ( tile_1.z == tile_2.z && tile_2.z == tile_3.z && tile_1.is_bridge() != true ) {
            build_bridge = 2
            build_tile = tile_3
          } else if ( tile_1.z < tile_2.z ) {
            build_tunnel = 3
            build_tile = tile_3
          }
        }
      } else if ( tile_1_d == tile_2_d && tile_1.z == tile_2.z && tile_1.get_slope() > 0 && tile_2.get_slope() > 0 && tile_1.is_bridge() != true && tile_2.is_bridge() != true ) {
        // slope down - slope up -> bridge
        // slope up - slope down -> terraform down ( build_tunnel = 1 )
        if ( (tile_1.get_slope() == 4 && tile_2.get_slope() == 36 && tile_1.y < tile_2.y) || (tile_1.get_slope() == 36 && tile_2.get_slope() == 4 && tile_1.y > tile_2.y)  ) {
          build_tunnel = 1
        } else if ( (tile_1.get_slope() == 12 && tile_2.get_slope() == 28 && tile_1.x < tile_2.x) || (tile_1.get_slope() == 28 && tile_2.get_slope() == 12 && tile_1.x > tile_2.x)  ) {
          build_tunnel = 1
        } else if ( (tile_1.get_slope() == 4 && tile_2.get_slope() == 36 && tile_1.y > tile_2.y) || (tile_1.get_slope() == 36 && tile_2.get_slope() == 4 && tile_1.y < tile_2.y) ) {
          build_bridge = 1
        } else if ( (tile_1.get_slope() == 12 && tile_2.get_slope() == 28 && tile_1.x > tile_2.x) || (tile_1.get_slope() == 28 && tile_2.get_slope() == 12 && tile_1.x < tile_2.x) ) {
          build_bridge = 1
        }

        build_tile = tile_2
      }

      // check is way our player
      local tile_1_pl = tile_1.find_object(mo_way).get_owner().nr //.get_desc()
      local tile_2_coord = coord3d_to_string(tile_2) // not use for debug
      local tile_2_pl = tile_2.find_object(mo_way).get_owner().nr //.get_desc()
      if ( tile_1_pl != our_player_nr || tile_2_pl != our_player_nr ) {
        continue
      }


      if ( (way_d > 0 || build_bridge > 0) && print_message_box > 0 ) { //
        //gui.add_message_at(our_player, " optimize way way_d " + way_d, tile_1)
        gui.add_message_at(our_player, " optimize way build_bridge " + build_bridge, tile_1)
      }
    }


    if ( build_bridge > 0 && print_message_box == 1 ) {
      gui.add_message_at(our_player, " optimize way build_bridge " + build_bridge, tile_1)
      //::debug.pause()
    }
    if ( build_tunnel > 0 && print_message_box == 2 ) {
      gui.add_message_at(our_player, " optimize way build_tunnel " + build_tunnel, tile_1)
      //::debug.pause()
    }

    // way
    local way_obj = route[0].find_object(mo_way).get_desc() //way_list[0]
    if ( !way_obj.is_available(world.get_time()) ) {
      way_obj = find_object("way", wt, way_obj.get_topspeed())
    }
    // catenary
    local catenary_obj = null
    if ( tile_1.find_object(mo_wayobj) != null ) {
      catenary_obj = tile_1.find_object(mo_wayobj).get_desc()
      if ( catenary_obj != null && !catenary_obj.is_available(world.get_time()) ) {
        catenary_obj = find_object("catenary", wt, catenary_obj.get_topspeed())
      }
    }

      // slope up - slope down -> tunnel
      if ( build_tunnel == 1 ) {
        local step_ok = true
        local err = null
        // not build tunnel -> set slope down
        local tile_4 = tile_x(route[i-2].x, route[i-2].y, route[i-2].z)
        tile_4_d = tile_4.get_way_dirs(wt)
        /*if ( tile_4.find_object(mo_building) != null || tile_4.find_object(mo_bridge) != null ) { //dir.is_single(tile_4_d)
          local tool = command_x(tool_remover)
          err = tool.work(our_player, tile_3)
          if ( err == null ) { err = tool.work(our_player, tile_2) }
        } else {
          local tool = command_x(tool_remove_way)
          local err = tool.work(our_player, tile_3, tile_4, "" + wt)
        }*/
        if ( tile_3.find_object(mo_bridge) == null && tile_4.find_object(mo_bridge) == null && tile_4.find_object(mo_building) == null && tile_3.find_object(mo_building) == null ) {
          local tool = command_x(tool_remove_way)
          err = tool.work(our_player, tile_4, tile_3, "" + wt)
        } else {
          remove_tile_to_empty(tile_1, wt, 0)
          remove_tile_to_empty(tile_2, wt, 0)
        }

        local way_obj = tile_4.find_object(mo_way).get_desc()
        if ( !way_obj.is_available(world.get_time()) ) {
          way_obj = find_object("way", wt, speed)
        }

        if ( err != null ) {
          // tile not remove -> restore way
          err = command_x.build_way(our_player, tile_4, tile_3, way_obj, true)
          step_ok = false
        }

        if ( step_ok ) {
          err = null
          // terraform down
          /*local t = []
          t.append(tile_1)
          t.append(tile_2)*/
          if ( check_tiles_for_terraform([tile_1, tile_2]) ) {
            // tiles left and right free -> terraform grid

            local cx = 0
            local cy = 0
            local terraform_tile = null
            if ( tile_1.y == tile_2.y ) {
              cy = 1
              if ( tile_1.x > tile_2.x ) {
                terraform_tile = tile_1
              } else {
                terraform_tile = tile_2
              }
            } else if ( tile_1.x == tile_2.x ) {
              cx = 1
              if ( tile_1.y > tile_2.y ) {
                terraform_tile = tile_1
              } else {
                terraform_tile = tile_2
              }
            }

            err = command_x.grid_lower(our_player, coord3d(terraform_tile.x, terraform_tile.y, terraform_tile.z))

            if ( print_message_box == 4 ) {
              gui.add_message_at(our_player, " terraform terraform_tile " + coord3d_to_string(terraform_tile) + " : " + err, world.get_time())
            }

            err = null
            err = command_x.grid_lower(our_player, coord3d(terraform_tile.x+cx, terraform_tile.y+cy, terraform_tile.z))

            if ( print_message_box == 4 ) {
              gui.add_message_at(our_player, " terraform terraform_tile " + coord3d_to_string(tile_x(terraform_tile.x+cx, terraform_tile.y+cy, terraform_tile.z)) + " : " + err, world.get_time())
            }
          } else {
            // tiles left and right not free -> terraform tile

            err = command_x.set_slope(our_player, tile_1, 83)

            err = command_x.set_slope(our_player, tile_2, 83)
          }

          // check ground is water after terraform
          if ( tile_1.is_water() ) {
            command_x.change_climate_at(our_player, tile_1, cl_temperate)
            remove_tile_to_empty(tile_2, wt, 0)
          }
          if ( tile_2.is_water() ) {
            command_x.change_climate_at(our_player, tile_2, cl_temperate)
            remove_tile_to_empty(tile_1, wt, 0)
            command_x.change_climate_at(our_player, tile_1, cl_temperate)
          }

          err = null
          err = command_x.build_way(our_player, tile_4, tile_3, way_obj, true)
          if (err != null ) {
            gui.add_message_at(our_player, " build tunnel " + coord3d_to_string(tile_4) + " - " + coord3d_to_string(tile_3) + ": " + err, world.get_time())
          } else {
            count_build++
          }

        }

        // restor exist catenary
        if ( catenary_obj != null ) {
          command_x.build_wayobj(our_player, tile_4, tile_3, catenary_obj)
        }
      } else if ( build_tunnel == 2 ) {
        err = null
        // build tunnel - not work tunnel tool script ai
        //local tile_4 = tile_x(route[i-2].x, route[i-2].y, route[i-2].z)
        local txt = coord3d_to_string(tile_1)
        local tool = command_x(tool_remove_way)
        local err = tool.work(our_player, tile_1, build_tile, "" + wt)

        err = command_x.build_tunnel_at(our_player, tile_1, tunnel_obj)
        if (err != null ) {
          gui.add_message_at(our_player, " build tunnel: " + err, world.get_time())
        } else {
          count_build++
        }
        // restor exist catenary
        if ( catenary_obj != null ) {
          command_x.build_wayobj(our_player, tile_1, build_tile, catenary_obj)
        }
      } else if ( build_tunnel == 3 ) {
        // build tunnel - not work tunnel tool script ai
        /*local tile_4 = tile_x(route[i-2].x, route[i-2].y, route[i-2].z)
        local txt = coord3d_to_string(tile_1)
        remove_tile_to_empty(tile_2, wt, 0)
        local err = command_x.build_tunnel_at(our_player, tile_1, tunnel_obj)
        if (err != null ) {
          gui.add_message_at(our_player, " build tunnel: " + err, world.get_time())
        } else {
          count_build++
        }*/
      }

      // slope down - slope up -> bridge
      if ( build_bridge == 1 ) {
        local err = null
        local tile_4 = tile_x(route[i-2].x, route[i-2].y, route[i-2].z)
        //local err = remove_tile_to_empty(tile_2, wt, 0)


        if ( tile_3.find_object(mo_bridge) == null && tile_4.find_object(mo_bridge) == null && tile_4.find_object(mo_building) == null && tile_3.find_object(mo_building) == null ) {
          local tool = command_x(tool_remove_way)
          err = tool.work(our_player, tile_4, tile_3, "" + wt)
        } else {
          err = remove_tile_to_empty(tile_1, wt, 0)
          if ( err ) {
            err = remove_tile_to_empty(tile_2, wt, 0)
          }
          if ( err ) {
            err = null
          } else {
            continue
          }
        }

        if ( print_message_box == 4 && err != null ) {
          gui.add_message_at(our_player, "#4376# remove tile_1 " + coord3d_to_string(tile_1) + " : " + err, world.get_time())
          gui.add_message_at(our_player, "#4376# remove tile_2 " + coord3d_to_string(tile_2) + " : " + err, world.get_time())
          //::debug.pause()
        }

        if (err == null) {
          //err = null
          /*local t = []
          t.append(tile_1)
          t.append(tile_2)*/
          if ( check_tiles_for_terraform([tile_1, tile_2]) ) {
            // tiles left and right free -> terraform grid

              local cx = 0
              local cy = 0
              local terraform_tile = null
              if ( tile_1.y == tile_2.y ) {
                cy = 1
                if ( tile_1.x > tile_2.x ) {
                  terraform_tile = tile_1
                } else {
                  terraform_tile = tile_2
                }
              } else if ( tile_1.x == tile_2.x ) {
                cx = 1
                if ( tile_1.y > tile_2.y ) {
                  terraform_tile = tile_1
                } else {
                  terraform_tile = tile_2
                }
              }

            err = command_x.grid_raise(our_player, coord3d(terraform_tile.x, terraform_tile.y, terraform_tile.z))
            //::debug.pause()
            if (err != null ) {
              if ( print_message_box == 4 ) {
                gui.add_message_at(our_player, "#4390# terraform fail -> build bridge : " + err, tile_1)
                ::debug.pause()
              }
              err = command_x.build_bridge(our_player, tile_1, build_tile, bridge_obj)

            } else {
              //gui.add_message_at(our_player, "#4344# terraform_tile : " + coord3d_to_string(terraform_tile), terraform_tile)
              err = command_x.grid_raise(our_player, coord3d(terraform_tile.x+cx, terraform_tile.y+cy, terraform_tile.z))


              if ( err == null ) {
                err = command_x.build_way(our_player, tile_4, tile_3, way_obj, true)
                if (err != null ) {
                  gui.add_message_at(our_player, "#4350# build way " + coord3d_to_string(tile_4) + " - " + coord3d_to_string(tile_3) + ": " + err, world.get_time())
                } else {
                  count_build++
                }
              } else {
                gui.add_message_at(our_player, "#4403# err : " + err, tile_1)
              }
            }

          } else {
            // tiles left and right not free -> build bridge

            err = command_x.build_bridge(our_player, tile_1, build_tile, bridge_obj)

          }


        }

        // restor exist catenary
        if ( catenary_obj != null ) {
          command_x.build_wayobj(our_player, tile_1, build_tile, catenary_obj)
        }
      } else if ( build_bridge == 2 ) {
        local tool = command_x(tool_remove_way)
        local err = tool.work(our_player, tile_1, build_tile, "" + wt)
            //gui.add_message_at(our_player, " remove way: " + err, tile_1)
        if (err == null) {
          err = command_x.build_bridge(our_player, tile_1, build_tile, bridge_obj)
          if (err != null ) {
            gui.add_message_at(our_player, " build bridge: " + err, tile_1)
            // restore way
            err = command_x.build_way(our_player, tile_1, build_tile, way_obj, true)
          } else {
            count_build++
          }
        }
        // restor exist catenary
        if ( catenary_obj != null ) {
          command_x.build_wayobj(our_player, tile_1, build_tile, catenary_obj)
        }
      }
    // END :: build bridges and tunnel - 2 to 4 tiles

    // START :: remove obsolete bridges
    if ( tile_2.is_bridge() && tile_1.z == tile_2.z && tile_2.get_slope() == 0 ) {
      local tile_way = tile_2.find_object(mo_bridge)
      local pl_check = tile_way.get_owner().nr

      local remove_bridge = 0
      local bridge_len = 0
      if ( tile_2_d == 5 && pl_check == our_player_nr ) {
        local t_tile = tile_2
        if ( tile_2.y > tile_1.y ) {
          local check_gr = tile_x(t_tile.x, t_tile.y+1, t_tile.z)
          for ( local x = 1; route[i].is_bridge(); x++ ) {
            if ( check_gr.is_ground() && check_gr.is_empty() ) {
              remove_bridge++
            }
            bridge_len = x
            i++

            check_gr = square_x(route[i].x, route[i].y).get_ground_tile()
          }

        } else {
          local check_gr = tile_x(t_tile.x, t_tile.y-1, t_tile.z)
          for ( local x = 1; route[i].is_bridge(); x++ ) {
            if ( check_gr.is_ground() && check_gr.is_empty() ) {
              remove_bridge++
            }
            bridge_len = x
            i++

            check_gr = square_x(route[i].x, route[i].y).get_ground_tile()
          }

        }
      } else if ( tile_2_d == 10 && pl_check == our_player_nr ) {
        local t_tile = tile_2
        if ( tile_2.x > tile_1.x ) {
          local check_gr = tile_x(t_tile.x+1, t_tile.y, t_tile.z)
          for ( local x = 1; route[i].is_bridge(); x++ ) {
            if ( check_gr.is_ground() && check_gr.is_empty() ) {
              remove_bridge++
            }
            bridge_len = x
            i++

            check_gr = square_x(route[i].x, route[i].y).get_ground_tile()
          }

        } else {
          local check_gr = tile_x(t_tile.x-1, t_tile.y, t_tile.z)
          for ( local x = 1; route[i].is_bridge(); x++ ) {
            if ( check_gr.is_ground() && check_gr.is_empty() ) {
              remove_bridge++
            }
            bridge_len = x
            i++

            check_gr = square_x(route[i].x, route[i].y).get_ground_tile()
          }

        }
      }
      if ( bridge_len == 4 && print_message_box == 1 ) {
        gui.add_message_at(our_player, " test bridge : bridge_len = " + bridge_len + " : remove_bridge = " + remove_bridge, tile_1)
        gui.add_message_at(our_player, " tile_4 " + coord3d_to_string(tile_4), world.get_time())
        gui.add_message_at(our_player, " build_tile " + coord3d_to_string(tile_x(route[i-1].x, route[i-1].y, route[i-1].z)), world.get_time())
        ::debug.pause()
        //sleep()
      }


      if ( remove_bridge > 0 && ( (remove_bridge+1) == bridge_len ) && bridge_len > 2 && bridge_len < 5 && pl_check == our_player_nr ) {
        build_tile = tile_x(route[i-1].x, route[i-1].y, route[i-1].z)
        if ( tile_2.z == build_tile.z ) {

          local err = remove_tile_to_empty(tile_2, wt, 0)
          if (err != null) { gui.add_message_at(our_player, " remove bridge: " + err, tile_1) }

          if (err) {
            err = check_ground(tile_2, build_tile, way_obj)

            build_tile = tile_x(route[i].x, route[i].y, route[i].z)

            err = command_x.build_way(our_player, tile_1, build_tile, way_obj, true)
            if (err != null ) {
              gui.add_message_at(our_player, " not build way: " + err, tile_1)
              // restore bridge
              err = command_x.build_bridge(our_player, tile_2, build_tile, bridge_obj)
            } else {
              count_build++
            }
          }

        } else {
          local err = check_ground(tile_2, build_tile, way_obj)
          if (!err) {
            err = command_x.build_way(our_player, tile_1, tile_2, way_obj, true)
          } else {
            err = command_x.build_bridge(our_player, tile_2, build_tile, bridge_obj)
          }
        }
      }
    }
    // END :: remove obsolete bridges

    // START :: crossing
    // replace crossing to road bridge
    local check_crossing = tile_2.find_object(mo_crossing)
    if ( check_crossing != null ) {
      local tile_way = [tile_1.find_object(mo_way), tile_3.find_object(mo_way)]
      local pl_check = [tile_way[0].get_owner().nr, tile_way[1].get_owner().nr]
      local build_check = 0
      if ( (pl_check[0] == our_player_nr || pl_check[0] == 1) && (pl_check[1] == our_player_nr || pl_check[1] == 1) ) {
        // no bridge by bridge and tunnel
        if ( !tile_1.is_bridge() && !tile_3.is_bridge() && !tile_1.is_tunnel() && !tile_3.is_tunnel() ) {
          build_check = 1
        }
      }

      if ( wt == wt_road && build_check == 1 && world.get_time().year >= 1935 ) {
        if ( (tile_1_d == 5 && tile_3_d == 5) || (tile_1_d == 10 && tile_3_d == 10)  ) {
          if ( print_message_box == 3 ) {
            gui.add_message_at(our_player, " test crossing = " + check_crossing, tile_2)
            gui.add_message_at(our_player, " find way = " + tile_2.find_object(mo_way), tile_2)
            gui.add_message_at(our_player, " way name = " + tile_2.find_object(mo_way).get_name() + " cnv count : " + tile_2.find_object(mo_way).get_convoys_passed()[1], tile_2)
            //gui.add_message_at(our_player, " crossing way = " + check_crossing., tile_2)
            //::debug.pause()
          }

          local cnv_count = tile_2.find_object(mo_way).get_convoys_passed()[1]
          if ( cnv_count > 100 ) {
            local tool = command_x(tool_remove_way)
            local err = tool.work(our_player, tile_1, tile_3, "" + wt)
            //gui.add_message_at(our_player, " remove way: " + err, tile_1)
            if (err == null) {
              err = command_x.build_bridge(our_player, tile_1, tile_3, bridge_obj)
              if (err != null ) {
                gui.add_message_at(our_player, " build bridge: " + err, tile_1)
              } else {
                count_build++
              }
            }
          }
        }
      }
    }
    // END :: crossing
  }

  // build stations awst
  if ( stations_awst != null ) {
    station_aw(stations_awst, wt, stations_awst_2)
  }

  if (count_build > 0 ) {
    local cs = tile_x(route[route.len()-1].x, route[route.len()-1].y, route[route.len()-1].z)//route[route.len()]
    local ce = tile_x(route[0].x, route[0].y, route[0].z)//route[0]

    local cs_coord = coord_to_string(cs)
    local ce_coord = coord_to_string(ce)

    local msgtext = format(translate("%s optimize way line from %s (%s) to %s (%s)"), our_player.get_name(), cs.get_halt().get_name(), coord_to_string(cs), ce.get_halt().get_name(), coord_to_string(ce))
    gui.add_message_at(our_player, msgtext, cs)

    return true
  } else {
    return false
  }

}

/*
 * check tiles free for terraform
 *
 * tiles = array tiles for check tiles left and right is free
 */
function check_tiles_for_terraform(tiles) {
  local tiles_free = true
  local cx = 0
  local cy = 0
  if ( tiles[0].x == tiles[1].x ) {
    cx = 1
  } else if ( tiles[0].y == tiles[1].y ) {
    cy = 1
  }
  for ( local x = 0; x < tiles.len(); x++ ) {
    //gui.add_message_at(our_player, "test_tile_is_empty tiles[x]+ " + test_tile_is_empty(tile_x(tiles[x].x+cx, tiles[x].y+cy, tiles[x].z)), tiles[x])
    //gui.add_message_at(our_player, "test_tile_is_empty tiles[x]- " + test_tile_is_empty(tile_x(tiles[x].x-cx, tiles[x].y-cy, tiles[x].z)), world.get_time())
    if ( !test_tile_is_empty(tile_x(tiles[x].x+cx, tiles[x].y+cy, tiles[x].z)) || !test_tile_is_empty(tile_x(tiles[x].x-cx, tiles[x].y-cy, tiles[x].z)) ) {
      tiles_free = false
    }
  }

  return tiles_free
}

/*
 * check double ways in new line
 * waytype: wt_rail
 *
 *
 */
function check_doubleway_in_line(route, wt) {
  //gui.add_message_at(our_player, " check_doubleway_in_line(route, wt) ", world.get_time())

  local treeway_tiles = []
  local nexttile = []

  local t = 0 // count treeway tile to signal tile

  foreach(node in route.routes) {
    local tile = tile_x(node.x, node.y, node.z)
    nexttile.append(tile)
    // check route to treeways

    if ( t > 0 ) { t++ }

    // treeway
    if ( dir.is_threeway(tile.get_way_dirs(wt)) ) {
      treeway_tiles.append(tile)
      t++
      if ( t <= 6  ) { t = 1 }
    }

    // test signals by rail
    if ( tile.find_object(mo_signal) != null && t < 6 ) {
      // connect in double way
      gui.add_message_at(our_player, " ## WARNING ## connect in double way ", tile)

    } else if ( t >= 6  ) {
      t = 0
    }
  }


  //::debug.pause()

    //local asf = astar_route_finder(wt)
    //wayline = asf.search_route([start_l], [end_l])


}


/*
 *  destroy line complete
 *  - destroy convoy
 *  - remove line
 *  check shedule/station connections befor
 *  - destroy stations
 *  - destroy depot by rail/ship
 *  - destroy way line
 */
function destroy_line(line_obj, good, link_obj) {

  if ( debug ) ::debug.set_pause_on_error(true)

  // 1 = messages
  // 2 = debug.pause()
  // 3 = line check
  local print_message_box = 0

  if ( print_message_box > 0 ) {
    gui.add_message_at(our_player, "+ destroy_line(line_obj, good, link) start line " + line_obj.get_name(), world.get_time())
  }

  local line_name = line_obj.get_name()
  local cnv_list = line_obj.get_convoy_list()
  local cnv_count = cnv_list.get_count()
  local wt = line_obj.get_waytype()

  local cnv = null
  local depot = null
  local depot_t = null

  local start_l = null
  local end_l = null
  local start_h = null
  local end_h = null

  local nexttile = []

  {
    local entries = line_obj.get_schedule().entries
    local entries_count = entries.len()

    if ( wt == wt_water ) {
      if ( entries.len() >= 2 ) {
        start_h = entries[0].get_halt(our_player)
        end_h = entries[entries.len()-1].get_halt(our_player)
        local t = start_h.get_tile_list()
        start_l = tile_x(t[0].x, t[0].y, t[0].z)
        t = end_h.get_tile_list()
        end_l = tile_x(t[0].x, t[0].y, t[0].z)
      }
    } else {
      if ( entries.len() >= 2 ) {
        start_l = tile_x(entries[0].x, entries[0].y, entries[0].z)
        end_l = tile_x(entries[entries.len()-1].x, entries[entries.len()-1].y, entries[entries.len()-1].z)
        start_h = start_l.get_halt()
        end_h = end_l.get_halt()
      }
    }

  }
  if ( print_message_box == 1 ) {
    gui.add_message_at(our_player, "  " + line_obj.get_name(), world.get_time())
  }

  local start_line_count = start_h.get_line_list().get_count()
  local end_line_count = end_h.get_line_list().get_count()

  local combined_s = test_halt_waytypes(start_l)
  local combined_e = test_halt_waytypes(end_l)

  if ( start_line_count >= 2 && end_line_count >= 2 && combined_s > 1 && combined_e > 1  ) {
    return false
  }

    local start_f = null
    local end_f = null
    //if ( wt != wt_water ) {
    start_f = start_h.get_factory_list()
    end_f = end_h.get_factory_list()
    //}
    if ( start_f.len() > 0 && (print_message_box == 1 || print_message_box == 3) ) {
      gui.add_message_at(our_player, " factory start " + start_f[0].get_name(), start_l)
    } else if ( print_message_box == 1 || print_message_box == 3 ) {
      gui.add_message_at(our_player, " not connect factory start ", world.get_time())
    }
    if ( end_f.len() > 0 && (print_message_box == 1 || print_message_box == 3) ) {
      gui.add_message_at(our_player, " factory end " + end_f[0].get_name(), end_l)
    } else if ( print_message_box == 1 || print_message_box == 3 ) {
      gui.add_message_at(our_player, " not connect factory end ", world.get_time())
    }

    local chk_link = 0
    if ( start_f.len() > 0 && end_f.len() > 0 ) {
      chk_link = check_factory_links(start_f[0], end_f[0], good.get_name())
    }

    //gui.add_message_at(our_player, "### check_factory_links(start_f[0], end_f[0], good.get_name()) " + check_factory_links(start_f[0], end_f[0], good.get_name()), world.get_time())
    // check links
    if ( combined_s == 1 && combined_e == 1 && chk_link == 1 ) {

      local good_list_in = [];
      local g_count_in = 0
      foreach(good, islot in start_f[0].input) {

        // test for in-storage or in-transit goods
        local st = islot.get_storage()
        local it = islot.get_in_transit()
        //gui.add_message_at(our_player, "### " + fab.get_name() + " ## " + good + " ## get_storage() " + st[0] + " get_in_transit() " + it[0], world.get_time())
        if (st[0] + st[1] + it[0] + it[1] > 0) {
          // something stored/in-transit in last and current month
          // no need to search for more supply
          good_list_in.append({ g = good, t = 1 })
          g_count_in++
        } else {
          good_list_in.append({ g = good, t = 0 })
        }
      }

      if (good_list_in.len() == g_count_in ) {
        if ( print_message_box == 1 || print_message_box == 3 ) {
          gui.add_message_at(our_player, "### last line connect factorys - stored/in-transit all goods > 0 factory start", world.get_time())
        }
        if ( start_f[0].get_suppliers().len() == 0 && end_f[0].get_consumers().len() == 0 ) {
          if ( print_message_box == 1 || print_message_box == 3 ) {
            gui.add_message_at(our_player, "### factory start generator && factory end  end-consumers", world.get_time())
          }
        } else {
          // 12 month no output goods -> line remove
          local output_count = 0

          local list_halts_dest = end_f[0].get_halt_list()
          // check conected halts
          if ( list_halts_dest.len() == 1 ) {
            // one halt remove link
            if ( print_message_box == 1 || print_message_box == 3 ) {
              gui.add_message_at(our_player, "### factory end : count halts = " + list_halts_dest.len(), world.get_time())
            }
            //return true
          } else {

          /*foreach(good, islot in end_f[0].output) {

            // test for in-storage or in-transit goods
            local st = end_f[0].get_desc().get_outputs()
            foreach(t in st) {
              gui.add_message_at(our_player, "### foreach(t in st) - t.keys() " + t.keys(), world.get_time())
              local tt = t.keys()
              for (local i = 0; i < tt.len(); i++) {
                gui.add_message_at(our_player, "**** keys() - tt["+i+"] " + tt[i], world.get_time())
              }
              tt.clear()
              tt = t.values()
              for (local i = 0; i < tt.len(); i++) {
                if ( i == 2 ) {
                  gui.add_message_at(our_player, "**** values() - tt["+i+"].get_name() " + tt[i].get_name(), world.get_time())
                } else {
                  gui.add_message_at(our_player, "**** values() - tt["+i+"] " + tt[i], world.get_time())
                }
              }
              //output_count += t
            }
          }*/
            if ( print_message_box == 1 || print_message_box == 3 ) {
              gui.add_message_at(our_player, "### factory end : count outputs good = " + output_count, world.get_time())
            }


            return false

          }
        }
      }
    }

  if (cnv_list.get_count() > 0 ) {
    foreach(cnv in cnv_list) {
      if ( depot == null ) {
        depot = tile_x(cnv.get_home_depot().x, cnv.get_home_depot().y, cnv.get_home_depot().z)
        if ( print_message_box == 1 ) {
          gui.add_message_at(our_player, "depot " + coord3d_to_string(depot), world.get_time())
        }
      }
      // mark convoy for destroying
      local cnv_trav_month = cnv.get_traveled_distance()
      local cnv_trav_month_count = 0
      for (local i = 0; i < cnv_trav_month.len(); i++ ) {
        cnv_trav_month_count += cnv_trav_month[i]
      }
        if ( print_message_box == 3 ) {
          gui.add_message_at(our_player, "cnv_trav_month_count " + cnv_trav_month_count, world.get_time())
          gui.add_message_at(our_player, "cnv.get_distance_traveled_total() " + cnv.get_distance_traveled_total(), world.get_time())
        }

      if ( cnv.get_distance_traveled_total() < 3 && cnv_trav_month_count == cnv.get_distance_traveled_total() && !cnv.is_loading() && (link_obj != null && check_fsrc_input(link_obj.f_src)) ) {
        if ( print_message_box == 3 ) {
          gui.add_message_at(our_player, "return cnv/line new", world.get_time())
        }
        return
      }
      if ( wt != wt_water && combined_s > 1 && start_line_count > 1 ) {
        if ( print_message_box == 1 ) {
          gui.add_message_at(our_player, "return start combined station, more lines ", world.get_time())
        }
        return false
      }
      if ( wt == wt_water && combined_e > 1 && end_line_count > 2 ) {
        if ( print_message_box == 1 ) {
          gui.add_message_at(our_player, "return end combined station, more lines ", world.get_time())
        }
        return false
      }
      cnv.destroy(our_player)
      //::debug.pause()

      // Prüfen warum next_vehicle_check nicht vorhanden bei Schiffslinien
      // line_x statt link.line
      //if ( wt != wt_water ) {
        line_obj.next_vehicle_check = world.get_time().ticks + (world.get_time().ticks_per_month * 1)
      //}
    }
    // sleep - convoys are destroyed when simulation continues
    sleep()
  }

  // check convoy count
  cnv_list = line_obj.get_convoy_list()
  if ( cnv_list.get_count() == 0 ) {
    //::debug.pause()
    // destroy line
    line_obj.destroy(our_player)
    //industry_manager.access_link(fsrc, fdest, freight).remove_line(c_line)
  } else {
    gui.add_message_at(our_player, " --> ERROR not all convoys delete from line ", world.get_time())
    return false
  }

  sleep()

  start_line_count = start_h.get_line_list().get_count()
  end_line_count = end_h.get_line_list().get_count()

  if ( print_message_box == 1 ) {
    gui.add_message_at(our_player, " combined station s waytypes = " + combined_s, start_l)
    gui.add_message_at(our_player, " combined station e waytypes = " + combined_e, end_l)
  }
  // check line way
  local wayline = null
  local treeways = 0
  local double_ways = 0
  // way station to next treeway by road
  local treeway_tile_s = [null, null]
  local treeway_tile_e = [null, null]

  //
  local double_way_tiles = []

  if ( wt != wt_water && wt != wt_air ) {
    //gui.add_message_at(our_player, " search way line ", world.get_time())
    local asf = astar_route_finder(wt)
    wayline = asf.search_route([start_l], [end_l])

    local i = 0

    // remove depot
    if ( depot == null ) {
      depot = search_depot(start_l, wt_road)
      if ( !depot ) {
        depot = search_depot(end_l, wt_road)
      }
    }
    if ( depot != false && check_home_depot(depot, wt) )  {
      // todo check vehicles in depot
      remove_tile_to_empty(depot, wt, 0)
    }
    //depot = search_depot(start_l, wt, 15)

    if ("err" in wayline) {
      gui.add_message_at(our_player, "ERROR search way line for remove", world.get_time())
    } else {
      local sig_tile = 0
      local t_t = 0
      local treeway_tiles = []
      foreach(node in wayline.routes) {
        local tile = tile_x(node.x, node.y, node.z)
        nexttile.append(tile)
        // check route to treeways

        // treeway tile other player, set next tile for remove
        if ( t_t == 1 ) {
          treeway_tile_s[1] = tile
          t_t = 0

        }

        // one treeway ( depot ) then no split line way
        if ( dir.is_threeway(tile.get_way_dirs(wt)) ) {
          if ( treeways == 0 ) {
            treeway_tile_e[0] = tile
            treeway_tile_s[0] = tile
          } else {
            //
            if ( !(depot == false) && !(depot == null) ) {
              if ( tile.x == depot.x || tile.y == depot.y ) {
                depot_t = tile
              } else {
                treeway_tile_s[0] = tile
              }
            } else {
              treeway_tile_s[0] = tile
            }

          }
          treeways++
          //gui.add_message_at(our_player, " ## treeway_tiles " + coord3d_to_string(tile), tile)
          treeway_tiles.append(tile)
          t_t = 1

          // test signals by rail
          if ( sig_tile == 1 && wt == wt_rail ) {
            double_way_tiles.append(treeway_tiles.top())
            sig_tile = 0
          }
        }

        // treeway tile other player, set next tile for remove
        if ( treeway_tile_e[0] == null ) {
          treeway_tile_e[1] = tile
        }

        // test signals by rail
        if ( tile.find_object(mo_signal) != null && wt == wt_rail ) {
          //double_way_tiles.append(treeway_tile_s[0])
          double_way_tiles.append(treeway_tiles.top())
          sig_tile = 1
          double_ways++
          //gui.add_message_at(our_player, " ## treeway_tiles.top() " + coord3d_to_string(treeway_tiles.top()), tile)
        } /*else if ( dir.is_threeway(tile.get_way_dirs(wt)) && sig_tile == 1 && wt == wt_rail ) {
          double_way_tiles.append(tile)
          sig_tile = 0
        }*/
      }
      if ( wt == wt_rail && print_message_box > 0 ) {
        gui.add_message_at(our_player, " double_ways " + double_ways, world.get_time())
        gui.add_message_at(our_player, " double_way_tiles.get_count() " + double_way_tiles.len(), world.get_time())
      }
    }

  }

  // world.get_convoy_list()

  if ( wt == wt_rail ) {
    local remove_all = 0
    if ( print_message_box == 1 ) {
      gui.add_message_at(our_player, " remove wt_rail line ", world.get_time())
    }
    // remove combined station ( rail - water ) on start
    local tool = command_x(tool_remove_way)
    if ( start_line_count < 2 ) {
      // remove combined waytype halt water - not rail
      if ( combined_s > 1 ) {
        local t = start_h.get_tile_list()
        for ( local i = 0; i < t.len(); i++ ) {
          local k = t[i].find_object(mo_building).get_desc().get_waytype()
          if ( print_message_box == 1 ) {
            gui.add_message_at(our_player, " station tile " + i + " waytype " + k, world.get_time())
          }
          if ( k == wt_water ) {
            local tool = command_x(tool_remover)
            //tool.work(our_player, t[i])
            // to do ship line destroy
            combined_s = 1
            start_line_count = 0
            break
          }
        }
      }

      if ( treeways == 0 && combined_s == 1 && combined_e == 1 && start_line_count == 0 && end_line_count == 0 ) {
        // remove all
        remove_all = 1
      }
/*
      // remove depot
      if ( check_home_depot(depot, wt) ) {
        remove_tile_to_empty(depot, wt, 0)
      }
*/
      if ( print_message_box > 0 ) {
        gui.add_message_at(our_player, " treeways " + treeways + " combined_s " + combined_s + " combined_e " + combined_e, world.get_time())
        gui.add_message_at(our_player, " start_line_count " + start_line_count + " end_line_count " + end_line_count + " remove_all " + remove_all, world.get_time())
        gui.add_message_at(our_player, " double_ways " + double_ways, world.get_time())
        //::debug.pause()
      }

      local cnv_count_start = start_l.get_way(wt_rail).get_convoys_passed()[0] + start_l.get_way(wt_rail).get_convoys_passed()[1]
      local cnv_count_end = end_l.get_way(wt_rail).get_convoys_passed()[0] + end_l.get_way(wt_rail).get_convoys_passed()[1]

      // remove way from start to first treeway
      if ( treeways > 1 && combined_s == 1 && combined_e == 1 && start_line_count == 0 && remove_all == 0 ) {
        // remove station and way to next treeway
        if ( print_message_box == 1 ) {
          gui.add_message_at(our_player, "treeway_tile_s[0] " + coord3d_to_string(treeway_tile_s[0]), treeway_tile_s[0])
        }
        local err = tool.work(our_player, start_l, treeway_tile_s[0], "" + wt_rail)
        if ( err != null ) {
          gui.add_message_at(our_player, " ## ERROR remove way start_l" + coord3d_to_string(start_l), start_l)
        }
        //remove_tile_to_empty(depot, wt, 0)
        //remove_tile_to_empty(depot_t, wt, 0)
      }

      // remove way from end to first treeway
      if ( treeways > 1 && combined_s == 1 && combined_e == 1 && end_line_count == 0 && remove_all == 0 ) {
        // remove station and way to next treeway
        if ( print_message_box == 1 ) {
          gui.add_message_at(our_player, "treeway_tile_e[0] " + coord3d_to_string(treeway_tile_e[0]), treeway_tile_e[0])
        }
        local err = tool.work(our_player, end_l, treeway_tile_e[0], "" + wt_rail)
        if ( err != null ) {
          gui.add_message_at(our_player, " ## ERROR remove way end_l" + coord3d_to_string(end_l), start_l)
        }
      }

      // remove double ways by rail
      if ( double_ways > 0 ) {
        local j = 0;

        local c = 0
        if ( double_ways > 4 ) {
          c = 1
        }

        for ( local i = 0; i < double_ways; i++ ) {
          // remove double way
          //    local cnv_count = t_field.get_convoys_passed()[0] + t_field.get_convoys_passed()[1]
          local cnv_count_0 = double_way_tiles[j].get_way(wt_rail).get_convoys_passed()[0] + double_way_tiles[j].get_way(wt_rail).get_convoys_passed()[1]
          local cnv_count_1 = double_way_tiles[j+1].get_way(wt_rail).get_convoys_passed()[0] + double_way_tiles[j+1].get_way(wt_rail).get_convoys_passed()[1]

          if ( print_message_box > 0 ) {
            gui.add_message_at(our_player, " double_way_tiles[j] " + coord3d_to_string(double_way_tiles[j]) + " double_way_tiles[j+1] " + coord3d_to_string(double_way_tiles[j+1]), double_way_tiles[j])
            gui.add_message_at(our_player, " j " + j, double_way_tiles[j+1])
            gui.add_message_at(our_player, " cnv_count_0 " + cnv_count_0 + " cnv_count_1 " + cnv_count_1 + " cnv_count_start " + cnv_count_start + " cnv_count_end " + cnv_count_end, double_way_tiles[i])
            //::debug.pause()
          }

          if ( cnv_count_0 == cnv_count_1 && (cnv_count_0 <= (cnv_count_start + c) || cnv_count_0 <= (cnv_count_end + c)) ) {
            tool.work(our_player, double_way_tiles[j], double_way_tiles[j+1], "" + wt_rail)
            tool.work(our_player, double_way_tiles[j+1], double_way_tiles[j], "" + wt_rail)
            if ( i < (double_ways-1) ) {
              // remove single way to next double way
              tool.work(our_player, double_way_tiles[j+1], double_way_tiles[j+2], "" + wt_rail)
              //::debug.pause()
            }

          } else if ( !dir.is_threeway(double_way_tiles[j].get_way_dirs(wt)) && !dir.is_threeway(double_way_tiles[j+1].get_way_dirs(wt)) ) {
            tool.work(our_player, double_way_tiles[j], double_way_tiles[j+1], "" + wt_rail)
            tool.work(our_player, double_way_tiles[j+1], double_way_tiles[j], "" + wt_rail)
          }

          j += 2;
        }
      }
    }

    if ( start_f.len() > 0 && combined_s > 1 && start_line_count > 0 ) {
      // check lines befor remove start
      local lines = start_h.get_line_list()
      local i = 1
      foreach(line in lines) {
        if ( line.get_waytype() == wt_rail ) { i++ }
      }
      remove_all = i
    }

    if ( end_f.len() > 0 && combined_e > 1 && end_line_count > 0 ) {
      // check lines befor remove end
      local lines = end_h.get_line_list()
      local i = 1
      foreach(line in lines) {
        if ( line.get_waytype() == wt_rail ) { i++ }
      }
      remove_all = i
    }


    // remove rail line way by single halt and no more treeways
    if ( remove_all == 1 ) {
      // remove line way
      if ( print_message_box == 1 ) {
        gui.add_message_at(our_player, " remove_all == 1 ", world.get_time())
      }
      local tool = command_x(tool_remove_way)
      if ( start_l.is_empty() || end_l.is_empty() ) {
        local c1 = 0
        local c2 = 0
        for ( local i = 0; i > 10; i++ ) {
          if ( !nexttile[nexttile.len-i].is_empty() ) {
            start_l = nexttile[nexttile.len-i]
            c1++
          }
          if ( !nexttile[i].is_empty() ) {
            end_l = nexttile[i]
            c2++
          }
          if ( (c1+c2) == 2 ) { break }
        }
      }

      local t = start_l.get_halt().get_tile_list()

      tool.work(our_player, start_l, end_l, "" + wt_rail)
      // remove all tiles from halt start
        for ( local i = 0; i < t.len(); i++ ) {
          if ( t[i].find_object(mo_building) ) {
            t[i].remove_object(our_player, mo_building)
          }
        }
    }


    //::debug.pause()
  }

  // remove road line
  if ( wt == wt_road ) {
    // remove depot not other road stations in range
    // to do check other stations
    /*if ( check_home_depot(depot, wt) ) {
      remove_tile_to_empty(depot, wt, 0)
    }*/

    // remove way line and station by 0 lines connected
    start_line_count = start_h.get_line_list().get_count()
    end_line_count = end_h.get_line_list().get_count()

    if ( print_message_box == 1 ) {
      gui.add_message_at(our_player, " treeway_tile_s " + treeway_tile_s[0] + "  " + treeway_tile_s[1], world.get_time())
      gui.add_message_at(our_player, " treeway_tile_e " + treeway_tile_e[0] + "  " + treeway_tile_e[1], world.get_time())
    }

    local tool = command_x(tool_remove_way)
    if ( start_line_count == 0 ) {
      // remove combined waytype halt water - not road
      if ( combined_s > 1 ) {
        local t = start_h.get_tile_list()
        for ( local i = 0; i < t.len(); i++ ) {
          local k = t[i].find_object(mo_building).get_desc().get_waytype()
          if ( k == wt_water ) {
            local tool = command_x(tool_remover)
            tool.work(our_player, t[i])
            break
          }
        }
      }
      // remove station and way to next treeway
      // treeway tile check player
      if ( treeway_tile_s[0] != null && treeway_tile_s[0].find_object(mo_way).get_owner() == our_player ) {
        tool.work(our_player, start_l, treeway_tile_s[0], "" + wt_road)
      } else if ( treeway_tile_s[1] != null && treeways > 0 ) {
        tool.work(our_player, start_l, treeway_tile_s[1], "" + wt_road)
        remove_tile_to_empty(treeway_tile_s[1], wt_road, 0)
      }
    }

    if ( end_line_count == 0 ) {
      // remove combined waytype halt water - not road
      if ( combined_e > 1 ) {
        local t = end_h.get_tile_list()
        for ( local i = 0; i < t.len(); i++ ) {
          local k = t[i].find_object(mo_building).get_desc().get_waytype()
          if ( k == wt_water ) {
            local tool = command_x(tool_remover)
            tool.work(our_player, t[i])
            //break
          }
        }
      }
      // remove station and way to next treeway
      // treeway tile check player
      if ( treeway_tile_e[0] != null && treeway_tile_e[0].find_object(mo_way).get_owner() == our_player ) {
        tool.work(our_player, end_l, treeway_tile_e[0], "" + wt_road)
      } else if ( treeway_tile_e[1] != null && treeways > 0 ) {
        tool.work(our_player, end_l, treeway_tile_e[1], "" + wt_road)
        remove_tile_to_empty(treeway_tile_e[1], wt_road, 0)
      }
    }

    if ( treeways == 0 ) {
      tool.work(our_player, end_l, start_l, "" + wt_road)
    }
  }

  // remove water line
  if ( wt == wt_water ) {
    local tool = command_x(tool_remover)

    // remove depot
    if ( check_home_depot(depot, wt) ) {
      local err = depot.remove_object(our_player, mo_depot_rail)
      if ( err != null ) {
        gui.add_message_at(our_player, "## ERROR depot remove " + depot + ": " + err, home_depot)
      }
      //tool.work(our_player, depot)
    }

    // remove way line and station by 0 lines connected
    start_line_count = start_h.get_line_list().get_count()
    end_line_count = end_h.get_line_list().get_count()

    if ( start_line_count <= 1 ) {
      // remove combined waytype halt water - not road/rail
      if ( combined_s >= 1 && combined_e == 1 ) {
        local t = start_h.get_tile_list()
        for ( local i = 0; i < t.len(); i++ ) {
          if ( t[i].find_object(mo_building) != null ) {
            local k = t[i].find_object(mo_building).get_desc().get_waytype()
            if ( k == wt_water ) {
              tool.work(our_player, t[i])
              //break
            } /*else if ( k == wt_rail ) {
              tool.work(our_player, t[i])
            }*/
          }
        }
        //t = start_h.get_tile_list()
        //tool.work(our_player, t[0])
      }
    }

    if ( end_line_count <= 1 ) {
      // remove combined waytype halt water - not road/rail
      if ( combined_e <= 2 ) {
        local t = end_h.get_tile_list()
        for ( local i = 0; i < t.len(); i++ ) {
          if ( t[i].find_object(mo_building) != null ) {
            local k = t[i].find_object(mo_building).get_desc().get_waytype()
            if ( k == wt_water ) {
              tool.work(our_player, t[i])
              /*  break
            } else if ( k == wt_rail ) {
              tool.work(our_player, t[i])*/
            }
          }
        }
      }
    }
  }
  if ( print_message_box > 0 ) {
    gui.add_message_at(our_player, "+ destroy_line(line_obj, good, link) finish line " + line_name, world.get_time())
    //::debug.pause()
  }

  local msgtext = format(translate("%s removes line: %s"), our_player.get_name(), line_name)
  gui.add_message_at(our_player, msgtext, world.get_time())

  //link_obj.status = 4
  //link_obj.next_check = today_plus_months(36)

  check_stations_connections()

  return true
}

/*
 * check waytypes from halt
 * tile = one tile from halt
 * return count waytypes
 */
function test_halt_waytypes(tile) {
  if ( tile.is_water() && tile.find_object(mo_building) == null ) {
    //gui.add_message_at(our_player, "halt is water tile " + coord3d_to_string(tile), tile)
    return 0
  }

  local halt_tiles = tile.get_halt().get_tile_list()
  //gui.add_message_at(our_player, "halt has tiles " + halt_tiles.len(), tile)

  local test_rail = 0
  local test_road = 0
  local test_water = 0
  for ( local i = 0; i < halt_tiles.len(); i++ ) {
    local building = halt_tiles[i].find_object(mo_building).get_desc()
    if ( building ) {
      switch (building.get_waytype()) {
        case wt_rail:
          test_rail++
          break
        case wt_road:
          test_road++
          break
        case wt_water:
          test_water++
          break
      }
    }
  }
  local test_way = 0
  if ( test_rail > 0 ) { test_way++ }
  if ( test_road > 0 ) { test_way++ }
  if ( test_water > 0 ) { test_way++ }

  return test_way
}

/*
 * check depot as home for other vehicles
 * tile = depot coord
 * wt   = waytype
 *
 * return: true = no vehicles; false = other vehicles
 */
function check_home_depot(tile, wt) {

  // load vehicle list
  local list = world.get_convoy_list()

  foreach(cnv in list) {
    if ( tile.x == cnv.get_home_depot().x && tile.y == cnv.get_home_depot().y && tile.z == cnv.get_home_depot().z ) {
      return false
    }
  }

  return true

}

/*
 * check stations to valid link
 *
 *
 *
 *
 */
function check_stations_connections() {
  //gui.add_message_at(our_player, "####### check_stations_connections()" , world.get_time())

  if ( debug ) ::debug.set_pause_on_error(true)

  local haltlist = []
  // list player halts
  foreach(halt in halt_list_x()) {
    if (halt.get_owner().nr == our_player_nr) {  // && halt.get_type == wt
      haltlist.append(halt)
    }
  }

  // halt no lines and convoys
  local halt_unused = []

  // test factory connection
  foreach(halt in haltlist) {

    local s = halt.get_name()

    local halt_factory_connect = halt.get_factory_list().len()
    local halt_lines = halt.get_line_list()
    local halt_line = null
    local halt_cnv = halt.get_convoy_list()

    //if ( halt_lines.get_count() == 1 ) { gui.add_message_at(our_player, "####### halt_lines[0]: " + halt_lines[0].get_name(), world.get_time()) }

    //local tool = command_x(tool_remover)

    if ( halt_lines.get_count() == 0 && halt_cnv.get_count() == 0 ) {

      //gui.add_message_at(our_player, "####### halt name: " + halt.get_name(), world.get_time())


      // halt no lines and convoys - remove
      local t = halt.get_tile_list()
      for ( local i = 0; i < t.len(); i++ ) {
        if ( t[i].has_way(wt_rail) ) {
          // remove way by wt_rail from halt tiles
          gui.add_message_at(our_player, "####### remove rail halt : " + halt.get_name(), t[i])
          remove_tile_to_empty(t, wt_rail, 1)
        } else {
          //gui.add_message_at(our_player, "####### remove building halt name: " + halt.get_name(), t[i])
          t[i].remove_object(our_player, mo_building)
        }
      }
    } else if ( halt_factory_connect == 0 && halt_lines.get_count() < 2 ) {

      local line_list = player_x(our_player.nr).get_line_list()
      foreach(line in line_list) {
        //gui.add_message_at(player_x(our_player.nr), "####### line check " + line.get_name(), world.get_time())
        if ( halt_lines[0].id == line.id ) {
          halt_line = line
          break
        }
      }

      //gui.add_message_at(our_player, "####### halt name: " + halt.get_name(), world.get_time())

      local home_depot = null
      local wt = null
      local good = null

      // combined station not finish
      if ( halt_cnv.get_count() > 0 ) {
        // remove convoys from halt
        local j = halt_cnv.get_count()
        home_depot = halt_cnv[0].get_home_depot()
        wt = halt_cnv[0].get_waytype()
        for ( local i = 0; i < j; i++ ) {
          halt_cnv[i].destroy(our_player)
        }
      } else if ( halt.get_line_list().get_count() == 1 ) {

        local link_list = industry_manager.link_list
        local search_line = null
        foreach(link in link_list) {
        // start_f[0], end_f[0], good.get_name()


          foreach(index, line in link.lines) {
            //gui.add_message_at(player_x(our_player.nr), "####### line check " + line.get_name(), world.get_time())
            if ( line.is_valid() && line.get_owner().nr == our_player.nr && halt_line.get_name() == line.get_name() ) {
              search_line = line
            }
          }

        }

        if ( search_line != null ) {
          destroy_line(search_line, good, null) //halt_line
        }

        break
      }

    }

  }

}

function check_combined_station(halt) {
  local lines = halt.get_line_list()
  local factorys = halt.get_factory_list()

  local halt_tiles = halt.get_tile_list()

  local wts = test_halt_waytypes(halt_tiles[0])

  if ( wts > 1 && lines.get_count() == 1 && factorys.len() > 0 ) {
    gui.add_message_at(our_player, "####### station more wt and one line - factory connect ", world.get_time())
    local wt = null

    foreach(line in lines) {
      wt = line.get_waytype()
    }

    local replace_tiles = []

    for ( local i = 0; i < halt_tiles.len(); i++ ) {
      local building = halt_tiles[i].find_object(mo_building).get_desc()
      if ( building ) {
        if ( building.get_waytype() != wt ) {
          replace_tiles.append(halt_tiles[i])
        }

      }
    }

    local extension = find_extension(wt_road)

    if ( extension != null ) {
      for ( local i = 0; i < replace_tiles.len()-1; i++ ) {
        remove_tile_to_empty(replace_tiles[i], wt_road, 0)
        command_x.build_station(our_player, replace_tiles[i], extension)

      }

    }
  }

}

function station_aw(start_field, wt, awst_array) {
  local print_message_box = 0
  local b_player = our_player
  //gui.add_message_at(b_player, "station_aw ", start_field)

  local way_obj = start_field.find_object(mo_way).get_desc() //way_list[0]
  if ( !way_obj.is_available(world.get_time()) ) {
    way_obj = find_object("way", wt, way_obj.get_topspeed())
  }

  local way_len = 8
  local d = start_field.get_way_dirs(wt)
  local tiles = []
  local tiles_build_l = []
  local tiles_build_r = []

  for (local i = 0; i < way_len; i++) {
    if ( d == 5 ) {
      // build from n to s
      // ns - r
      local ref_ground = square_x(start_field.x, start_field.y + i).get_ground_tile()
      local t = tile_x(start_field.x + 1, start_field.y + i, start_field.z)
      if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(start_field.x, start_field.y + i).get_ground_tile().z == ref_ground.z
        tiles_build_r.append(tile_x(start_field.x + 1, start_field.y + i, ref_ground.z))
      }
      // ns - l
      t = tile_x(start_field.x - 1, start_field.y + i, start_field.z)
      if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(start_field.x, start_field.y + i).get_ground_tile().z == ref_ground.z
        tiles_build_l.append(tile_x(start_field.x - 1, start_field.y + i, ref_ground.z))
      }

      tiles.append(ref_ground)

    } else if ( d == 10 ) {
      //gui.add_message_at(our_player, "tile r " + coord3d_to_string(tile_x(start_field.x + i, start_field.y + 1, start_field.z)) + " empty " + tile_x(start_field.x + i, start_field.y + 1, start_field.z).is_empty(), start_field)
      //gui.add_message_at(our_player, " -- tile l " + coord3d_to_string(tile_x(start_field.x + i, start_field.y - 1, start_field.z)) + " to empty " + tile_x(start_field.x - i, start_field.y + 1, start_field.z).is_empty(), start_field)

      // ew - r
      // build from w to e
      local ref_ground = square_x(start_field.x + i, start_field.y).get_ground_tile()
      local t = tile_x(start_field.x + i, start_field.y + 1, start_field.z)
      if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(start_field.x + i, start_field.y).get_ground_tile().z == ref_ground.z
        tiles_build_r.append(tile_x(start_field.x + i, start_field.y + 1, ref_ground.z))
      }
      // ew - l
      t = tile_x(start_field.x + i, start_field.y - 1, start_field.z)
      if ( t.is_valid() && test_tile_is_empty(t) ) { //&& square_x(start_field.x + i, start_field.y).get_ground_tile().z == ref_ground.z
        tiles_build_l.append(tile_x(start_field.x + i, start_field.y - 1, ref_ground.z))
      }
      tiles.append(ref_ground)
    }
  }

  local tl = 0
  local tr = 0
  local tiles_build = null
  //way_len -= 1
  if ( tiles_build_r.len() == way_len || tiles_build_l.len() == way_len ) {
      if ( tiles_build_r.len() == way_len ) {
        for ( local i = 0; i < way_len; i++ ) {
          if ( tiles_build_r[i].get_slope() == 0 ) {
            tr++
          }
        }
      }
      if ( tiles_build_l.len() == way_len ) {
        for ( local i = 0; i < way_len; i++ ) {
          if ( tiles_build_l[i].get_slope() == 0 ) {
            tl++
          }
        }
      }

    local err = null
    local terraform = 0

    if ( tr == way_len ) {
      if ( print_message_box == 1 ) {
        gui.add_message_at(b_player, "build flat right ", start_field)
      }
      tiles_build = tiles_build_r
      tr = way_len
      tl = 0
    }
    else if ( tl == way_len ) {
      if ( print_message_box == 1 ) {
        gui.add_message_at(b_player, "build flat left ", start_field)
      }
      tiles_build = tiles_build_l
      tl = way_len
      tr = 0
    }
    else if ( ( tr < tiles_build_r.len() && tiles_build_r.len() == way_len ) && tr >= tl ) {
      // check terraform tr
      if ( print_message_box == 1 ) {
        gui.add_message_at(b_player, "build right check terraform", world.get_time())
      }
      tiles_build = tiles_build_r
      tr = way_len
      tl = 0
      terraform = 1
    }
    else if ( tl < tiles_build_l.len() && tiles_build_l.len() == way_len ) {
      // check terraform tl
      if ( print_message_box == 1 ) {
        gui.add_message_at(b_player, "build left check terraform", world.get_time())
      }
      tiles_build = tiles_build_l
      tl = way_len
      tr = 0
      terraform = 1
    }

    if ( terraform == 1 ) {
      // change terraform

      local g = terraform_way(tiles, tiles_build, tr, tl, way_len, d)
      tr = g[0]
      tl = g[1]
      if ( way_len > g[2] ) {
        tiles_build.remove(tiles_build.len()-1)
        tiles.remove(tiles.len()-1)
      }

    /*
            if ( tr == way_len ) {
              tr--
            }
            if ( tl == way_len ) {
              tl--
            }
            way_len--
    */
    }
    // set build left or right
    /*if ( tr == way_len && tl == way_len ) {

    }
    else */
    if ( tr == way_len ) {
      tiles_build = tiles_build_r
    }
    else if ( tl == way_len ) {
      tiles_build = tiles_build_l
    }

    if ( tiles_build == null ) {
      //gui.add_message_at(b_player, " ERROR no double way found " + coord3d_to_string(tiles[0]), start_field)
      return false
    }



    // build way
    local err = null
    if ( start_field.get_way_dirs(wt) == 5 || start_field.get_way_dirs(wt) == 10 ) {
      if ( print_message_box == 1 ) {
        gui.add_message_at(b_player, "5/10 build tiles[0] " + coord3d_to_string(tiles[0]) + " to tiles_build[0] " + coord3d_to_string(tiles_build[0]), start_field)
      }
      // connect diagonal way
      local b_tile = null
      local tile_a = null
      local tile_b = null
      local tile_check = 0
      if ( tl == way_len ) {
        if ( start_field.get_way_dirs(wt) == 5 ) {
          tile_a = tile_x(tiles[0].x, tiles[0].y-1, tiles[0].z)
          tile_b = tile_x(tiles[0].x-1, tiles[0].y-1, tiles[0].z)
          if ( tile_a.get_way_dirs(wt) == 12 && (tile_b.get_way_dirs(wt) == 3 || (tile_b.get_way_dirs(wt) == 10 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0)) ) {
            b_tile = tile_b
          }
          tile_check = 1
          //gui.add_message_at(b_player, "(2765) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)
        } else if ( start_field.get_way_dirs(wt) == 10 ) {
          tile_a = tile_x(tiles[0].x-1, tiles[0].y, tiles[0].z)
          tile_b = tile_x(tiles[0].x-1, tiles[0].y-1, tiles[0].z)
          if ( (tile_a.get_way_dirs(wt) == 12 || tile_a.get_way_dirs(wt) == 3) && (tile_b.get_way_dirs(wt) == 3 || tile_b.get_way_dirs(wt) == 12 || (tile_b.get_way_dirs(wt) == 5 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0)) ) {
            b_tile = tile_b
          }
          tile_check = 2
          //gui.add_message_at(b_player, "(2773) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)
        }

      } else if ( tr == way_len ) {
        if ( start_field.get_way_dirs(wt) == 5 ) {
          tile_a = tile_x(tiles[0].x, tiles[0].y-1, tiles[0].z)
          tile_b = tile_x(tiles[0].x+1, tiles[0].y-1, tiles[0].z)
          if ( tile_a.get_way_dirs(wt) == 6 && (tile_b.get_way_dirs(wt) == 9 || (tile_b.get_way_dirs(wt) == 10 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0)) ) {
            b_tile = tile_b
          }
          tile_check = 3
          //gui.add_message_at(b_player, "(2785) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)

        } else if ( start_field.get_way_dirs(wt) == 10 ) {
          tile_a = tile_x(tiles[0].x-1, tiles[0].y, tiles[0].z)
          tile_b = tile_x(tiles[0].x-1, tiles[0].y+1, tiles[0].z)
          if ( tile_a.get_way_dirs(wt) == 6 && (tile_b.get_way_dirs(wt) == 9 || (tile_b.get_way_dirs(wt) == 5 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0)) ) {
            b_tile = tile_b
          }
          tile_check = 4
          //gui.add_message_at(b_player, "(2795) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)
        }

      }
      if ( b_tile == null ) {
        err = command_x.build_way(b_player, tiles[0], tiles_build[0], way_obj, true)
      } else {
        err = command_x.build_way(b_player, b_tile, tiles_build[0], way_obj, true)
      }

      if ( err == null ) {
        if ( print_message_box == 1 ) {
          gui.add_message_at(b_player, "  build tiles_build[0] " + coord3d_to_string(tiles_build[0]) + " to tiles_build[" + (way_len - 1) + "] " + coord3d_to_string(tiles_build[way_len - 1]), start_field)
        }
        err = command_x.build_way(b_player, tiles_build[0], tiles_build[way_len - 1], way_obj, true)
        if ( err != null ) {
          gui.add_message_at(b_player, " ERROR => build tile " + coord3d_to_string(tiles[0]) + " to tile " + coord3d_to_string(tiles_build[way_len - 1]), tiles[0])
          err = null
        }
        if ( print_message_box == 1 ) {
          gui.add_message_at(b_player, "  build tiles_build[" + (way_len - 1) + "] " + coord3d_to_string(tiles_build[way_len - 1]) + " to tiles[" + (way_len - 1) + "] " + coord3d_to_string(tiles[way_len - 1]), start_field)
        }
        // connect diagonal way
        b_tile = null
        if ( tile_check == 1 ) {

            tile_a = tile_x(tiles[way_len - 1].x, tiles[way_len - 1].y+1, tiles[way_len - 1].z)
            tile_b = tile_x(tiles[way_len - 1].x-1, tiles[way_len - 1].y+1, tiles[way_len - 1].z)
            if ( tile_a.get_way_dirs(wt) == 9 && (tile_b.get_way_dirs(wt) == 6 || tile_b.get_way_dirs(wt) == 10 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0) ) {
              b_tile = tile_b
            }
            //gui.add_message_at(b_player, "(2827) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)

        } else if ( tile_check == 2 ) {
            tile_a = tile_x(tiles[way_len - 1].x+1, tiles[way_len - 1].y, tiles[way_len - 1].z)
            tile_b = tile_x(tiles[way_len - 1].x+1, tiles[way_len - 1].y-1, tiles[way_len - 1].z)
            if ( tile_a.get_way_dirs(wt) == 6 && (tile_b.get_way_dirs(wt) == 9 || tile_b.get_way_dirs(wt) == 10 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0) ) {
              b_tile = tile_b
            }
            //gui.add_message_at(b_player, "(2836) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)
        } else if ( tile_check == 3 ) {
            tile_a = tile_x(tiles[way_len - 1].x, tiles[way_len - 1].y+1, tiles[way_len - 1].z)
            tile_b = tile_x(tiles[way_len - 1].x+1, tiles[way_len - 1].y+1, tiles[way_len - 1].z)
            if ( tile_a.get_way_dirs(wt) == 3 && (tile_b.get_way_dirs(wt) == 12 || (tile_b.get_way_dirs(wt) == 5 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0)) ) {
              b_tile = tile_b
            }
            //gui.add_message_at(b_player, "(2844) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)

        } else if ( tile_check == 4 ) {
            tile_a = tile_x(tiles[way_len - 1].x+1, tiles[way_len - 1].y, tiles[way_len - 1].z)
            tile_b = tile_x(tiles[way_len - 1].x+1, tiles[way_len - 1].y+1, tiles[way_len - 1].z)
            if ( tile_a.get_way_dirs(wt) == 12 && (tile_b.get_way_dirs(wt) == 3 || (tile_b.get_way_dirs(wt) == 5 && !tile_b.is_bridge() && !tile_b.is_tunnel() && tile_b.get_slope() == 0)) ) {
              b_tile = tile_b
            }
            //gui.add_message_at(b_player, "(2853) tile_a " + coord3d_to_string(tile_a) + " tile_b " + coord3d_to_string(tile_b), start_field)

        }

        if ( b_tile == null ) {
          err = command_x.build_way(b_player, tiles_build[way_len - 1], tiles[way_len - 1], way_obj, true)

        } else {
          err = command_x.build_way(b_player, tiles_build[way_len - 1], b_tile, way_obj, true)
        }

        local staw = []
        for ( local j = 0; j < tiles.len(); j++ ) {
          if ( tiles[j].find_object(mo_building) == null && ( tiles[j].get_way_dirs(wt) == 5 || tiles[j].get_way_dirs(wt) == 10 ) ) {
            staw.append(tiles[j])
          }
        }
        remove_tile_to_empty(staw, wt, 1)
      }

    }
  }
  /*
  if ( tiles[0].x == tiles[1].x && tiles[0].y < tiles[1].y ) {
    // N -> S
  } else if ( tiles[0].x < tiles[1].x && tiles[0].y == tiles[1].y ) {
    // W -> E
  }
  */
  //err = command_x.build_way(b_player, tiles_build[way_len - 1], tiles[way_len - 1], way_obj, true)
  //tiles_build[way_len - 1]
  // awst_array
  //gui.add_message_at(b_player, "(6663) tr " + tr + " tl " + tl, start_field)
  // Ausweichstelle nach Station
  if ( tl == way_len ) {
    //gui.add_message_at(b_player, "(6664)  ", awst_array[0])
    if ( awst_array[0].x == awst_array[awst_array.len()-1].x || awst_array[0].y == awst_array[awst_array.len()-1].y ) {
      local tile_c = tile_x(awst_array[0].x, awst_array[0].y-1, awst_array[0].z)
      //gui.add_message_at(b_player, "(6667)  ", tile_c)
      local err = command_x.build_way(b_player, awst_array[0], tile_c, way_obj, true)
      gui.add_message_at(b_player, "(6088) err " + err, awst_array[0])
      if ( err != null ) {
        remove_tile_to_empty(awst_array, wt_rail, 1)
      } else {
        err = command_x.build_way(b_player, tile_c, tiles_build[0], way_obj, true)
        gui.add_message_at(b_player, "(6093) err " + err, tiles_build[0])
        if ( err != null ) {
          remove_tile_to_empty(awst_array, wt_rail, 1)
        } else {
          local obj_sign = find_signal("is_signal", wt)
          local signal = null
          local s_tile = tile_x(awst_array[1].x,awst_array[1].y-1, awst_array[1].z)
          if ( s_tile.get_way_dirs(wt) == 10 ) {
            // ew
            signal = [{coor=coord3d(awst_array[1].x,awst_array[1].y-1, awst_array[1].z), ribi=8}, {coor=coord3d(awst_array[awst_array.len()-2].x, awst_array[awst_array.len()-2].y, awst_array[awst_array.len()-2].z), ribi=2}]
          } else if ( s_tile.get_way_dirs(wt) == 5 ) {
            // ns
            signal = [{coor=coord3d(awst_array[1].x,awst_array[1].y-1, awst_array[1].z), ribi=1}, {coor=coord3d(awst_array[awst_array.len()-2].x, awst_array[awst_array.len()-2].y, awst_array[awst_array.len()-2].z), ribi=4}]
          }

          for ( local j=0; j < 2; j++ ) {
            local s_ribi = signal[j].ribi
            local signal_build_tile = tile_x(signal[j].coor.x, signal[j].coor.y, signal[j].coor.z)
              gui.add_message_at(b_player, "(6103) signal_build_tile " + coord3d_to_string(signal_build_tile), signal_build_tile)
              while(true){
                err = command_x.build_sign_at(b_player, signal_build_tile, obj_sign)
                if ( err != null ) {
                  break
                }
                local ribi = signal_build_tile.get_way_dirs_masked(wt)
                if (ribi == s_ribi)
                  break
              }
          }
          if ( err != null ) {
            remove_tile_to_empty(awst_array, wt_rail, 1)
          }

        }

      }
    }
  }

}

function terraform_way(tiles, tiles_build, tr, tl, way_len, d) {
  local err = null
  local b_player = our_player
  local print_message_box = 0

  if ( print_message_box > 0 ) {
    gui.add_message_at(b_player, " ---=> terraform_way() ", world.get_time())
  }


      for ( local i = 0; i < tiles_build.len(); i++ ) {
        //local r = square_x(tiles_build[i].x, tiles_build[i].y)
        local z = 0
        //local f = tile_x(tiles_build[i].x, tiles_build[i].y, z.z)
        local build_hight = square_x(tiles_build[i].x, tiles_build[i].y).get_ground_tile()
        local ref_hight = square_x(tiles[i].x, tiles[i].y).get_ground_tile()

        local straight_slope = false
        if ( ref_hight.get_slope() == 4 || ref_hight.get_slope() == 12 || ref_hight.get_slope() == 28 || ref_hight.get_slope() == 36 ) {
          // single hight && double hight 1
          straight_slope = true
        } else if ( ref_hight.get_slope() == 8 || ref_hight.get_slope() == 24 || ref_hight.get_slope() == 56 || ref_hight.get_slope() == 72 ) {
          // double hight 2
          straight_slope = true
        }

        if ( print_message_box == 21 ) {
          gui.add_message_at(b_player, " ---=> tiles[i] ground " + coord3d_to_string(ref_hight), ref_hight)
          gui.add_message_at(b_player, " ---=> tiles_build[i] ground " + coord3d_to_string(build_hight), build_hight)
          gui.add_message_at(b_player, " ---=> ref_hight.get_slope() " + ref_hight.get_slope(), world.get_time())
          gui.add_message_at(b_player, " ---=> straight_slope " + straight_slope, world.get_time())
        }

        if ( build_hight.is_empty() && ( build_hight.get_slope() > 0 || ref_hight.z != build_hight.z ) ) {

          if ( print_message_box == 21 ) {
            gui.add_message_at(b_player, " ---=> terraform", world.get_time())
            gui.add_message_at(b_player, " ---=> tiles_build.z " + build_hight.z + " tiles.z " + ref_hight.z, world.get_time())
          }

          if ( (build_hight.z < ref_hight.z && build_hight.z >= (ref_hight.z - 2)) ) { //|| straight_slope == true
          // terraform up
            if ( print_message_box == 21 ) {
              gui.add_message_at(b_player, " ---=> terraform up ", world.get_time())
            }

              local terraform_tile = 1
              // terraform grid up
              if ( print_message_box == 21 ) {
                gui.add_message_at(b_player, " build_hight.get_slope() " + build_hight.get_slope(), build_hight)
              }

              local tile_build_slope = [37, 9, 31, 1, 3]
              if ( tile_build_slope.find(build_hight.get_slope()) != null ) {
                local cx = 0
                local cy = 0
                local build_side = 0
                if ( d == 10 ) {
                  cx = 1
                  if ( tiles_build[i].y > tiles[i].y ) {
                    build_side = 1
                  }
                } else if ( d == 5 ) {
                  cy = 1
                  if ( tiles_build[i].x > tiles[i].x ) {
                    build_side = 1
                  }
                }

                local tile_a = square_x(tiles_build[i].x+cx, tiles_build[i].y+cy).get_ground_tile()
                local tile_a1 = null
                local tile_b1 = null
                if ( build_side == 1 ) {
                  tile_a1 = square_x(tiles_build[i].x+cy, tiles_build[i].y+cx).get_ground_tile()
                  tile_b1 = square_x(tile_a.x+cy, tile_a.y+cx).get_ground_tile()
                } else {
                  tile_a1 = square_x(tiles_build[i].x-cy, tiles_build[i].y-cx).get_ground_tile()
                  tile_b1 = square_x(tile_a.x-cy, tile_a.y-cx).get_ground_tile()
                }

                if ( print_message_box == 21 ) {
                  gui.add_message_at(b_player, " tile_a.get_slope() " + tile_a.get_slope(), tile_a)
                  gui.add_message_at(b_player, " tile_a1.get_slope() " + tile_a1.get_slope(), tile_a1)
                  gui.add_message_at(b_player, " tile_b1.get_slope() " + tile_b1.get_slope(), tile_b1)
                }

                local tile_a_slope = [27, 36, 31, 1]
                if ( build_hight.get_slope() == 37 && tile_a_slope.find(tile_a.get_slope()) != null && test_tile_is_empty(tile_a1) && test_tile_is_empty(tile_b1) ) {
                  err = command_x.grid_raise(our_player, coord3d(tile_b1.x, tile_b1.y, tile_b1.z))
                  if ( err == null ) {
                    terraform_tile = 0
                  }
                } else if ( (build_hight.get_slope() == 31 || build_hight.get_slope() == 3) && (tile_a.get_slope() == 4 || tile_a.get_slope() == 13 || tile_a.get_slope() == 1) ) {
                  local tile_a1 = null
                  local tile_b1 = null
                  if ( build_side == 1 ) {
                    tile_a1 = square_x(tiles_build[i].x+cy, tiles_build[i].y+cx).get_ground_tile()
                    tile_b1 = square_x(tile_a.x+cy, tile_a.y+cx).get_ground_tile()
                  } else {
                    tile_a1 = square_x(tiles_build[i].x-cy, tiles_build[i].y-cx).get_ground_tile()
                    tile_b1 = square_x(tile_a.x-cy, tile_a.y-cx).get_ground_tile()
                  }

                  //tile_a1 = square_x(tiles_build[i].x, tiles_build[i].y-1).get_ground_tile()
                  //tile_b1 = square_x(tile_a.x, tile_a.y-1).get_ground_tile()

                  if ( test_tile_is_empty(tile_a1) && test_tile_is_empty(tile_b1) ) {
                    err = command_x.grid_raise(our_player, coord3d(tile_a.x, tile_a.y, tile_a.z))
                    terraform_tile = 0
                  }

                }


              }

              // terraform tile
              if ( terraform_tile == 1 ) {
                do {
                  err = command_x.set_slope(b_player, build_hight, 82 )
                  if ( err != null ) { break }
                  z = square_x(tiles_build[i].x, tiles_build[i].y).get_ground_tile()
                } while(z.z < ref_hight.z )

              }


          } else if ( build_hight.z >= ref_hight.z || build_hight.z <= (ref_hight.z + 1) ) {
            // terraform down
            if ( print_message_box == 21 ) {
              gui.add_message_at(b_player, " ---=> terraform down  ", world.get_time())
            }
            if ( (i == 0 && tiles[0].get_slope() > 0 ) || ( i == (way_len-1) && tiles[way_len-1].get_slope() > 0 ) ) {

            } else {
              local terraform_tile = 1
              // terraform grid down
              if ( print_message_box == 21 ) {
                gui.add_message_at(b_player, " build_hight.get_slope() " + build_hight.get_slope(), build_hight)
              }

              local tile_build_slope = [3, 1, 9, 37, 39]
              if ( tile_build_slope.find(build_hight.get_slope()) != null ) {
                local cx = 0
                local cy = 0
                local build_side = 0
                if ( d == 10 ) {
                  cx = 1
                  if ( tiles_build[i].y > tiles[i].y ) {
                    build_side = 1
                  }
                } else if ( d == 5 ) {
                  cy = 1
                  if ( tiles_build[i].x > tiles[i].x ) {
                    build_side = 1
                  }
                }

                local tile_a = square_x(tiles_build[i].x+cx, tiles_build[i].y+cy).get_ground_tile()
                local tile_a1 = null
                local tile_b1 = null

                local tile_a_slope = null
                local tile_a_slope_WE = null
                local tile_a_slope_NS = null


                if ( build_side == 1 ) {
                  // x/y build way > exists way
                  tile_a1 = square_x(tiles_build[i].x+cy, tiles_build[i].y+cx).get_ground_tile()
                  tile_b1 = square_x(tile_a.x+cy, tile_a.y+cx).get_ground_tile()
                  tile_a_slope = [12, 13, 1, 4]
                  tile_a_slope_WE = [39, 27, 36]
                  tile_a_slope_NS = [9]
                } else {
                  // x/y build way < exists way
                  tile_a1 = square_x(tiles_build[i].x-cy, tiles_build[i].y-cx).get_ground_tile()
                  tile_b1 = square_x(tile_a.x-cy, tile_a.y-cx).get_ground_tile()
                  tile_a_slope = [9, 12, 13, 1, 4, 39]
                  tile_a_slope_WE = [39, 27, 36]
                  tile_a_slope_NS = [31]
                }

                if ( print_message_box == 21 ) {
                  gui.add_message_at(b_player, " tile_a.get_slope() " + tile_a.get_slope(), tile_a)
                  gui.add_message_at(b_player, " tile_a1.get_slope() " + tile_a1.get_slope(), tile_a1)
                  gui.add_message_at(b_player, " tile_b1.get_slope() " + tile_b1.get_slope(), tile_b1)
                }

                if ( tile_build_slope.find(build_hight.get_slope()) != null && tile_a_slope.find(tile_a.get_slope()) != null && test_tile_is_empty(tile_a1) && test_tile_is_empty(tile_b1) ) {
                  //gui.add_message_at(b_player, " #3036# --", world.get_time())
                  if ( straight_slope == true && build_side == 0 ) {
                    err = command_x.grid_raise(our_player, coord3d(tile_a.x, tile_a.y, tile_a.z))
                  } else if ( straight_slope == true && build_side == 1 ) {
                    err = command_x.grid_raise(our_player, coord3d(tile_a1.x, tile_a1.y, tile_a1.z))
                  } else {
                    err = command_x.grid_lower(our_player, coord3d(tile_b1.x, tile_b1.y, tile_b1.z))
                  }
                  if ( err == null ) {
                    terraform_tile = 0
                  } else {
                    gui.add_message_at(b_player, " #3047# err " + err, world.get_time())
                  }

                } else if ( tile_build_slope.find(build_hight.get_slope()) != null && tile_a_slope_NS.find(tile_a.get_slope()) != null ) {
                  if ( test_tile_is_empty(tile_a1) && test_tile_is_empty(tile_b1) ) {
                    //gui.add_message_at(b_player, " #3065#  straight_slope " + straight_slope + " -- build_side " + build_side, world.get_time())
                    if ( straight_slope == true && build_side == 0 ) {
                      err = command_x.grid_lower(our_player, coord3d(tile_a.x, tile_a.y, tile_a.z))
                    } else if ( straight_slope == true && build_side == 1 ) {
                      gui.add_message_at(b_player, " #3069#  " + coord3d_to_string(tile_b1), world.get_time())
                      err = command_x.grid_lower(our_player, coord3d(tile_b1.x, tile_b1.y, tile_b1.z))
                    } else {
                      err = command_x.grid_lower(our_player, coord3d(build_hight.x, build_hight.y, build_hight.z))
                    }
                    if ( err == null ) {
                      terraform_tile = 0
                    } else {
                      gui.add_message_at(b_player, " #3076# err " + err, world.get_time())
                    }
                  }

                } else if ( tile_build_slope.find(build_hight.get_slope()) != null && tile_a_slope_WE.find(tile_a.get_slope()) != null ) {

                  if ( test_tile_is_empty(tile_a1) && test_tile_is_empty(tile_b1) ) {
                    if ( straight_slope == true || build_side == 0 ) {
                      err = command_x.grid_lower(our_player, coord3d(tile_a.x, tile_a.y, tile_a.z))
                    } else if ( straight_slope == true || build_side == 1 ) {
                      err = command_x.grid_raise(our_player, coord3d(tile_b1.x, tile_b1.y, tile_b1.z))
                    } else {
                      err = command_x.grid_lower(our_player, coord3d(tile_b1.x, tile_b1.y, tile_b1.z))
                    }
                    if ( err == null ) {
                      terraform_tile = 0
                    } else {
                      gui.add_message_at(b_player, " #3070# err " + err, world.get_time())

                    }
                  }

                }

              }

              // double hight 8, 24, 56, 72

              // terraform tile
              if ( terraform_tile == 1 || (!straight_slope && build_hight.get_slope() > 0) ) { //&& ref_hight.z < build_hight.z
                if ( print_message_box == 21 ) {
                  gui.add_message_at(b_player, "#3132# terraform down ", world.get_time())
                  //gui.add_message_at(b_player, " tile_a1.get_slope() " + tile_a1.get_slope(), tile_a1)
                }
                do {
                  err = command_x.set_slope(b_player, build_hight, 83 )
                  if ( err != null ) { break }
                  z = square_x(tiles_build[i].x, tiles_build[i].y).get_ground_tile()
                } while(z.z > ref_hight.z )
                // replace water to land
                if ( z.is_water() ) { command_x.change_climate_at(our_player, z, cl_temperate) }

              }
            }
          }
          if ( err ) {
            return false
          }
          build_hight = square_x(tiles_build[i].x, tiles_build[i].y).get_ground_tile()
        }

        if ( ref_hight.z == build_hight.z && ref_hight.get_slope() == build_hight.get_slope() ) {

          if ( print_message_box == 21 ) {
            gui.add_message_at(b_player, "#3135# ---=> slope tiles == tiles_build * tiles.z == tiles_build.z ", world.get_time())
          }

        } else if ( build_hight.is_empty() && straight_slope == true ) {
          // set slope ramp
          if ( i < (tiles_build.len()-1) ) {
            if ( print_message_box == 21 ) {
              gui.add_message_at(b_player, " ---=> terraform slope ramp", world.get_time())
              gui.add_message_at(b_player, " ---=> tiles_build.z " + build_hight.z + " tiles.z " + ref_hight.z, world.get_time())
            }
            err = command_x.set_slope(b_player, build_hight, ref_hight.get_slope())
            if ( err != null ) {
              gui.add_message_at(b_player, " ERROR " + err, world.get_time())
              err = null
            }

          } else {
            if ( print_message_box == 21 ) {
              gui.add_message_at(b_player, " ---=> last tile slope ramp", world.get_time())
            }
            //tiles_build.remove(tiles_build.len()-1)
            //tiles.remove(tiles.len()-1)
            if ( tr == way_len ) {
              tr--
            }
            if ( tl == way_len ) {
              tl--
            }
            way_len--
          }
        }


      }

  local output = []
  output.append(tr)
  output.append(tl)
  output.append(way_len)
  return output
}

/*
 * search my_line_t from line_x
 *
 *
 */
function select_linkline(s_line) {

    foreach(link in link_list) {

      foreach(index, line in link.lines) {
        if ( line.is_valid() ) {
          //local s_line_name = s_line.get_name()
          //local line_name = line.get_name()
          if ( s_line.get_name() == line.get_name() ) {
            return line
          }

        }
      }

    }

}
