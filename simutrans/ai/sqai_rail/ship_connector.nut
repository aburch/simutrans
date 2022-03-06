class ship_connector_t extends manager_t
{
  // input data
  fsrc = null
  fdest = null
  freight = null
  planned_station = null // harbour on slope
  planned_harbour_flat = null // dock flat ground to water
  planned_convoy = null
  planned_way = null // unused
  planned_depot = null
  finalize = true

  // step-by-step construct the connection
  phase = 0
  // can be provided optionally
  c_start = null // array, must be water tiles
  c_end   = null // array, must be water tiles
  c_route = null // array
  c_harbour_tiles = null
  // generated data
  c_depot = null
  c_sched = null
  c_line  = null
  c_cnv   = null

  // print messages box
  // 1 = way
  // 2 = stations
  // 3 = depot
  // 5 = errors
  print_message_box = 0

  constructor()
  {
    base.constructor("ship_connector_t")
    c_harbour_tiles = {}
    debug = true
  }

  function work()
  {
    planned_way = openwater
    // TODO check if child does the right thing
    local pl = our_player
    local tic = get_ops_total();

    local fs = fsrc.get_tile_list()
    local fd = fdest.get_tile_list()

    if ( check_factory_links(fsrc, fdest, freight) >= 2 && phase == 0 ) {
      //gui.add_message_at(pl, "no build line from " + fsrc.get_name() + " (" + coord_to_string(fs[0]) + ") to " + fdest.get_name() + " (" + coord_to_string(fd[0]) + ") to many links", world.get_time())
      return r_t(RT_TOTAL_FAIL)
    }

    switch(phase) {

      case 0: {
        if ( print_message_box > 0 ) {
          gui.add_message_at(pl, "______________________ build ship ______________________", world.get_time())
          gui.add_message_at(pl, " line from " + fsrc.get_name() + " (" + coord_to_string(fs[0]) + ") to " + fdest.get_name() + " (" + coord_to_string(fd[0]) + ")", world.get_time())
        }
        // find flat harbour building
        if (planned_harbour_flat == null) {
          local station_list = building_desc_x.get_available_stations(building_desc_x.flat_harbour, wt_water, good_desc_x(freight))
          planned_harbour_flat = industry_connection_planner_t.select_station(station_list, 1, planned_convoy.capacity)
        }
        if ( print_message_box == 2 ) {
          local flat = "----"
          if ( planned_harbour_flat ) { flat = planned_harbour_flat.get_name() }
          gui.add_message_at(pl," ... build station: " + planned_station.get_name() + " / " + flat, world.get_time())
        }

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
          if ( print_message_box == 1 ) { gui.add_message_at(pl, "Water way from " + coord_to_string(c_start[0]) + " to " + coord_to_string(c_end[0]), world.get_time()) }

          phase ++
        }
        else {
          if ( print_message_box == 2 ) {
            gui.add_message_at(pl, " ... ERROR No station places found ", world.get_time())
            gui.add_message_at(pl, "---------------------- ship end -----------------------", world.get_time())
          }
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
            //if ( tile_x(c_start.x, c_start.y, c_start.z).is_empty() ) {

            key = coord3d_to_key(c_start[0])

            if (key in c_harbour_tiles) {
              if ( c_harbour_tiles[key].get_halt() && c_harbour_tiles[key].get_halt().get_owner().nr == our_player_nr && c_harbour_tiles[key].find_object(mo_building) && c_harbour_tiles[key].find_object(mo_building).get_desc().get_type()==building_desc_x.station ) {
                // station from player exists - combined station
                if ( print_message_box == 2 ) {
                  gui.add_message_at(our_player, "Cannot place any harbour at " + coord_to_string(c_harbour_tiles[key]) + " station exists :: search new place", c_harbour_tiles[key])
                }
                // to do search new field
                c_start.clear()
                c_start = find_new_place(c_harbour_tiles[key].get_halt(),  planned_station, planned_harbour_flat, c_harbour_tiles)
                key = coord3d_to_key(c_start[0])

                if ( c_harbour_tiles[key].get_halt() && c_harbour_tiles[key].get_halt().get_owner().nr == our_player_nr && c_harbour_tiles[key].find_object(mo_building) && c_harbour_tiles[key].find_object(mo_building).get_desc().get_type()==building_desc_x.station ) {
                  gui.add_message_at(our_player, "Cannot place any harbour at " + coord_to_string(c_harbour_tiles[key]), c_harbour_tiles[key])
                  return r_t(RT_TOTAL_FAIL)
                } else {
                  err = build_harbour(c_harbour_tiles[key], c_start)
                }
              } else {
                err = build_harbour(c_harbour_tiles[key], c_start)
                if (err) {
                  if ( c_start.len() > 1 ) {
                    c_start.clear()
                    c_start = find_anchorage(fsrc,  planned_station, planned_harbour_flat, c_harbour_tiles)
                    key = coord3d_to_key(c_start[0])
                    if ( c_start.len() > 0 ) {
                      err = null
                      err = build_harbour(c_harbour_tiles[key], c_start)
                    }
                  }
                }
              }
            }

            if (err) {
              print("Failed to build harbour at " + key + " / " + err)
              if ( print_message_box == 2 ) {
                gui.add_message_at(pl, " --- Failed to build harbour at " + key + " / " + err, world.get_time())
                gui.add_message_at(pl, " --- c_start " + coord_to_string(c_start[0]), world.get_time())
              }
              return r_t(RT_TOTAL_FAIL)
            }
          }
          // check station connection to factory or combined station


          if (err == null) {

            //remove_field(c_end[0])

            key = coord3d_to_key(c_end[0])
            if (key in c_harbour_tiles) {
              if ( c_harbour_tiles[key].find_object(mo_building) != null ) {
                gui.add_message_at(pl, " --- tile to build harbour not free", world.get_time())
                return r_t(RT_TOTAL_FAIL)
              }
              err = build_harbour(c_harbour_tiles[key], c_end)
            }
          }
          if (err) {
            print("Failed to build harbour at " + key + " / " + err)
            if ( print_message_box == 5 ) {
              gui.add_message_at(pl, " --- Failed to build harbour at " + key + " / " + err, world.get_time())
              gui.add_message_at(pl, " --- c_end " + coord_to_string(c_end[0]), world.get_time())
            }
            return error_handler()
          }

          c_harbour_tiles = null
          if ( print_message_box == 2 ) { gui.add_message_at(pl, "Build harbour on " + coord_to_string(c_start[0]) + " and " + coord_to_string(c_end[0]), world.get_time()) }
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
          // search depot to range start and end station
          local depot_found = search_depot(c_start[0], wt_water)
          local starts_field = c_start[0]
          if ( !depot_found ) {
            depot_found = search_depot(c_end[0], wt_water)
            starts_field = c_end[0]
          }

          if ( !depot_found && print_message_box == 3 ) {
            gui.add_message_at(pl," *** depot not found *** ", world.get_time())
          } else if ( print_message_box == 3 ) {
            gui.add_message_at(pl," ---> depot found : " + depot_found.get_pos(), coord_to_string(depot_found))
          }

          // build ship to depot
          if ( depot_found ) {
            c_depot = depot_found
            //local err = command_x.build_road(pl, starts_field, c_depot, planned_way, false, true)
          } else {
            // depot already existing ?

            local depot_tiles = []
            local tile_range = 4
            depot_tiles.append(tile_x(c_start[0].x-tile_range, c_start[0].y-tile_range, c_start[0].z))
            depot_tiles.append(tile_x(c_start[0].x+tile_range, c_start[0].y-tile_range, c_start[0].z))
            depot_tiles.append(tile_x(c_start[0].x-tile_range, c_start[0].y+tile_range, c_start[0].z))
            depot_tiles.append(tile_x(c_start[0].x+tile_range, c_start[0].y+tile_range, c_start[0].z))
            for ( local i = 0; i < depot_tiles.len(); i++ ) {
              //gui.add_message_at(pl, "depot_tiles[i].is_water() " + coord_to_string(depot_tiles[i]) + " " + depot_tiles[i].is_water(), depot_tiles[i])
              //gui.add_message_at(pl, "depot_tiles[i].is_empty() " + coord_to_string(depot_tiles[i]) + " " + depot_tiles[i].is_empty(), depot_tiles[i])
              local tile_halt = ::halt_x.get_halt(depot_tiles[i], player_x(1))
              if ( tile_halt != null ) {
                //gui.add_message_at(pl, "tile_halt.get_halt() " + coord_to_string(depot_tiles[i]) + " " + tile_halt.get_halt(player_x(1)), depot_tiles[i])
              }

              if ( depot_tiles[i].is_water() && tile_halt == null && depot_tiles[i].get_objects().get_count()==0 ) { //
                c_depot = depot_tiles[i]
                //gui.add_message_at(pl, "depot tile " + coord_to_string(c_depot), c_depot)
                break
              }
            }

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
            if ( print_message_box == 3 ) { gui.add_message_at(pl, "Build depot on " + coord_to_string(c_depot), world.get_time()) }
          }
          phase ++
        }
      case 6: // create schedule
        {
          local sched = schedule_x(wt_water, [])
          sched.entries.append( schedule_entry_x(c_start[0], 100, 0) );
          sched.entries.append( schedule_entry_x(c_end[0], 0, 0) );

          if ( sched.entries[0].get_halt(pl).get_name() == sched.entries[1].get_halt(pl).get_name() ) {
            // set new start

            //gui.add_message_at(pl," ... start name (" + sched.entries[0].get_halt(pl).get_name() + ") == end name (" + sched.entries[1].get_halt(pl).get_name() + ")", world.get_time())
            //gui.add_message_at(pl," ... c_start len = " + c_start.len(), world.get_time())

            c_start.clear()
            c_start = find_anchorage(fsrc,  planned_station, planned_harbour_flat, c_harbour_tiles)

            sched.entries[0] = schedule_entry_x(c_start[0], 100, 0);

            //gui.add_message_at(pl," ... new start name (" + sched.entries[0].get_halt(pl).get_name() + ")", world.get_time())
            //::debug.pause()
          }

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
          c.p_count  = min(planned_convoy.nr_convoys, 1) // 1 ship to begin
          append_child(c)

          local toc = get_ops_total();
          print("ship_connector wasted " + (toc-tic) + " ops")

          phase ++

          return r_t(RT_PARTIAL_SUCCESS)
        }
      case 9: // build station extension
        {
          // optimize way line save in c_route
          //if ( tile_x(c_start.x, c_start.y, c_start.z).find_object(mo_building) != null && tile_x(c_end.x, c_end.y, c_end.z).find_object(mo_building) != null && c_route.len() > 0 ) {

            // rename line
            local line_name = c_line.get_name()
            local str_search = ") " + translate("Line")
            local st_names = c_line.get_schedule().entries
            if ( line_name.find(str_search) != null ) {
              local new_name = translate("Ship") + " " + translate(freight) + " " + st_names[0].get_halt(pl).get_name() + " - " + st_names[1].get_halt(pl).get_name()
              c_line.set_name(new_name)
            }
          //}
        }
    }

    if (finalize) {
      industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_built)
    }
    industry_manager.access_link(fsrc, fdest, freight).append_line(c_line)
    /*
    local cs = []
    local ce = []
    if (c_start.len()>0  &&  c_end.len()>0) {
      cs.append(tile_x(c_start[0].x, c_start[0].y, c_start[0].z))
      ce.append(tile_x(c_end[0].x, c_end[0].y, c_end[0].z))
    } else {
      cs.append(c_start[0])
      ce.append(c_end[0])
    }

    gui.add_message_at(pl, pl.get_name() + " build ship line from " + f_name[0] + " (" + coord_to_string(square_x(cs[0].x, cs[0].y)) + ") to " + f_name[1] + " (" + coord_to_string(square_x(ce[0].x, ce[0].y)) + ")", c_start)
    */

    if (c_start.len()>0  &&  c_end.len()>0) {
      local st = halt_x.get_halt(c_start[0], pl)
      local f_name = ["", ""]
      if ( st ) {
        local fl_st = st.get_factory_list()
        if ( fl_st.len() > 0 ) {
          f_name[0] = fl_st[0].get_name()
        } else {
          f_name[0] = st.get_name()
        }
      }
      st = halt_x.get_halt(c_end[0], pl)
      if ( st ) {
        local fl_st = st.get_factory_list()
        if ( fl_st.len() > 0 ) {
          f_name[1] = fl_st[0].get_name()
        } else {
          f_name[1] = st.get_name()
        }
      }
      local msgtext = format(translate("%s build ship line from %s (%s) to %s (%s)"), pl.get_name(), f_name[0], coord_to_string(c_start[0]), f_name[1], coord_to_string(c_end[0]))
      //gui.add_message_at(pl, pl.get_name() + " build ship line from " + f_name[0] + " (" + coord_to_string(c_start[0]) + ") to " + f_name[1] + " (" + coord_to_string(c_end[0]) + ")", c_start[0])
      gui.add_message_at(pl, msgtext, c_start[0])
    } else {
      local st = halt_x.get_halt(c_start, pl)
      local f_name = ["", ""]
      if ( st ) {
        local fl_st = st.get_factory_list()
        if ( fl_st.len() > 0 ) {
          f_name[0] = fl_st[0].get_name()
        } else {
          f_name[0] = st.get_name()
        }
      }
      st = halt_x.get_halt(c_end, pl)
      if ( st ) {
        local fl_st = st.get_factory_list()
        if ( fl_st.len() > 0 ) {
          f_name[1] = fl_st[0].get_name()
        } else {
          f_name[1] = st.get_name()
        }
      }
      local msgtext = format(translate("%s build ship line from %s (%s) to %s (%s)"), pl.get_name(), f_name[0], coord_to_string(c_start), f_name[1], coord_to_string(c_end))
      //gui.add_message_at(pl, pl.get_name() + " build ship line from " + f_name[0] + " (" + coord_to_string(c_start) + ") to " + f_name[1] + " (" + coord_to_string(c_end) + ")", c_start)
      gui.add_message_at(pl, msgtext, c_start)
    }

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

        local h = halt_x.get_halt(tile, our_player)
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

          if ( print_message_box == 2 && to.is_empty() ) {
            gui.add_message_at(our_player, " ... place finder: to.is_empty() " + coord3d_to_string(to) + " = "  + to.is_empty(), world.get_time())
          }

          if (to  &&  get_harbour_type_for_tile(to, planned_station, planned_harbour_flat, d) > 0) {

            anch.append(tile)
            c_harbour_tiles[coord3d_to_key(tile)] <- to
            break
          }

          /*if (to  &&  get_harbour_type_for_tile(to, planned_station, planned_harbour_flat, d) > 0) {

            anch.append(tile)
            c_harbour_tiles[coord3d_to_key(tile)] <- to
            break
          }*/
        }
      }
    }
    return anch
  }

  static function find_new_place(halt, planned_station, planned_harbour_flat, c_harbour_tiles)
  {
    local tile_list = ::finder.find_water_places( ::finder.get_tiles_near_halt(halt) )
    local anch = []
    // find places to build harbour
    if (tile_list.len()>0) {
      foreach(tile in tile_list) {
        for(local d = 1; d<16; d*=2) {
          local to = tile.get_neighbour(wt_all, d)

          if ( print_message_box == 2 && to.is_empty() ) {
            gui.add_message_at(our_player, " ... place finder: to.is_empty() " + coord3d_to_string(to) + " = "  + to.is_empty(), world.get_time())
          }

          if (to  &&  get_harbour_type_for_tile(to, planned_station, planned_harbour_flat, d) > 0) {

            anch.append(tile)
            c_harbour_tiles[coord3d_to_key(tile)] <- to
            break
          }

          /*if (to  &&  get_harbour_type_for_tile(to, planned_station, planned_harbour_flat, d) > 0) {

            anch.append(tile)
            c_harbour_tiles[coord3d_to_key(tile)] <- to
            break
          }*/
        }
      }
    }
    return anch
  }

  /**
   * Checks whether we can build harbour: empty tile, slopes, enough space on water
   * returns type of harbour that can be placed here: 0 - nothing, 1 - harbour, 2 - flat harbour
   */
  static function get_harbour_type_for_tile(tile, planned_harbour, planned_harbour_flat, d_water_to_land /*direction*/)
  {
    local type = 0 // 1 - harbour, 2 - flat harbour
    if (!tile.is_empty()) {
      return 0
    }
    // first slope
    if (planned_harbour) {
      if (tile.get_slope() != 0) {
        // can place on slopes, target tile is sloped and empty -> we can terraform
        type = 1
      }
      else if (planned_harbour_flat == null  &&  command_x.can_set_slope(our_player, tile, dir.to_slope(d_water_to_land))==null) {
        // no slope, no flat dock, but terraforming possible
        type = 1
      }
    }
    if (planned_harbour_flat) {
      if (tile.get_slope() == 0) {
        // flat place
        type = 2
      }
      else if (planned_harbour == null  &&  command_x.can_set_slope(our_player, tile, 0)==null) {
        // not flat, not harbour for slopes, can flatten
        type = 2
      }
    }
    if (type == 0) {
      return 0
    }
    // then space
    local size = type == 1  ?  planned_harbour.get_size(0) : planned_harbour_flat.get_size(0)

    return finder.check_harbour_place(tile, size.x*size.y, dir.backward(d_water_to_land)) ? type : 0
  }

  /**
   * Checks whether there is already a harbour on this tile.
   * @returns corresponding halt_x object (or null)
   */
  static function get_harbour_halt(tile)
  {
    local halt = tile.get_halt()
    if (halt  &&  halt.get_owner().nr == our_player_nr) {
      // our halt
      local harb = tile.find_object(mo_building)
      if (harb  &&  (harb.get_desc().get_type()==building_desc_x.harbour  ||  harb.get_desc().get_type()==building_desc_x.flat_harbour) ) {
        return halt
      }
    }
    return null
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

    if ( print_message_box == 2 ) {
      gui.add_message_at(our_player, " --- Place harbour at " + coord3d_to_string(tile) + " to access " + coord3d_to_string(water), world.get_time())
    }
    print("Place harbour at " + coord3d_to_string(tile) + " to access " + coord3d_to_string(water) )

    if ( tile.get_halt() && tile.get_halt().get_owner().nr == our_player_nr && tile.find_object(mo_building) && tile.find_object(mo_building).get_desc().get_type()==building_desc_x.station ) {
      //
      gui.add_message_at(our_player, "Cannot place any harbour at " + coord_to_string(tile) + " station exists", tile)
      // to do search new field

    }

    if (get_harbour_halt(tile)) {
      // already there
    }
    else {
      local d_water_to_land = coord_to_dir(dif)
      local type = get_harbour_type_for_tile(tile, planned_station, planned_harbour_flat, d_water_to_land)

      switch(type) {
        case 0: {
          err = "Cannot place harbour here"
          gui.add_message_at(our_player, "Cannot place any harbour at " + coord_to_string(tile), tile)
          // no build dock on station tile destroy exists way line
          // to do
          break
        }
        case 1: {
          // harbour on slope
          local slope = dir.to_slope(d_water_to_land)
          // terraform ??
          if (tile.get_slope() != slope  &&  tile.get_slope() != 2*slope) {
            err = command_x.set_slope(our_player, tile, slope )
            if (err) gui.add_message_at(our_player, "Failed to change slope at " + coord_to_string(tile) +"\n" + err, tile)
          }
          if (err == null) {
            err = command_x.build_station(our_player, tile, planned_station)
            if (err) gui.add_message_at(our_player, "Failed to harbour at " + coord_to_string(tile) +"\n" + err, tile)
          }
          local size = planned_station.get_size(0)
          len = size.x*size.y
          break
        }
        case 2: {
          // flat dock
          // flatten slope
          if (tile.get_slope() != 0) {
            err = command_x.set_slope(our_player, tile, 0 )
            if (err) gui.add_message_at(our_player, "Failed to flatten slope at " + coord_to_string(tile) +"\n" + err, tile)
          }
          if (err == null) {
            err = command_x.build_station(our_player, tile, planned_harbour_flat, dir.backward(d_water_to_land))
            if (err) gui.add_message_at(our_player, "Failed to build flat harbour at " + coord_to_string(tile) +"\n" + err, tile)
          }
          local size = planned_harbour_flat.get_size(0)
          len = size.x*size.y
          break
        }
      }
    }
    if (err) {
      return err;
    }

    // halt at this harbour
    local harbour_halt = halt_x.get_halt(tile, our_player)

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

            local halt = halt_x.get_halt(to, our_player)

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

  function repair_keys()
  {
    local cht = {}
    foreach(key,val in c_harbour_tiles) {
      local rkey = repair_key(key)
      cht[rkey] <- val
    }
    c_harbour_tiles = cht
  }

  static function repair_key(key)
  {
    // replace ``dd:dd:dd'' by ``coord3d_dd_dd_dd''
    local a = split(key, ":-")
    if (a.len() == 1) {
      return key
    }
    local rkey = "coord3d_" + a[0]
    for(local i=1; i<a.len(); i++) {
      rkey += "_" + a[i]
    }
    return rkey
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
          local move   = cnode.is_straight_move(d)  ?  cost_straight  :  cost_curve
          local dist   = estimate_distance(to)

          local cost   = cnode.cost + move
          local weight = cost + dist
          // use jump-point search (see dataobj/route.cc)
          local jps = cnode.previous ? (water_dir ^ 0x0f) | d | cnode.dir | from.get_canal_ribi() : 0x0f

          local node = ab_node(to, cnode, cost, dist, d, jps)
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
      add_to_open(ab_node(s, null, 1, dist+1, 0), dist+1)
    }

    search()

    if (route.len() == 0) {
      print("No water route found")
      return { err =  "No route" }
    }

    if (route.len() > 0) {
      return { start = route.top(), end = route[0] }
    }

    print("No water depot route found")
    return { err =  "No route" }
  }
}

class route_finder_water_depot extends route_finder_water
{
  function estimate_distance(c)
  {
    local t = tile_x(c.x, c.y, c.z)
    if (t.is_water()  &&  t.get_objects().get_count()==0) {
      return 0
    }
    local depot = t.find_object(mo_depot_water)
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
      if (t.get_objects().get_count()==0) {
        // depot not existing, we must build, increase weight
        weight += 25 * cost_straight
      }
    }
    base.add_to_open(c, weight)
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
      process_node(ab_node(watertiles[i], null, 1, dist+1, 0))
    }

    search()

    if (route.len() > 0) {
      return { depot = route[0] }
    }
    print("No water depot route found")
    return { err =  "No route" }
  }
}
