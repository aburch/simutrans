class rail_connector_t extends manager_t
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

  // print messages box
  // 1 = way
  // 2 = stations
  // 3 = depot
  print_message_box = 0

  constructor()
  {
    base.constructor("rail_connector_t")
    debug = true
  }

  function work()
  {
    // TODO check if child does the right thing
    local pl = our_player
    local tic = get_ops_total();

    local fs = fsrc.get_tile_list()
    local fd = fdest.get_tile_list()

    local s_src = null
    local s_dest = null

    if ( check_factory_links(fsrc, fdest, freight) >= 2 && phase == 0 ) {
      //gui.add_message_at(pl, "no build line from " + fsrc.get_name() + " (" + coord_to_string(fs[0]) + ") to " + fdest.get_name() + " (" + coord_to_string(fd[0]) + ") to many links", world.get_time())
      return r_t(RT_TOTAL_FAIL) //
    } else {
      //gui.add_message_at(pl, "check line from " + fsrc.get_name() + " (" + coord_to_string(fs[0]) + ") to " + fdest.get_name() + " (" + coord_to_string(fd[0]) + ") " + freight, world.get_time())
      local st_dock = search_station(c_start, wt_water, 1)
      if ( !check_factory_link_line(fsrc, fdest, freight) && !st_dock ) {
        //gui.add_message_at(pl, " st_dock " + st_dock, world.get_time())
        return r_t(RT_TOTAL_FAIL)
      }
    }

    local err = null

    switch(phase) {
      case 0: // find places for stations
        if ( print_message_box > 0 ) {
          gui.add_message_at(pl, "______________________ build rail ______________________", world.get_time())
          gui.add_message_at(pl, " line from " + fsrc.get_name() + " (" + coord_to_string(fs[0]) + ") to " + fdest.get_name() + " (" + coord_to_string(fd[0]) + ")", world.get_time())
        }
        if (c_start == null) {
          c_start = ::finder.find_station_place(fsrc, fdest)
        }
        if (c_start  &&  c_end == null) {
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
          local t_start = []
          local t_end = []
          local st_lenght = 0
          local d = pl.get_current_cash()
          local err = null

          // test route for calculate cost
          local calc_route = test_route(our_player, c_start, c_end, planned_way)
          if ( calc_route == "No route" || calc_route.routes.len() < 7 ) {
            return r_t(RT_TOTAL_FAIL)
          } else {
            if ( calc_route.routes.len() > 150 ) {
              //gui.add_message_at(our_player, "distance " + distance, world.get_time())
              gui.add_message_at(our_player, "calc route " + coord3d_to_string(c_start[0]) +  " to " + coord3d_to_string(c_end[0]) + ": way tiles = " + calc_route.routes.len() + " bridge tiles = " + calc_route.bridge_lens + " tree tiles = " + calc_route.tiles_tree, world.get_time())
            }
            local s = calc_route.routes.len()-3
            t_start = calc_route.routes.slice(s)
            t_start.reverse()
            //t_start.append(tile_x(c_start[0].x, c_start[0].y, c_start[0].z))
            t_end = calc_route.routes.slice(0, 3)
            //t_end.append(tile_x(c_end[0].x, c_end[0].y, c_end[0].z))
            // stations lenght
            local a = planned_convoy.length
            do {
              a -= 16
              st_lenght += 1
            } while(a > 0)

            if ( !planned_way.is_available(world.get_time()) ) {
              planned_way = find_object("way", wt_rail, planned_way.get_topspeed())
            }

            // check exist way
            local check_way = [0, 0, 0, 0, 0, 0]
            for ( local i = 0; i < 6; i++ ) {
              if ( i < 3 ) {
                // start
                if ( tile_x(t_start[i].x, t_start[i].y, t_start[i].z).find_object(mo_way) ) {
                  check_way[i] = 1
                }
              } else {
                // end
                if ( tile_x(t_end[i-3].x, t_end[i-3].y, t_end[i-3].z).find_object(mo_way) ) {
                  check_way[i] = 1
                }
              }
            }

            err = command_x.build_way(pl, t_start[0], t_start[1], planned_way, true)
            if ( err != null ) { gui.add_message_at(pl, "check_station build_way " + err, t_start[0]) }
            err = command_x.build_way(pl, t_start[1], t_start[2], planned_way, true)
            if ( err != null ) { gui.add_message_at(pl, "check_station build_way " + err, t_start[0]) }
            if ( err == null ) {
              err = check_station(pl, t_start[0], st_lenght, wt_rail, planned_station, 0)
              if ( err == false ) {
                gui.add_message_at(pl, "check_station " + err, t_start[0])
              }
              if ( err == true ) {
                // station start ok
                err = command_x.build_way(pl, t_end[0], t_end[1], planned_way, true)
                if ( err != null ) { gui.add_message_at(pl, "check_station build_way t_end " + err, t_end[0]) }
                err = command_x.build_way(pl, t_end[1], t_end[2], planned_way, true)
                if ( err != null ) { gui.add_message_at(pl, "check_station build_way t_end " + err, t_end[0]) }
                //local tool = command_x(tool_remove_way)
                if ( err == null ) {
                  err = check_station(pl, t_end[0], st_lenght, wt_rail, planned_station, 0)
                  if ( err == true ) {
                    // station end ok
                    // remove track -> error by build
                    for ( local i = 0; i < 6; i++ ) {
                      if ( check_way[i] == 0 && i < 3 ) {
                        remove_tile_to_empty(t_start[i], wt_rail, 0)
                      } else if ( check_way[i] == 0 && i > 2 ) {
                        remove_tile_to_empty(t_end[i-3], wt_rail, 0)
                      }
                    }
                    //tool.work(our_player, t_start[0], t_start[2], "" + wt_rail)
                    //tool.work(our_player, t_end[0], t_end[2], "" + wt_rail)
                  } else {
                    // failed station place end
                    for ( local i = 0; i < 6; i++ ) {
                      if ( check_way[i] == 0 && i < 3 ) {
                        remove_tile_to_empty(t_start[i], wt_rail, 0)
                      } else if ( check_way[i] == 0 && i > 2 ) {
                        remove_tile_to_empty(t_end[i-3], wt_rail, 0)
                      }
                    }

                    return error_handler()
                  }
                } else {
                  // remove start and end
                    for ( local i = 0; i < 6; i++ ) {
                      if ( check_way[i] == 0 && i < 3 ) {
                        remove_tile_to_empty(t_start[i], wt_rail, 0)
                      } else if ( check_way[i] == 0 && i > 2 ) {
                        remove_tile_to_empty(t_end[i-3], wt_rail, 0)
                      }
                    }
                }
              } else {
                // failed station place start
                // remove start
                //::debug.pause()
                for ( local i = 0; i < 3; i++ ) {
                  if ( check_way[i] == 0 && i < 3 ) {
                    remove_tile_to_empty(t_start[i], wt_rail, 0)
                  }
                }
                return error_handler()
              }
            } else {
              // no built way start -> remove start
              for ( local i = 0; i < 3; i++ ) {
                if ( check_way[i] == 0 && i < 3 ) {
                  remove_tile_to_empty(t_start[i], wt_rail, 0)
                }
              }
            }
            if ( print_message_box > 0 ) {
              gui.add_message_at(pl, "plan station start " + t_start[0] + " - plan station end " + t_end[0], t_start[2])
            }
          }

          sleep()
          local build_cost = (calc_route.routes.len() * planned_way.get_cost()) + ((st_lenght*2)*planned_station.get_cost()) + planned_depot.get_cost() + (calc_route.bridge_lens * calc_route.bridge_obj.get_cost())
          local cost_monthly = (calc_route.routes.len() * planned_way.get_maintenance()) + ((st_lenght*2)*planned_station.get_maintenance()) + planned_depot.get_maintenance() + (calc_route.bridge_lens * calc_route.bridge_obj.get_maintenance())
          build_cost = build_cost/100

          build_cost = build_cost + (calc_route.tiles_tree * ((tree_desc_x.get_price()/100)*2))

          //gui.add_message_at(pl, "tree remove cost: " + tree_desc_x.get_price(), world.get_time())
          //gui.add_message_at(pl, "terraform cost: " + command_x.slope_get_price(82), world.get_time())

          // terraform cost
          local terraform_cost = command_x.slope_get_price(82)/100
          build_cost += st_lenght*terraform_cost

          //gui.add_message_at(pl, "cash: " + pl.get_current_cash() + " build cost: " + build_cost + " montly cost: " + (cost_monthly/100), world.get_time())
          cost_monthly = (cost_monthly/100)+(pl.get_current_maintenance()/100)
          //gui.add_message_at(pl, "cash: " + pl.get_current_cash() + " current_maintenance(): " + pl.get_current_maintenance() + " get_maintenance()[0]: " + (-pl.get_maintenance()[0]), world.get_time())
          //gui.add_message_at(pl, " montly cost new: " + cost_monthly, world.get_time())

          sleep()
          // if combined station from ship
          local cash = pl.get_current_cash()
          local st_dock = search_station(t_start[0], wt_water, 1)
          //gui.add_message_at(our_player, "search_station(t_start[0], wt_water, 1) " + st_dock + " t_start[0] " + coord3d_to_string(t_start[0]), world.get_time())
          if ( st_dock ) {
            local st = halt_x.get_halt(st_dock[0], our_player)
            if ( st ) {
              local fl_st = st.get_factory_list()
              if ( fl_st.len() == 0 ) {
                cash = our_player.get_current_net_wealth()
                gui.add_message_at(our_player, "rail: combined station -> get_current_net_wealth() " + (our_player.get_current_net_wealth()/100), world.get_time())
              } else {

              }
            }
          }

          if ( (cash-build_cost) < (cost_monthly*4) ) {
            //remove_tile_to_empty(t_start, wt_rail, 1)
            //remove_tile_to_empty(t_end, wt_rail, 1)
            industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_missing)
            gui.add_message_at(pl, "Way construction cost to height: cash: " + cash + " build cost: " + build_cost, world.get_time())
            return error_handler()
          }
/*
          if ( calc_route.routes.len() > 150 && (cash-build_cost-(build_cost/2)) < (cost_monthly*10) ) {
            //remove_tile_to_empty(t_start, wt_rail, 1)
            //remove_tile_to_empty(t_end, wt_rail, 1)
            industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_missing)
            gui.add_message_at(pl, "Way to long for rentabel build.", world.get_time())
            return error_handler()
          }
*/
          //gui.add_message_at(pl, "c_start.len() " + c_start.len() + " - c_end.len() " + c_end.len(), world.get_time())
          err = construct_rail(pl, c_start, c_end, planned_way )
          print("Way construction cost: " + (d-pl.get_current_cash()) )

          if (err) { // fail, c_start, c_end still arrays
            print("Failed to build way from " + coord_to_string(c_start[0]) + " to " + coord_to_string(c_end[0]))
            return error_handler()
          }
          else {
            // success, now c_start, c_end contain the start/end points where something was built
            if ( print_message_box == 1 ) {
              gui.add_message_at(pl, "Build rail from " + coord_to_string(c_start) + " to " + coord_to_string(c_end), world.get_time())
            }
            // test connect in double way
            local asf = astar_route_finder(wt_rail)
            // check start -> end
            local wayline = asf.search_route([c_start], [c_end])
            if ( "err" in wayline ) {
              // no route found
            } else {
              check_doubleway_in_line(wayline, wt_rail)
              // check end -> start
              wayline.clear()
              wayline = asf.search_route([c_end], [c_start])
              check_doubleway_in_line(wayline, wt_rail)

            }
          }
          phase ++
        }
      case 2: // build station
        {
          /*
          local err = command_x.build_station(pl, c_start, planned_station )
          if (err) {
            print("Failed to build station at " + coord_to_string(c_start))
            if ( print_message_box == 2 ) { gui.add_message_at(pl, "Failed to build rail station at  " + coord_to_string(c_start) + " error " + err, world.get_time()) }
            return error_handler()
          }
          */
          if ( print_message_box == 2 ) {
            gui.add_message_at(pl, " planned_convoy.length " + planned_convoy.length, world.get_time())
          }

          // stations lenght
          local a = planned_convoy.length
          local count = 0
          do {
            a -= 16
            count += 1
          } while(a > 0)

          if ( print_message_box == 1 ) {
            gui.add_message_at(our_player, " rail_connector stations lenght: " + count, world.get_time())
          }

          // check place and build station to c_start
          s_src = check_station(pl, c_start, count, wt_rail, planned_station)
          if ( s_src == false ) {
            print("Failed to build station at " + coord_to_string(c_start))
            if ( print_message_box > 0 ) {
              gui.add_message_at(pl, "Failed to build rail station at s_src " + coord3d_to_string(c_start), world.get_time())
            }
            remove_wayline(c_route, c_route.len()-1, wt_rail)
            return error_handler()
          }

          // low cost station for line end
          local station_list = building_desc_x.get_available_stations(building_desc_x.station, wt_rail, good_desc_x(freight))
          local x = 0
          local station_select = planned_station
          foreach(station in station_list) {
            if ( station.cost < station_select.cost ) {
              station_select = station
            }
          }
          // check place and build station to c_end
          s_dest = check_station(pl, c_end, count, wt_rail, station_select)
          if ( s_dest == false ) {
            print("Failed to build station at " + coord_to_string(c_end))
            if ( print_message_box > 0 ) {
              gui.add_message_at(pl, "Failed to build rail station at s_dest " + coord_to_string(c_end), world.get_time())
            }

            remove_wayline(c_route, c_route.len()-1, wt_rail, s_src.len())
            remove_tile_to_empty(s_src, wt_rail)
            remove_tile_to_empty(c_start, wt_rail, 0)
            remove_tile_to_empty(c_end, wt_rail, 0)
            return error_handler()
          }

          //local
          if ( finalize ) {
            // store place of unload station for future use
            local fs = ::station_manager.access_freight_station(fdest)
            if (fs.rail_unload == null) {
              fs.rail_unload = c_end

              print( recursive_save({unload = c_end}, "\t\t\t", []) )
            }
          }

          if ( print_message_box == 2 ) {
            //gui.add_message_at(our_player, " ... rotate " + rotate, world.get_time())
            gui.add_message_at(pl, "Build station on " + coord3d_to_string(c_start) + " and " + coord3d_to_string(c_end), world.get_time())
          }

          phase ++
        }
      case 3: // find depot place
        {
          phase += 2
        }
      case 5: // build depot
        {
          if ( print_message_box == 3 ) {
            gui.add_message_at(pl, "___________ exists depots rail ___________", world.get_time())
            gui.add_message_at(pl," c_start pos: " + coord_to_string(c_start) + " : c_end pos: " + coord_to_string(c_end), world.get_time())
          }

          // search depot to range start and end station
          local depot_found = search_depot(c_start, wt_rail)
          local starts_field = c_start
          if ( !depot_found ) {
            depot_found = search_depot(c_end, wt_rail)
            starts_field = c_end
          }

          if ( !depot_found && print_message_box == 3 ) {
            gui.add_message_at(pl," *** depot not found *** ", world.get_time())
          } else if ( print_message_box == 3 ) {
            gui.add_message_at(pl," ---> depot found : " + depot_found.get_pos(), coord_to_string(depot_found))
          }

          // build rail to depot
          if ( depot_found ) {
            c_depot = depot_found
            local err = command_x.build_road(pl, starts_field, c_depot, planned_way, false, true)
          } else {
            local i = 0
            if ( c_route.len() > 10 ) {
              i = c_route.len() - 5 - s_src.len()
            }
            local err = construct_rail_to_depot(pl, c_route[i], planned_way) //c_start
            if (err) {
              print("Failed to build depot access from " + coord_to_string(c_start))
              return error_handler()
            }
            // depot already existing ?
            if (c_depot.find_object(mo_depot_rail) == null) {
              // no: build
              local err = command_x.build_depot(pl, c_depot, planned_depot )
              if (err) {
                print("Failed to build depot at " + coord_to_string(c_depot))
                return error_handler()
              }
              if (finalize) {
                // store depot location
                local fs = ::station_manager.access_freight_station(fsrc)
                if (fs.rail_depot == null) {
                  fs.rail_depot = c_depot
                }
              }
            }
            if ( print_message_box == 1 ) { gui.add_message_at(pl, "Build depot on " + coord_to_string(c_depot), world.get_time()) }
          }

          phase ++
        }
      case 6: // create schedule
        {
          local sched = schedule_x(wt_rail, [])
          sched.entries.append( schedule_entry_x(c_start, 100, 0) );
          sched.entries.append( schedule_entry_x(c_end, 0, 0) );
          c_sched = sched
          phase ++
        }

      case 7: // create line and set schedule
        {
          pl.create_line(wt_rail)

          // find the line - it is a line without schedule and convoys
          local list = pl.get_line_list()
          foreach(line in list) {
            if (line.get_waytype() == wt_rail  &&  line.get_schedule().entries.len()==0) {
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
          c.p_count  = min(planned_convoy.nr_convoys, 1)
          append_child(c)

          local toc = get_ops_total();
          print("rail_connector wasted " + (toc-tic) + " ops")

          phase ++

          return r_t(RT_PARTIAL_SUCCESS)
        }
      case 9: // build station extension
        {
          // optimize way line save in c_route
          if ( tile_x(c_start.x, c_start.y, c_start.z).find_object(mo_building) != null && tile_x(c_end.x, c_end.y, c_end.z).find_object(mo_building) != null && c_route.len() > 0 ) {
            // tile c_start ans c_end have station
            if (our_player.get_current_cash() > 50000) {
              //optimize_way_line(c_route, wt_rail)
            }

            // rename line
            local line_name = c_line.get_name()
            local str_search = ") " + translate("Line")
            local st_names = c_line.get_schedule().entries
            if ( line_name.find(str_search) != null ) {
              local new_name = translate("Train") + " " + translate(freight) + " " + st_names[0].get_halt(pl).get_name() + " - " + st_names[1].get_halt(pl).get_name()
              c_line.set_name(new_name)
            }

          }
        }

    }

    if (finalize) {
      industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_built)
    }
    industry_manager.access_link(fsrc, fdest, freight).append_line(c_line)

    local cs = tile_x(c_start.x, c_start.y, c_start.z)
    local ce = tile_x(c_end.x, c_end.y, c_end.z)

    /*if (c_start.len()>0  &&  c_end.len()>0) {
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
    local msgtext = format(translate("%s build rail line from %s (%s) to %s (%s)"), pl.get_name(), f_name[0], coord_to_string(cs), f_name[1], coord_to_string(ce))
    //gui.add_message_at(pl, pl.get_name() + " build rail line from " + f_name[0] + " (" + coord_to_string(cs) + ") to " + f_name[1] + " (" + coord_to_string(ce) + ")", c_start)
    gui.add_message_at(pl, msgtext, c_start)

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

  function construct_rail(pl, starts, ends, way)
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

  function construct_rail_to_depot(pl, start, way)
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
    local depot = t.find_object(mo_depot_rail)
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
          gui.add_message_at(our_player, "Failed to build rail from  " + coord_to_string(route[i-1]) + " to " + coord_to_string(route[i]) +"\n" + err, route[i])
          return { err =  err }
        }
      }
      return { start = route.top(), end = route[0] }
    }
    print("No route found")
    return { err =  "No route" }
  }
}
