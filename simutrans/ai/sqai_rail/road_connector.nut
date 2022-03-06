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
  finalize = true

  // step-by-step construct the connection
  phase = 0
  // can be provided optionally
  c_start = null // array
  c_end   = null // array
  c_route = null // array
  // generated data
  c_depot = null
  c_sched = null
  c_line  = null
  c_cnv   = null
  c_generate_start = 0 // compute start/end ourselves
  c_generate_end   = 0
  c_trial_route    = 0 // we tried to build route & stations several times

  // print messages box
  // 1 = way
  // 2 = station
  // 3 = depot
  print_message_box = 0

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

    c_generate_start = c_start == null
    c_generate_end   = c_end   == null

    local fs = fsrc.get_tile_list()
    local fd = fdest.get_tile_list()

    if ( check_factory_links(fsrc, fdest, freight) >= 2 && phase == 0 ) {
      //gui.add_message_at(pl, "no build line from " + fsrc.get_name() + " (" + coord_to_string(fs[0]) + ") to " + fdest.get_name() + " (" + coord_to_string(fd[0]) + ") to many links", world.get_time())
      return r_t(RT_TOTAL_FAIL) //
    } else {
      //gui.add_message_at(pl, "check line from " + fsrc.get_name() + " (" + coord_to_string(fs[0]) + ") to " + fdest.get_name() + " (" + coord_to_string(fd[0]) + ") " + freight, world.get_time())
      local st_dock = search_station(c_start, wt_water, 1)
      if ( !check_factory_link_line(fsrc, fdest, freight) && !st_dock ) {
        return r_t(RT_TOTAL_FAIL)
      }
    }

    switch(phase) {
      case 0:  // station places
        if ( print_message_box > 0 ) {
          gui.add_message_at(our_player, "______________________ build road ______________________", world.get_time())
          gui.add_message_at(pl, " line from " + fsrc.get_name() + " (" + coord_to_string(fs[0]) + ") to " + fdest.get_name() + " (" + coord_to_string(fd[0]) + ")", world.get_time())
        }
        // count trials, and fail if necessary
        if (c_trial_route > 3) {
          print("Route building failed " + c_trial_route + " times.")
          gui.add_message_at(pl, "Failed to complete route from  " + coord_to_string(fsrc) + " to " + coord_to_string(fdest) + " after " + c_trial_route + " attempts", fsrc)
          return error_handler()
        }
        c_trial_route++
        // make arrays if necessary
        if (type(c_start) == "instance") { c_start = [c_start] }
        if (type(c_end)   == "instance") { c_end   = [c_end]   }

        // find places for stations
        if (c_generate_start) {
          c_start = ::finder.find_station_place(fsrc, fdest)
        }
        if (c_generate_end) {
          c_end   = ::finder.find_station_place(fdest, c_start, finalize)
        }

        if (c_start.len()>0  &&  c_end.len()>0) {
          phase ++
        }
        else {
          print("No station places found")
          return error_handler()
        }
      case 1: // build way
        {
          sleep()
          local d = pl.get_current_cash();

          // test route for calculate cost
          local calc_route = test_route(our_player, c_start, c_end, planned_way)
          //gui.add_message_at(our_player, "distance " + distance, world.get_time())
          if ( calc_route == "No route" ) {
            return r_t(RT_TOTAL_FAIL)
          }

          //gui.add_message_at(our_player, "calc route " + coord3d_to_string(c_start[0]) +  " to " + coord3d_to_string(c_end[0]) + ": way tiles = " + calc_route.routes.len() + " bridge tiles = " + calc_route.bridge_lens + " tree tiles = " + calc_route.tiles_tree, world.get_time())

          sleep()
          local build_cost = (calc_route.routes.len() * planned_way.get_cost()) + (planned_station.get_cost()*2) + planned_depot.get_cost() + (calc_route.bridge_lens * calc_route.bridge_obj.get_cost())
          local cost_monthly = (calc_route.routes.len() * planned_way.get_maintenance()) + (planned_station.get_maintenance()*2) + planned_depot.get_maintenance() + (calc_route.bridge_lens * calc_route.bridge_obj.get_maintenance())
          build_cost = build_cost/100

          build_cost = build_cost + (calc_route.tiles_tree * (tree_desc_x.get_price()/100))

          //gui.add_message_at(pl, "tree remove cost: " + tree_desc_x.get_price(), world.get_time())

          //gui.add_message_at(pl, "cash: " + pl.get_current_cash() + " build cost: " + build_cost + " montly cost: " + (cost_monthly/100), world.get_time())
          cost_monthly = (cost_monthly/100)+(pl.get_current_maintenance()/100)
          //gui.add_message_at(pl, "cash: " + pl.get_current_cash() + " current maintenance: " + pl.get_current_maintenance(), world.get_time())
          //gui.add_message_at(pl, " montly cost new: " + cost_monthly, world.get_time())

          sleep()
          // if combined station from ship
          local cash = pl.get_current_cash()
          local st_dock = search_station(calc_route.routes[calc_route.routes.len()-1], wt_water, 1)
          if ( st_dock ) {
            local st = halt_x.get_halt(st_dock[0], our_player)
            if ( st ) {
              local fl_st = st.get_factory_list()
              if ( fl_st.len() == 0 ) {
                cash = our_player.get_current_net_wealth()
                gui.add_message_at(our_player, "road: combined station -> get_current_net_wealth() " + (our_player.get_current_net_wealth()/100), world.get_time())
              } else {

              }
            }
          }

          if ( (cash-build_cost) < (cost_monthly*4) ) {
            //gui.add_message_at(pl, "Way construction cost to height: cash: " + pl.get_current_cash() + " build cost: " + build_cost, world.get_time())
            industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_missing)

            return error_handler()
          }

          if ( !planned_way.is_available(world.get_time()) ) {
            planned_way = find_object("way", wt_road, planned_way.get_topspeed())
          }

          local err = construct_road(pl, c_start, c_end, planned_way )
          print("Way construction cost: " + (d-pl.get_current_cash()) )
          if (err) {

            if ( c_start.len()>0  &&  c_end.len()>0) {
              print("Failed to build way from " + coord_to_string(c_start[0]) + " to " + coord_to_string(c_end[0]))
            } else if (err) {
              print("Failed to build way from " + coord_to_string(c_start) + " to " + coord_to_string(c_end))
            }

            if (err == "No route") {
              print("No route found from " + coord_to_string(c_start[0])+ " to " + coord_to_string(c_end[0]))
              // no point to try again
              return error_handler()
            }
            // try again
            return restart_with_phase0()

          }
          if ( print_message_box == 1 ) { gui.add_message_at(our_player, "Build road from " + coord_to_string(c_start) + " to " + coord_to_string(c_end), world.get_time()) }
          phase ++
        }
      case 2: // build station
        {
          if ( tile_x(c_start.x, c_start.y, c_start.z).find_object(mo_building) != null ) {
            if (debug) gui.add_message_at(pl, " --- tile to build station not free", world.get_time())
            return restart_with_phase0()
          }
          local err = command_x.build_station(pl, c_start, planned_station )
          if (err) {
            if (debug) gui.add_message_at(pl, "Failed to build road station at  " + coord_to_string(c_start) + " [" + err + "]", c_start)// try again
            if ( print_message_box == 2 ) { gui.add_message_at(pl, "Failed to build road station at  " + coord_to_string(c_start) + " error " + err, world.get_time()) }
            return restart_with_phase0()
          }
          else {
            c_generate_start = false // station build, do not search for another place
          }
          local err = command_x.build_station(pl, c_end, planned_station )
          if (err) {
          if (debug) gui.add_message_at(pl, "Failed to build road station at  " + coord_to_string(c_end) + " [" + err + "]", c_end)
            // try again
            return restart_with_phase0()
          }
          else {
            c_generate_end = false // station build, do not search for another place
          }

          if (finalize) {
            // store place of unload station for future use
            local fs = ::station_manager.access_freight_station(fdest)
            if (fs.road_unload == null) {
              fs.road_unload = c_end
            }
          }
          if (debug  &&  c_trial_route>1) {
            gui.add_message_at(pl, "Completed route from  " + coord_to_string(c_start) + " to " + coord_to_string(c_end) + " after " + c_trial_route + " attempts", c_end)
          }
          if ( print_message_box == 2 ) { gui.add_message_at(our_player, "Build station on " + coord_to_string(c_start) + " and " + coord_to_string(c_end), world.get_time()) }

          //
          local asf = astar_route_finder(wt_road)
          local result = asf.search_route([c_start], [c_end])
          if (  result.routes.len() <= 3 ) {
            //gui.add_message_at(pl, "route len < 3 tiles  ", c_end)
            local extension = find_extension(wt_road)
            local start_h = tile_x(c_start.x, c_start.y, c_start.z).get_halt().get_name()
            local end_h = tile_x(c_end.x, c_end.y, c_end.z).get_halt().get_name()

            if ( extension != null && start_h == end_h ) {
              remove_tile_to_empty(c_start, wt_road, 0)
              command_x.build_station(our_player, c_start, extension)
              remove_tile_to_empty(c_end, wt_road, 0)
              command_x.build_station(our_player, c_end, extension)
              if ( result.routes.len() == 3 ) {
                foreach(node in result.routes) {
                  local tile = tile_x(node.x, node.y, node.z)
                  if ( tile.find_object(mo_building) == null ) {
                    remove_tile_to_empty(tile, wt_road, 0)
                  }
                }
              }
            }
            return r_t(RT_TOTAL_SUCCESS)
          }

          phase += 3
        }
      case 3: // find depot place
        {
          /*
          local trial = 0
          local err = null
          do { // try 3x to find road to suitable depot spot
            err = construct_road_to_depot(pl, c_start, planned_way)
            trial ++
          } while (err != null  &&  err != "No route"  &&  trial < 3)
          if (err) {
            print("Failed to build depot access from " + coord_to_string(c_start))
            return error_handler()
          }
          */
          //phase += 2
        }
      case 5: // build depot
        {


          if ( print_message_box == 3 ) {
            gui.add_message_at(our_player, "___________ exists depots road ___________", world.get_time())
            gui.add_message_at(our_player," c_start pos: " + coord_to_string(c_start) + " : c_end pos: " + coord_to_string(c_end), world.get_time())
          }

          // search depot to range start and end station
          local depot_found = search_depot(c_start, wt_road)
          local starts_field = c_start
          if ( !depot_found ) {
            depot_found = search_depot(c_end, wt_road)
            starts_field = c_end
          }

          if ( !depot_found && print_message_box == 3 ) {
            gui.add_message_at(pl," *** depot not found *** ", world.get_time())
          } else if ( print_message_box == 3 ) {
            gui.add_message_at(pl," ---> depot found : " + depot_found.get_pos(), coord_to_string(depot_found))
          }


          local err = null
          // build road to depot
          if ( depot_found ) {
            c_depot = depot_found
            err = command_x.build_road(pl, starts_field, c_depot, planned_way, false, true)
            //err = construct_road(our_player, station_to_depot, c_depot, planned_way)
          } else {
            local i = c_route.len()-1
            if ( i > 4 ) { i -= 4 }

            local trial = 0
            local err = null
            do { // try 3x to find road to suitable depot spot
              err = construct_road_to_depot(pl, c_route[i], planned_way)
              trial ++
            } while (err != null  &&  err != "No route"  &&  trial < 3)

            if (err) {
              // we do not like to fail at this phase, try to find another spot
              phase = 3
              return r_t(RT_PARTIAL_SUCCESS)
            }

            // depot already existing ?
            if (c_depot.find_object(mo_depot_road) == null) {
              // no: build
              local err = command_x.build_depot(pl, c_depot, planned_depot )
              if (err) {
                print("Failed to build depot at " + coord_to_string(c_depot))
                return error_handler()
              }
              if (finalize) {
                // store depot location
                local fs = ::station_manager.access_freight_station(fsrc)
                if (fs.road_depot == null) {
                  fs.road_depot = c_depot
                }
              }
            }
            if ( print_message_box == 3 ) { gui.add_message_at(our_player, "Build depot on " + coord_to_string(c_depot), world.get_time()) }
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
          if ( world.get_time().year < 1935 ) {
            c.p_count  = min(planned_convoy.nr_convoys, 6)
          } else {
            c.p_count  = min(planned_convoy.nr_convoys, 3)
          }
          append_child(c)

          local toc = get_ops_total();
          print("road_connector wasted " + (toc-tic) + " ops")
          c_generate_start = c_start == null
          c_generate_end   = c_end   == null

          phase ++

          return r_t(RT_PARTIAL_SUCCESS)
        }
      case 9: // build station extension
        {
          // optimize way line save in c_route
          if ( tile_x(c_start.x, c_start.y, c_start.z).find_object(mo_building) != null && tile_x(c_end.x, c_end.y, c_end.z).find_object(mo_building) != null && c_route.len() > 0 ) {
            // tile c_start ans c_end have station
            if (our_player.get_current_cash() > 50000) {
              //optimize_way_line(c_route, wt_road)
            }

            // rename line
            local line_name = c_line.get_name()
            local str_search = ") " + translate("Line")
            local st_names = c_line.get_schedule().entries
            if ( line_name.find(str_search) != null ) {
              local new_name = translate("road") + " " + translate(freight) + " " + st_names[0].get_halt(pl).get_name() + " - " + st_names[1].get_halt(pl).get_name()
              c_line.set_name(new_name)
            }
          }
        }
    }

    if (finalize) {
      industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_built)
    }
    industry_manager.access_link(fsrc, fdest, freight).append_line(c_line)

    local cs = c_start
    local ce = c_end
    /*
    if (c_start.len()>0  &&  c_end.len()>0) {
      local cs = c_start[0]
      local ce = c_end[0]
    }*/

    local st = halt_x.get_halt(cs, pl)
    local f_name = ["", ""]
    if ( st ) {
      local fl_st = st.get_factory_list()
      if ( fl_st.len() > 0 ) {
        f_name[0] = fl_st[0].get_name()
      } else {
        f_name[0] = st.get_name()
      }
    }
    st = halt_x.get_halt(ce, pl)
    if ( st ) {
      local fl_st = st.get_factory_list()
      if ( fl_st.len() > 0 ) {
        f_name[1] = fl_st[0].get_name()
      } else {
        f_name[1] = st.get_name()
      }
    }
    local msgtext = format(translate("%s build road line from %s (%s) to %s (%s)"), pl.get_name(), f_name[0], coord_to_string(cs), f_name[1], coord_to_string(ce))
    //gui.add_message_at(pl, pl.get_name() + " build road line from " + f_name[0] + " (" + coord_to_string(cs) + ") to " + f_name[1] + " (" + coord_to_string(ce) + ")", c_start)
    gui.add_message_at(pl, msgtext, c_start)

    return r_t(RT_TOTAL_SUCCESS)
  }

  function restart_with_phase0()
  {
    if (c_generate_start) { c_start = null }
    if (c_generate_end  ) { c_end   = null }
    phase = 0
    return r_t(RT_PARTIAL_SUCCESS)
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
    c_route = res.routes

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
    local depot = t.find_object(mo_depot_road)
    if (depot  &&  depot.get_owner().nr == our_player_nr) {
      return 0
    }
    return 10
  }
  function add_to_open(c, weight)
  {
    if (c.dist == 0) {
      // test for depot
      local t = tile_x(c.x, c.y, c.z)
      if (t.is_empty()) {
        // depot not existing, we must build, increase weight
        weight += 25 * cost_straight
      }
    }
    base.add_to_open(c, weight)
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
          return { err =  err }
        }
      }
      return { start = route.top(), end = route[0] }
    }
    print("No route found")
    return { err =  "No route" }
  }
}
