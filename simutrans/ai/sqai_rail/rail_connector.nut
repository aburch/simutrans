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

  t_start = []
  t_end = []
  st_lenght = 0

  pl = null

  // print messages box
  // 1 = way
  // 2 = stations
  // 3 = depot
  print_message_box = 0

  constructor()
  {
    base.constructor("rail_connector_t")
    debug = false
  }

  function work()
  {
    // TODO check if child does the right thing
    pl = our_player
    local tic = get_ops_total();

    local fs = fsrc.get_tile_list()
    local fd = fdest.get_tile_list()

    local s_src = null
    local s_dest = null



    if ( phase == 0 ) {
      if ( !check_link_catg_goods(fsrc, fdest, freight) ) {
        //gui.add_message_at(pl, "no build line from " + fsrc.get_name() + " (" + coord_to_string(fs[0]) + ") to " + fdest.get_name() + " (" + coord_to_string(fd[0]) + ") exists catg links", world.get_time())
        return r_t(RT_TOTAL_FAIL) //
      } else if ( check_factory_links(fsrc, fdest, freight) >= 2 ) {
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

      local build_cash = player_x(our_player.nr).get_current_cash() + (((player_x(our_player.nr).get_current_net_wealth()/100) - player_x(our_player.nr).get_current_cash())/2)
      local build_cost = industry_manager.get_link_build_cost(fsrc, fdest, freight, 1)
      if ( industry_manager.get_link_build_cost(fsrc, fdest, freight, 0) > 0 ) {
        build_cost = industry_manager.get_link_build_cost(fsrc, fdest, freight, 0)
      }
      if ( (build_check_month > world.get_time().ticks || build_cost > build_cash) && industry_manager.get_combined_link(fsrc, fdest, freight) == 0 ) {
        // not build link
        if ( debug ) gui.add_message_at(our_player, "#rail_conn# not build line : build_check_month = " + build_check_month + " or build cost link > cash : build cost line " + industry_manager.get_link_build_cost(fsrc, fdest, freight, 1) + " | build cost link " + industry_manager.get_link_build_cost(fsrc, fdest, freight, 0), world.get_time())
        if ( debug ) gui.add_message_at(our_player, " ---> link " + fsrc + "  " + fsrc.get_name() + " - " + fdest.get_name(), world.get_time())
        //if ( debug ) gui.add_message_at(our_player, " ---> phase " + phase, world.get_time())

        industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_missing)

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
          local d = pl.get_current_cash()
          local err = null

          local line_start = null

          if ( (abs(c_start[0].x - c_end[0].x) + abs(c_start[0].y - c_end[0].y)) < 13 && industry_manager.get_combined_link(fsrc, fdest, freight) > 0) {
            //gui.add_message_at(pl, "c_start " + coord_to_string(c_start[0]) + " c_end " + coord_to_string(c_end[0]), c_end[0])
            local d = settings.get_station_coverage()
            local f = 0
            for ( local i = 0; i < fd.len(); i++ ) {
              f = abs(c_start[0].x - fd[i].x) + abs(c_start[0].y - fd[i].y)
              if ( f <= d ) {
                local extension = find_extension(wt_rail)
                if ( extension != null ) {
                  local err = command_x.build_station(our_player, c_start[0], extension)
                  if ( err == null ) {

                    return r_t(RT_TOTAL_SUCCESS)
                  }
                }
              }
            }
            if ( debug ) {
              gui.add_message_at(pl, "c_start " + coord_to_string(c_start[0]) + " c_end " + coord_to_string(c_end[0]), c_end[0])
              ::debug.pause

            }
          }

          // test route for calculate cost
          local calc_route = null
          local r1 = test_route(our_player, c_start, c_end, planned_way)
          local r2 = test_route(our_player, c_end, c_start, planned_way)
          if ( r1 == "No route" && r2 == "No route" ) {
            return r_t(RT_TOTAL_FAIL)
          } else if ( r1 == "No route" && r2 != "No route" ) {
            calc_route = r2
          } else if ( r1 != "No route" && r2 == "No route" ) {
            calc_route = r1
          } else if ( r1.routes.len() > r2.routes.len() ) {
            calc_route = r2
            r1 = null
          } else {
            calc_route = r1
          }

          local build_status = null
          if ( calc_route.routes.len() < 13 ) {

            //gui.add_message_at(pl, "route len < 13 tiles  ", c_end)
            //gui.add_message_at(pl, "c_start " + coord_to_string(c_start) + " c_end " + coord_to_string(c_end), c_end)
            local extension = find_extension(wt_rail)
            //local start_h = tile_x(c_start.x, c_start.y, c_start.z).get_halt().get_name()
            //local end_h = tile_x(c_end.x, c_end.y, c_end.z).get_halt().get_name()

            return error_handler()
          } else {
            build_status = check_build_station(calc_route)
            //gui.add_message_at(pl, "(129) build_status : " + build_status, world.get_time())
            //::debug.pause()
            if ( build_status != true ) {
              calc_route = test_route(our_player, c_end, c_start, planned_way)
              build_status = check_build_station(calc_route)
              if ( build_status == true ) {
                local c = c_start
                c_start = c_end
                c_end = c
                line_start = t_end[0]
              } else {
                return r_t(RT_TOTAL_FAIL)
              }
            } else {
              if ( build_status == true ) {
                line_start = t_start[0]
              } else {
                return r_t(RT_TOTAL_FAIL)
              }
            }
          }

          //gui.add_message_at(pl, "(146) build_status : " + build_status, world.get_time())

          sleep()
          local build_cost = (calc_route.routes.len() * planned_way.get_cost()) + ((st_lenght*2)*planned_station.get_cost()) + planned_depot.get_cost() + (calc_route.bridge_lens * calc_route.bridge_obj.get_cost())
          local cost_monthly = (calc_route.routes.len() * planned_way.get_maintenance()) + ((st_lenght*2)*planned_station.get_maintenance()) + planned_depot.get_maintenance() + (calc_route.bridge_lens * calc_route.bridge_obj.get_maintenance())
          build_cost = build_cost/100
          //build_cost +
          build_cost += (calc_route.tiles_tree * ((tree_desc_x.get_price()/100)*2))

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
          local st_dock = search_station(line_start, wt_water, 1)
          //gui.add_message_at(our_player, "search_station(t_start[0], wt_water, 1) " + st_dock + " t_start[0] " + coord3d_to_string(t_start[0]), world.get_time())
          if ( st_dock ) {
            local st = halt_x.get_halt(st_dock[0], our_player)
            if ( st ) {
              local fl_st = st.get_factory_list()
              if ( fl_st.len() == 0 ) {
                cash = our_player.get_current_net_wealth()
                //gui.add_message_at(our_player, "rail: combined station -> get_current_net_wealth() " + (our_player.get_current_net_wealth()/100), world.get_time())
              } else {

              }
            }
          }

          if ( (cash-build_cost) < (cost_monthly*4) ) {
            //remove_tile_to_empty(t_start, wt_rail, 1)
            //remove_tile_to_empty(t_end, wt_rail, 1)
            industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_missing)
            gui.add_message_at(pl, "Rail: Way construction cost to height: cash: " + cash + " build cost: " + build_cost, world.get_time())

            build_check_month = world.get_time().ticks + (build_check_time(build_cost) * world.get_time().ticks_per_month)

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
          //err = construct_rail(pl, c_start, c_end, planned_way )
          if ( r1 == null ) {
            err = construct_rail(pl, c_end, c_start, planned_way )
            local c = c_start
            c_start = c_end
            c_end = c
          } else {
            err = construct_rail(pl, c_start, c_end, planned_way )
          }
          print("Way construction cost: " + (d-pl.get_current_cash()) )

          if (err) { // fail, c_start, c_end still arrays
            //print("Failed to build way from " + coord_to_string(c_start[0]) + " to " + coord_to_string(c_end[0]))
            gui.add_message_at(pl, "Failed to build way from " + coord_to_string(c_start[0]) + " to " + coord_to_string(c_end[0]), world.get_time())
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
          if ( line_start == t_end[0] ) {
            local c = c_start
            c_start = c_end
            c_end = c
            c_route.reverse()
          }
          phase ++
        }
      case 2: // build station and extensions
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

          // check combined station or connect factory src
          local tiles_y = abs(fs[0].y - c_start.y)
          local tiles_x = abs(fs[0].x - c_start.x)
          //
          local fsrc_building = fs[0].find_object(mo_building).get_desc()
          local size = fsrc_building.get_size(0)
          //local size_x = size.slice(0, size.find(","))
          //local size_y = size.slice(size.find(","))

          local tiles_c = 0//(fs.len() / 2) + settings.get_station_coverage() + 2

          if ( size.x == size.y ) {
            tiles_c = size.x + 2
          } else if ( size.x > size.y ) {
            tiles_c = size.x + 2
          } else if ( size.x < size.y ) {
            tiles_c = size.y + 2
          } else {
            tiles_c = (fs.len() / 2) + settings.get_station_coverage() + 2
          }

          //gui.add_message_at(pl, "fsrc tiles_c " + tiles_c, world.get_time())

          local combined_halt = false
          if (tiles_x > tiles_c || tiles_y > tiles_c) {
            //gui.add_message_at(pl, "tiles_x = " + tiles_x + " - tiles_y = " + tiles_y + " - tiles_c = " + tiles_c, world.get_time())
            combined_halt = true
          }

          //
          // check place and build station to c_start
          s_src = check_station(pl, c_start, c_route, count, wt_rail, planned_station, 1, combined_halt)
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

          // check combined station or connect factory dest
          tiles_y = abs(fd[0].y - c_end.y)
          tiles_x = abs(fd[0].x - c_end.x)
          //
          local fdest_building = fd[0].find_object(mo_building).get_desc()
          size = fdest_building.get_size(0)
          //size_x = size.slice(0, size.find(","))
          //size_y = size.slice(size.find(","))

          tiles_c = 0//(fs.len() / 2) + settings.get_station_coverage() + 2

          if ( size.x == size.y ) {
            tiles_c = size.x + 2
          } else if ( size.x > size.y ) {
            tiles_c = size.x + 2
          } else if ( size.x < size.y ) {
            tiles_c = size.y + 2
          } else {
            tiles_c = (fd.len() / 2) + settings.get_station_coverage() + 2
          }

          //gui.add_message_at(pl, "fdest tiles_c " + tiles_c, world.get_time())

          combined_halt = false
          if (tiles_x > tiles_c || tiles_y > tiles_c) {
            //gui.add_message_at(pl, "tiles_x = " + tiles_x + " - tiles_y = " + tiles_y + " - tiles_c = " + tiles_c, world.get_time())
            combined_halt = true
          }


          // check place and build station to c_end
          s_dest = check_station(pl, c_end, c_route, count, wt_rail, station_select, 1, combined_halt)
          if ( s_dest == false ) {
            print("Failed to build station at " + coord_to_string(c_end))
            if ( print_message_box > 0 ) {
              gui.add_message_at(pl, "Failed to build rail station at s_dest " + coord_to_string(c_end), world.get_time())
            }

            build_check_month = world.get_time().ticks + world.get_time().ticks_per_month

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

          phase += 3
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
            if ( !depot_found ) {

            } else {
              starts_field = c_end
            }
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
      case 8: // append vehicle_constructor and set line
        {
          local c = vehicle_constructor_t()
          c.p_depot  = depot_x(c_depot.x, c_depot.y, c_depot.z)
          c.p_line   = c_line
          c.p_convoy = planned_convoy
          c.p_count  = min(planned_convoy.nr_convoys, 1)
          append_child(c)

          local toc = get_ops_total();
          print("rail_connector wasted " + (toc-tic) + " ops")

          // rename line
          local line_name = c_line.get_name()
          local str_search = ") " + translate("Line")
          local st_names = c_line.get_schedule().entries
          if ( line_name.find(str_search) != null ) {
            local new_name = translate("Train") + " " + translate(freight) + " " + st_names[0].get_halt(pl).get_name() + " - " + st_names[1].get_halt(pl).get_name()
            c_line.set_name(new_name)
          }

          phase ++

          return r_t(RT_PARTIAL_SUCCESS)
        }
      case 9: // currently not used
        {

        }

    }
      //gui.add_message_at(pl, "finalize industry_manager.set_link_state ", world.get_time())

    if (finalize) {
      industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_built)
      if ( industry_manager.get_combined_link(fsrc, fdest, freight) > 0 ) {
        local a = industry_manager.get_combined_link(fsrc, fdest, freight) - 1
        industry_manager.set_combined_link(fsrc, fdest, freight, a)
      }
    }
    industry_manager.access_link(fsrc, fdest, freight).append_line(c_line)

    //gui.add_message_at(pl, "Build cost link " + industry_manager.get_link_build_cost(fsrc, fdest, freight, 1), world.get_time())

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
      print("Error during setup of rail connection.")
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

  function check_build_station(calc_route) {
    local routes = calc_route.routes
    local err = null

    // count tiles test for station
    local nr_tile_test = 3

    if ( routes.len() > 150 ) {
      //gui.add_message_at(our_player, "distance " + distance, world.get_time())
      //gui.add_message_at(our_player, "calc route " + coord3d_to_string(c_start[0]) +  " to " + coord3d_to_string(c_end[0]) + ": way tiles = " + calc_route.routes.len() + " bridge tiles = " + calc_route.bridge_lens + " tree tiles = " + calc_route.tiles_tree, world.get_time())
    }
    local s = routes.len()-nr_tile_test
    t_start = routes.slice(s)
    t_start.reverse()
    //t_start.append(tile_x(c_start[0].x, c_start[0].y, c_start[0].z))
    t_end = routes.slice(0, nr_tile_test)
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
    local check_way = []
    for ( local j = 0; j < (nr_tile_test*2); j++ ) {
      check_way.append(0)
    }
    for ( local i = 0; i < (nr_tile_test*2); i++ ) {
      if ( i < nr_tile_test ) {
        // start
        if ( tile_x(t_start[i].x, t_start[i].y, t_start[i].z).has_way(wt_rail) ) { //find_object(mo_way)
          check_way[i] = 1
        }
      } else {
        // end
        if ( tile_x(t_end[i-nr_tile_test].x, t_end[i-nr_tile_test].y, t_end[i-nr_tile_test].z).has_way(wt_rail) ) { //find_object(mo_way)
          check_way[i] = 1
        }
      }
    }

    for ( local j = 0; j < t_start.len()-1; j++ ) {
      err = command_x.build_way(pl, t_start[j], t_start[j+1], planned_way, true)
      if ( err != null ) {
        //gui.add_message_at(pl, "check_station build_way " + err, t_start[0])
      }
    }

            //err = command_x.build_way(pl, t_start[0], t_start[1], planned_way, true)
            //if ( err != null ) { gui.add_message_at(pl, "check_station build_way " + err, t_start[0]) }
            //err = command_x.build_way(pl, t_start[1], t_start[2], planned_way, true)
            //if ( err != null ) { gui.add_message_at(pl, "check_station build_way " + err, t_start[0]) }
            //::debug.pause()
    if ( err == null ) {
      err = check_station(pl, t_start[0], routes, st_lenght, wt_rail, planned_station, 0)
      if ( err == false ) {
        gui.add_message_at(pl, "check_station " + err, t_start[0])
      }
      if ( err == true ) {
        // station start ok
        for ( local j = 0; j < t_end.len()-1; j++ ) {
          err = command_x.build_way(pl, t_end[j], t_end[j+1], planned_way, true)
          if ( err != null ) {
            gui.add_message_at(pl, "check_station build_way " + err, t_end[0])
          }
        }
                //::debug.pause()
                //err = command_x.build_way(pl, t_end[0], t_end[1], planned_way, true)
                //if ( err != null ) { gui.add_message_at(pl, "check_station build_way t_end " + err, t_end[0]) }
                //err = command_x.build_way(pl, t_end[1], t_end[2], planned_way, true)
                //if ( err != null ) { gui.add_message_at(pl, "check_station build_way t_end " + err, t_end[0]) }
                //local tool = command_x(tool_remove_way)

        if ( err == null ) {
          err = check_station(pl, t_end[0], routes, st_lenght, wt_rail, planned_station, 0)
          if ( err == true ) {
            // station end ok
            // remove track -> error by build
                    /*for ( local i = 0; i < (nr_tile_test*2); i++ ) {
                      if ( i < nr_tile_test && check_way[i] == 0 ) {
                        remove_tile_to_empty(t_start[i], wt_rail, 0)
                      } else if ( i > (nr_tile_test-1) && check_way[i] == 0 ) {
                        remove_tile_to_empty(t_end[i-nr_tile_test], wt_rail, 0)
                      }
                    }*/
            remove_test_way(check_way, nr_tile_test, t_start, t_end, 1)
                    //tool.work(our_player, t_start[0], t_start[2], "" + wt_rail)
                    //tool.work(our_player, t_end[0], t_end[2], "" + wt_rail)
          } else {
            // failed station place end
                    /*for ( local i = 0; i < (nr_tile_test*2); i++ ) {
                      if ( check_way[i] == 0 && i < nr_tile_test ) {
                        remove_tile_to_empty(t_start[i], wt_rail, 0)
                        //gui.add_message_at(pl, "remove " + coord3d_to_string(t_start[i]), t_start[i])
                      } else if ( check_way[i] == 0 && i > (nr_tile_test-1) ) {
                        remove_tile_to_empty(t_end[i-nr_tile_test], wt_rail, 0)
                        //gui.add_message_at(pl, "remove " + coord3d_to_string(t_end[i-nr_tile_test]), t_end[i-nr_tile_test])
                      }
                    }*/
            remove_test_way(check_way, nr_tile_test, t_start, t_end, 1)

            return error_handler()
          }
        } else {
          // remove start and end
                    /*for ( local i = 0; i < (nr_tile_test*2); i++ ) {
                      if ( check_way[i] == 0 && i < nr_tile_test ) {
                        remove_tile_to_empty(t_start[i], wt_rail, 0)
                      } else if ( check_way[i] == 0 && i > (nr_tile_test-1) ) {
                        remove_tile_to_empty(t_end[i-nr_tile_test], wt_rail, 0)
                      }
                    }*/
          remove_test_way(check_way, nr_tile_test, t_start, t_end, 1)
        }
      } else {
        // failed station place start
        // remove start
                //::debug.pause()
                /*for ( local i = 0; i < nr_tile_test; i++ ) {
                  if ( check_way[i] == 0 && i < nr_tile_test ) {
                    remove_tile_to_empty(t_start[i], wt_rail, 0)
                  }
                }*/
        remove_test_way(check_way, nr_tile_test, t_start, t_end, 0)
        return error_handler()
      }
    } else {
      // no built way start -> remove start
              /*for ( local i = 0; i < nr_tile_test; i++ ) {
                if ( check_way[i] == 0 && i < nr_tile_test ) {
                  remove_tile_to_empty(t_start[i], wt_rail, 0)
                }
              }*/
      remove_test_way(check_way, nr_tile_test, t_start, t_end, 0)
    }

    if ( print_message_box > 0 ) {
      gui.add_message_at(pl, "plan station start " + t_start[0] + " - plan station end " + t_end[0], t_start[2])
    }

    return true
  }

  function remove_test_way(check_way, nr_tile_test, t_start, t_end, s) {
    for ( local i = 0; i < (nr_tile_test*2); i++ ) {
      if ( check_way[i] == 0 && i < nr_tile_test ) {
        remove_tile_to_empty(t_start[i], wt_rail, 0)
      } else if ( s == 1 && check_way[i] == 0 && i > (nr_tile_test-1) ) {
        remove_tile_to_empty(t_end[i-nr_tile_test], wt_rail, 0)
      }
    }
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
        if (debug) gui.add_message_at(our_player, "command_x.build_way rail  search_route :" + err, world.get_time())
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
