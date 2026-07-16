/**
 * @brief air_connector.nut create air lines
 *
 */

class air_connector_t extends manager_t
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

  start_airport = null // tiles array
  end_airport   = null // tiles array


  // print messages box
  // 1 = way
  // 2 = station
  // 3 = depot
  // 4 = info
  print_message_box = 1

  constructor()
  {
    base.constructor("air_connector_t")
    debug = false
  }

  function work()
  {
    planned_way = openair
    // TODO check if child does the right thing
    local pl = our_player
    local tic = get_ops_total();

    c_generate_start = c_start == null
    c_generate_end   = c_end   == null

    local fs = fsrc.get_tile_list()
    local fd = fdest.get_tile_list()

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
          return r_t(RT_TOTAL_FAIL)
        }
      }
      local build_cash = player_x(our_player.nr).get_current_cash() + (((player_x(our_player.nr).get_current_net_wealth()/100) - player_x(our_player.nr).get_current_cash())/2)
      local build_cost = industry_manager.get_link_build_cost(fsrc, fdest, freight, 4)
      if ( industry_manager.get_link_build_cost(fsrc, fdest, freight, 0) > 0 ) {
        build_cost = industry_manager.get_link_build_cost(fsrc, fdest, freight, 0)
      }
      if ( (build_check_month > world.get_time().ticks || build_cost > build_cash) && industry_manager.get_combined_link(fsrc, fdest, freight) == 0 ) {
        // not build link
        if ( debug ) gui.add_message_at(our_player, "#air_conn# not build line : build_check_month = " + build_check_month + " or build cost link > cash : build cost line " + industry_manager.get_link_build_cost(fsrc, fdest, freight, 4) + " | build cost link " + industry_manager.get_link_build_cost(fsrc, fdest, freight, 0), world.get_time())
        if ( debug ) gui.add_message_at(our_player, " ---> link " + fsrc + "  " + fsrc.get_name() + " - " + fdest.get_name(), world.get_time())

        industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_missing)

        return r_t(RT_TOTAL_FAIL)
      }
    }


    switch(phase) {
      case 0:  // station places
        if ( print_message_box > 0 && print_message_box != 4 ) {
          gui.add_message_at(our_player, "______________________ build airports ______________________", world.get_time())
          gui.add_message_at(pl, " line from " + fsrc.get_name() + " (" + coord_to_string(fs[0]) + ") to " + fdest.get_name() + " (" + coord_to_string(fd[0]) + ")", world.get_time())
        }
        // count trials, and fail if necessary
        if (c_trial_route > 3) {
          print("Route building failed " + c_trial_route + " times.")
          if (debug) gui.add_message_at(pl, "Failed to complete route from  " + coord_to_string(fsrc) + " to " + coord_to_string(fdest) + " after " + c_trial_route + " attempts", fsrc)
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

        for ( local i = 0; i < c_start.len(); i++ ) {
          start_airport = check_ground(c_start[i])
          if ( start_airport != false ) {
            //gui.add_message_at(our_player, "start_airport " + start_airport, world.get_time())
            break
          }
        }

        for ( local i = 0; i < c_end.len(); i++ ) {
          end_airport = check_ground(c_end[i])
          if ( end_airport != false ) {
            //gui.add_message_at(our_player, "end_airport " + end_airport, world.get_time())
            break
          }
        }

          if ( print_message_box > 0 && print_message_box != 4 ) {
            gui.add_message_at(our_player, "start_airport " + start_airport, world.get_time())
            gui.add_message_at(our_player, "end_airport " + end_airport, world.get_time())

          }

        if ( start_airport != false  &&  end_airport != false && start_airport != null && end_airport != null ) {
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

          // build airport ways
          local taxiway = find_object("way", wt_air, 100, st_flat)
          local runway  = find_object("way", wt_air, 100, st_runway)
          // build airport c_start
          local err = command_x.build_way(our_player, start_airport[0], start_airport[5], taxiway, true)
          err = command_x.build_way(our_player, start_airport[1], start_airport[3], taxiway, true)
          err = command_x.build_way(our_player, start_airport[4], start_airport[6], runway, true)
          if ( print_message_box > 0 && print_message_box != 4 ) {
            gui.add_message_at(our_player, "build f_src", start_airport[0])
            gui.add_message_at(our_player, "err " + err, world.get_time())
          }

          // build airport c_end
          err = command_x.build_way(our_player, end_airport[0], end_airport[5], taxiway, true)
          err = command_x.build_way(our_player, end_airport[2], end_airport[3], taxiway, true)
          err = command_x.build_way(our_player, end_airport[4], end_airport[6], runway, true)
          if ( print_message_box > 0 && print_message_box != 4 ) {
            gui.add_message_at(our_player, "build airport f_desc ", end_airport[0])
            gui.add_message_at(our_player, "err " + err, world.get_time())
          }


          phase ++
        }
      case 2: // build station and extensions
        {
          local err = null
          local obj_building = tile_x(start_airport[0].x, start_airport[0].y, start_airport[0].z).find_object(mo_building)
          if ( obj_building != null && obj_building.get_owner() != our_player ) {
            if (debug) gui.add_message_at(pl, " --- tile to build station not free", c_start)

            build_check_month = world.get_time().ticks + world.get_time().ticks_per_month

            return restart_with_phase0()
            //return error_handler()
          } else if ( c_generate_start || obj_building == null) {
            err = build_air_station(start_airport[0], planned_station)
            err = build_air_station(start_airport[3], planned_station)
            //err = command_x.build_station(pl, c_start, planned_station )
          }
          if (err) {
            if (debug) gui.add_message_at(pl, "Failed to build air station at  " + coord_to_string(start_airport[0]) + " [" + err + "]", start_airport[0])// try again
            if ( print_message_box == 2 ) { gui.add_message_at(pl, "Failed to build air station at  " + coord_to_string(start_airport[0]) + " error " + err, world.get_time()) }
            return restart_with_phase0()
          }
          else {
            c_generate_start = false // station build, do not search for another place
          }
          err = build_air_station(end_airport[0], planned_station)
          err = build_air_station(end_airport[3], planned_station)
          //err = command_x.build_station(pl, c_end, planned_station )
          if (err) {
            if (debug) gui.add_message_at(pl, "Failed to build air station at  " + coord_to_string(end_airport[0]) + " [" + err + "]", end_airport[0])
            // try again
            return restart_with_phase0()
          }
          else {
            c_generate_end = false // station build, do not search for another place
          }

          if (finalize) {
            // store place of unload station for future use
            local fs = ::station_manager.access_freight_station(fdest)
            if (fs.air_unload == null) {
              fs.air_unload = end_airport[0]
            }
          }

          // check accept goods
          local accept_freight = planned_station.enables_freight()
            gui.add_message_at(our_player, "air - planned_station.enables_freight() " + accept_freight, world.get_time())
          if ( !accept_freight ) {
            local extension = find_extension(wt_air)
            if ( extension != null ) {
              if ( !build_exrension(start_airport[0]) ) {
                // not build extension
              }

              if ( !build_exrension(end_airport[0]) ) {
                err = command_x.build_station(our_player, end_airport[1], extension)
              }

            }
          }

          if ( print_message_box == 2 ) { gui.add_message_at(our_player, "Build station on " + coord_to_string(start_airport[0]) + " and " + coord_to_string(end_airport[0]), world.get_time()) }

          phase += 3
        }
      case 5: // build depot
        {


          if ( print_message_box == 3 ) {
            gui.add_message_at(our_player, "___________ exists depots air ___________", world.get_time())
            gui.add_message_at(our_player," c_start pos: " + coord_to_string(start_airport[0]) + " : c_end pos: " + coord_to_string(end_airport[0]), world.get_time())
          }

          c_depot = start_airport[1]

             // depot already existing ?
            if (c_depot.find_object(mo_depot_air) == null) {
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

          phase ++
        }
      case 6: // create schedule
        {
          local sched = schedule_x(wt_air, [])
          sched.entries.append( schedule_entry_x(start_airport[0], 100, 0) );
          sched.entries.append( schedule_entry_x(end_airport[0], 0, 0) );
          c_sched = sched
          phase ++
        }

      case 7: // create line and set schedule
        {
          pl.create_line(wt_air)

          // find the line - it is a line without schedule and convoys
          local list = pl.get_line_list()
          foreach(line in list) {
            if (line.get_waytype() == wt_air  &&  line.get_schedule().entries.len()==0) {
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
          c.p_count  = min(planned_convoy.nr_convoys, 2)

          append_child(c)

          local toc = get_ops_total();
          print("air_connector wasted " + (toc-tic) + " ops")
          c_generate_start = start_airport[0] == null
          c_generate_end   = end_airport[0]   == null

          // rename line
          local line_name = c_line.get_name()
          local str_search = ") " + translate("Line")
          local st_names = c_line.get_schedule().entries
          if ( line_name.find(str_search) != null ) {
            local new_name = translate("airplane") + " " + translate(freight) + " " + st_names[0].get_halt(pl).get_name() + " - " + st_names[1].get_halt(pl).get_name()
            c_line.set_name(new_name)
          }
          sleep()

          phase ++

        }
      case 9: // currently not used
        {
          //gui.add_message_at(pl, "c_line.get_convoy_list().get_count() " + c_line.get_convoy_list().get_count(), world.get_time())
          /*if ( c_line.get_convoy_list().get_count() == 2 ) {
            local err = build_air_station(start_airport[3], planned_station)
            err = build_air_station(end_airport[3], planned_station)
          }*/

          phase ++

          return r_t(RT_PARTIAL_SUCCESS)
        }
    }

    if (finalize) {
      industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_built)
      if ( industry_manager.get_combined_link(fsrc, fdest, freight) > 0 ) {
        local a = industry_manager.get_combined_link(fsrc, fdest, freight) - 1
        industry_manager.set_combined_link(fsrc, fdest, freight, a)
      }
    }
    industry_manager.access_link(fsrc, fdest, freight).append_line(c_line)

    //gui.add_message_at(pl, "Build cost link " + industry_manager.get_link_build_cost(fsrc, fdest, freight, 2), world.get_time())

    local cs = start_airport[0]
    local ce = end_airport[0]
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
    local msgtext = format(translate("%s build air line from %s (%s) to %s (%s)"), pl.get_name(), f_name[0], coord_to_string(cs), f_name[1], coord_to_string(ce))
    //gui.add_message_at(pl, pl.get_name() + " build road line from " + f_name[0] + " (" + coord_to_string(cs) + ") to " + f_name[1] + " (" + coord_to_string(ce) + ")", c_start)
    gui.add_message_at(pl, msgtext, start_airport[0])

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
      print("Error during setup of road connection.")
      industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_failed);
    }
    return r
  }

  function build_air_station(tile, station) {
    local err = null

    local d = tile_x(tile.x, tile.y, tile.z).get_way_dirs(wt_air)
    local rotation = 0
    switch (d) {
      case 2:
        rotation = 1
        break
      case 1:
        rotation = 2
        break
      case 8:
        rotation = 3
        break
      case 10:
        rotation = 1
        break
      default:

    }


    err = command_x.build_station(our_player, tile, station, rotation )

    return err
  }

  function check_ground(tile) {
    //local tile_list_rotate = [] // 0 = n; 1 = w; 2 = s; 3 = e
    local tiles = []

    // n
    local t = tile_x(tile.x-1, tile.y-1, tile.z)
    tiles.append(tile)
    for ( local i = 0; i < 2; i++ ) {
      for ( local j = 0; j < 3; j++ ) {
        tiles.append(tile_x(t.x+j, t.y-i, t.z))
      }
    }
    //gui.add_message_at(our_player, "tiles " + tiles.len(), world.get_time())

    local check_tiles = check_area(tiles)
    if ( check_tiles != false ) {
      return check_tiles
    }

    tiles.clear()
    // w
    t = tile_x(tile.x-1, tile.y-1, tile.z)
    tiles.append(tile)
    for ( local i = 0; i < 2; i++ ) {
      for ( local j = 0; j < 3; j++ ) {
        tiles.append(tile_x(t.x-i, t.y+j, t.z))
      }
    }
    check_tiles = check_area(tiles)
    if ( check_tiles != false ) {
      return check_tiles
    }

    tiles.clear()
    // s
    t = tile_x(tile.x-1, tile.y+1, tile.z)
    tiles.append(tile)
    for ( local i = 0; i < 2; i++ ) {
      for ( local j = 0; j < 3; j++ ) {
        tiles.append(tile_x(t.x+j, t.y+i, t.z))
      }
    }
    local check_tiles = check_area(tiles)
    if ( check_tiles != false ) {
      return check_tiles
    }

    tiles.clear()
    // e
    t = tile_x(tile.x+1, tile.y-1, tile.z)
    tiles.append(tile)
    for ( local i = 0; i < 2; i++ ) {
      for ( local j = 0; j < 3; j++ ) {
        tiles.append(tile_x(t.x+i, t.y+j, t.z))
      }
    }
    local check_tiles = check_area(tiles)
    if ( check_tiles != false ) {
      return check_tiles
    }

    tiles.clear()

    return false
  }

  function check_area(tile_list) {

      local tiles_free = 0
      for ( local i = 1; i < tile_list.len(); i++ ) {
        local ground = tile_list[i].is_ground()
        local slope = tile_list[i].get_slope()
        local free = test_tile_is_empty(tile_list[i])
        //gui.add_message_at(our_player, coord3d_to_string(tile_list[i]) + " - ground " + ground + " - slope " + slope + " - free " + free, tile_list[i])
        if ( ground && free && slope == 0 ) {
          tiles_free++
          //gui.add_message_at(our_player, i + " tiles_free " + tiles_free, world.get_time())
        }
      }
      if ( tiles_free == 6 ) {
        return tile_list
      }

    return false
  }

  function build_exrension(tile) {
    local d = tile.get_way_dirs(wt_air)
              local tiles = []
              switch(d) {
                case 1:
                  // n
                  tiles.append(square_x(start_airport[0].x-1, tile.y).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x+1, tile.y).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x-1, tile.y+1).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x, tile.y+1).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x+1, tile.y+1).get_ground_tile())

                  break
                case 2:
                // e
                  tiles.append(square_x(start_airport[0].x, tile.y+1).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x, tile.y-1).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x-1, tile.y-1).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x-1, tile.y).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x-1, tile.y+1).get_ground_tile())

                  break
                case 4:
                  // s
                  tiles.append(square_x(start_airport[0].x-1, tile.y).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x+1, tile.y).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x-1, tile.y-1).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x, tile.y-1).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x+1, tile.y-1).get_ground_tile())

                  break
                case 8:
                  // w
                  tiles.append(square_x(start_airport[0].x, tile.y+1).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x, tile.y-1).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x+1, tile.y+1).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x+1, tile.y).get_ground_tile())
                  tiles.append(square_x(start_airport[0].x+1, tile.y-1).get_ground_tile())

                  break
              }

              for ( local i = 0; i < tiles.len(); i++ ) {
                if ( test_tile_is_empty(tiles[i]) ) {
                  local err = command_x.build_station(our_player, tiles[i], extension)
                  return true
                }
              }
    return false
  }
}
