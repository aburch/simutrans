/**
 * Classes to manage industry connections.
 */

/**
 * Class to store lines plus extra data.
 * Derived from line_x, so it inherits all the methods from line_x.
 */
class my_line_t extends line_x
{
  //dings_bums = 0 // extra data, you can put more such variables here.
  double_ways_count   = 0 // count of double way build
  double_ways_build   = 0 // double way build: 0 = no ; 1 = yes
  optimize_way_line   = 0 // is way line optimize: 0 = no ; 1 = run 1 ; 2 = run 2 ....
  destroy_line_month  = world.get_time().month // test month save
  line_way_speed      = 0 // save way speed for line
  build_line          = world.get_time() // create line
  next_vehicle_check  = world.get_time().ticks + world.get_time().ticks_per_month // save ticks for next vehicle check
  next_check          = world.get_time().ticks
  stuck_check         = 0 // check stuck vehicles
  halt_length         = 0 // tiles from halts
  add_convoy_time     = world.get_time().ticks + world.get_time().ticks_per_month // save ticks for next add convoy - destroy convoy then add time
  line_bridges_count  = 0 // count non player bridges from line
  line_bridges        = [] // array bridge start/end from not player bridges
  way_line_count      = 1 // more lines on way > 1

  constructor(line /* line_x */)
  {
    base.constructor(line.id)
    // never change id !
  }
  function _save()
  {
    return ::saveinstance("my_line_t", this)
  }
}

/**
 * A link is a connection between two factories.
 * Save its state here.
 */
class industry_link_t
{
  f_src   = null // factory_x
  f_dest  = null // factory_x
  freight = null // good_desc_x

  state = 0
  lines = null   // array<line_x>
  build_cost_link = 0
  build_cost_rail = 0
  build_cost_road = 0
  build_cost_ship = 0
  // 1 - link has combined lines
  combined_link = 0
  /*
  double_ways_count   = 0 // count of double way build
  double_ways_build   = 0 // double way build: 0 = no ; 1 = yes
  optimize_way_line   = 0 // is way line optimize: 0 = no ; 1 = yes
  destroy_line_month  = null // test month save
  line_way_speed      = 0 // save way speed for line
  build_line          = 0 // create line
  next_vehicle_check  = 0 // save ticks for next vehicle check
  */
  // next check needed if ticks > next_check
  // state == st_missing: check availability again
  // state == st_build: check for possible upgrades
  next_check = 0 // if world.get_time().ticks > next_check then state is set to free (for state == failed, missing)

  static st_free    = 0 /// not registered
  static st_planned = 1 /// link is planned
  static st_failed  = 2 /// construction failed, do not try again
  static st_built   = 3 /// connection successfully built
  static st_missing = 4 /// missing infrastructure, try again later

  constructor(s,d,f)
  {
    f_src = s
    f_dest = d
    freight = good_desc_x(f)
    lines = []
  }

  function append_line(l)
  {
    lines.append(my_line_t(l))
  }
  function remove_line(l)
  {
    foreach(idx, line in lines) {
      if (line.id == l.id) {
        lines.remove(idx)
        break
      }
    }
  }
  function _save()
  {
    return ::saveinstance("industry_link_t", this)
  }
  // upgrade elements in lines array from line_x to my_line_t
  function update_lines()
  {
    if (lines.len() > 0  &&  lines[0].getclass() == line_x) {
      local map_to_my_line_t = function(line) { return my_line_t(line); }
      lines.apply(map_to_my_line_t)
    }
  }
}

/**
 * Manage the links operated by us.
 */
class industry_manager_t extends manager_t
{
  link_list = null
  link_iterator = null

  // print messages box
  // 1 = vehicles
  // 2 =
  // 3 =
  print_message_box = 0

  constructor()
  {
    base.constructor("industry_manager_t")
    link_list = {}
    ::industry_manager = this
    debug = false
  }

  /// Generate unique key from link data
  static function key(src, des, fre)
  {
    return ("freight-" + fre + "-from-" + coord_to_key(src) + "-to-"  + coord_to_key(des) ).toalnum()
  }

  function set_link_state(src, des, fre, state)
  {
    local k = key(src, des, fre)

    try {
      link_list[k].state = state
    }
    catch(ev) {
      // not existing - create entry
      local l = industry_link_t(src, des, fre)
      l.state = state
      link_list[k] <- l
    }

    switch(state) {
      case industry_link_t.st_built:
        link_list[k].next_check = today_plus_months(1)

        local text = ""
        text = "Transport " + translate(fre) + " from "
        text += coord(src.x, src.y).href(src.get_name()) + " to "
        text += coord(des.x, des.y).href(des.get_name()) + "<br>"

        if ( des.output.len() == 0 ) {
            build_check_month = world.get_time().ticks + (3 * world.get_time().ticks_per_month)
            //gui.add_message_at(our_player, "### " + des.get_name() + " ## end consumer set build_check_month = " + build_check_month, world.get_time())
        } else {
            build_check_month = world.get_time().ticks + world.get_time().ticks_per_month
            //gui.add_message_at(our_player, "### " + des.get_name() + " ##  set build_check_month = " + build_check_month, world.get_time())
        }
        break
      case industry_link_t.st_missing:
        link_list[k].next_check = today_plus_months(3)
        //gui.add_message_at(our_player, "### link " + k + " ##  state = " + state, world.get_time())
        break
      case industry_link_t.st_failed:
        link_list[k].next_check = today_plus_months(12)
        link_list[k].state = 4
        //gui.add_message_at(our_player, "### link " + k + " ##  state = " + state, world.get_time())
    }
  }

  function get_link_state(src, des, fre)
  {
    local link = access_link(src, des, fre)

    return link  ?  link.state  :  industry_link_t.st_free
  }


  /*
   * select
   *  0 - build_cost_link
   *  1 - build_cost_rail
   *  2 - build_cost_road
   *  3 - build_cost_ship
   *
   */
  function set_link_build_cost(src, des, fre, cost, select)
  {
    local k = key(src, des, fre)
    local l = industry_link_t(src, des, fre)
    local i = 0

    switch(select) {
      case 0:
        try {
          link_list[k].build_cost_link = cost
        }
        catch(ev) {
          // not existing - create entry
          l.build_cost_link = cost
          i = 1
        }
        break
      case 1:
        try {
          link_list[k].build_cost_rail = cost
        }
        catch(ev) {
          // not existing - create entry
          l.build_cost_rail = cost
          i = 1
        }
        break
      case 2:
        try {
          link_list[k].build_cost_road = cost
        }
        catch(ev) {
          // not existing - create entry
          l.build_cost_road = cost
          i = 1
        }
        break
      case 3:
        try {
          link_list[k].build_cost_ship = cost
        }
        catch(ev) {
          // not existing - create entry
          l.build_cost_ship = cost
          i = 1
        }
        break
    }
    if ( i == 1 ) {
      link_list[k] <- l
    }

  }

  function get_link_build_cost(src, des, fre, select)
  {
    local link = access_link(src, des, fre)

    switch(select) {
      case 0:
        return link.build_cost_link
        break
      case 1:
        return link.build_cost_rail
        break
      case 2:
        return link.build_cost_road
        break
      case 3:
        return link.build_cost_ship
        break
    }
  }

  function set_combined_link(src, des, fre, state)
  {
    local k = key(src, des, fre)

    try {
      link_list[k].combined_link = state
    }
    catch(ev) {
      // not existing - create entry
      local l = industry_link_t(src, des, fre)
      l.combined_link = state
      link_list[k] <- l
    }
  }

  function get_combined_link(src, des, fre)
  {
    local link = access_link(src, des, fre)

    return link.combined_link
  }

  function access_link(src, des, fre)
  {
    local k = key(src, des, fre)
    local res
    try {
      res = link_list[k]
    }
    catch(ev) {
      res = null
    }
    return res
  }

  /**
   * Loop through all links.
   */
  function work()
  {
    month_check_message()

    set_map_vehicles_counts()

    // iterate the link_iterator, which is a generator
    if (link_iterator == null) {
      // this is a generator
      link_iterator = link_iteration()
    }
    if (link_iterator.getstatus() != "dead") {
      resume link_iterator
    }
    else {
      link_iterator = null
      return r_t(RT_SUCCESS);
    }

    return r_t(RT_PARTIAL_SUCCESS);
  }

  function link_iteration()
  {
    foreach(link in link_list) {
      if (!check_link(link)) {
        continue
      }
      yield link
    }
  }

  /**
   * Check link:
   * - if state is st_missing set state to st_free after some time
   * - for working links see after their lines
   * - @returns true if some work was done
   */
  function check_link(link)
  {
    //gui.add_message_at(our_player, "#######" + link.f_src + " - " + link.f_src.get_name() + " - link.state " + link.state, world.get_time())
    switch(link.state) {
      case industry_link_t.st_free:
        break
      case industry_link_t.st_planned:
        return false
      case industry_link_t.st_built:
        if (link.lines.len()==0) return false
        break
      case industry_link_t.st_failed:
        break
      case industry_link_t.st_missing:
        if (link.next_check >= world.get_time().ticks) return false
        // try to plan again
        link.state = industry_link_t.st_free
        link.next_check = 0
        return false
    }

    // iterate through all lines
    foreach(index, line in link.lines) {
      if ( line.is_valid() ) {
        //gui.add_message_at(our_player, "####### valid line " + line.get_name(), world.get_time())
        if ( line.next_vehicle_check < world.get_time().ticks ) {
          check_link_line(link, line)
        }
      } else {
        //gui.add_message_at(our_player, "####### invalid line " + line, world.get_time())
        link.lines.remove(index)
      }
    }


    return true
  }


  /**
    *
    *
    *
    */
  function check_pl_lines() {
    local line_list = player_x(our_player.nr).get_line_list()
/*
    f_src = s
    f_dest = d
    freight = good_desc_x(f)
*/
    local print_message_box = 0

    local pl_lines = []

    local line_ai_count = 0
    foreach(link in link_list) {
      //gui.add_message_at(player_x(our_player.nr), " ####### link check: f_src " + link.f_src.get_name() + " f_dest " + link.f_dest.get_name() + " freight " + link.freight.get_name(), world.get_time())

      foreach(index, line in link.lines) {
        //gui.add_message_at(player_x(our_player.nr), "####### line check " + line.get_name(), world.get_time())
        if ( line.is_valid() && line.get_owner().nr == our_player.nr ) {
          line_ai_count++
          pl_lines.append(line)
        }
      }
    }

    local ai_lines_missing = []
    if ( pl_lines.len() > 0 ) {
      foreach(line in line_list) {
        local c = 0
        for ( local i = 0; i < pl_lines.len(); i++ ) {
          if ( line.get_name() == pl_lines[i].get_name() ) {
            c = 1
            break
          }
        }
        if ( c == 0 ) {
          ai_lines_missing.append(line)
        }
      }
    }
    //::debug.pause()
    //gui.add_message_at(player_x(our_player.nr), " ####### line check: line_list.get_count() " + line_list.get_count() + " ## line_ai_count " + line_ai_count, world.get_time())

    //
    if ( line_list.get_count() > line_ai_count ) {
      for ( local i = 0; i < ai_lines_missing.len(); i++ ) {
        // rename line
        local line_name   = ai_lines_missing[i].get_name()
        local str_search  = ") " + translate("Line")
        local st_names    = ai_lines_missing[i].get_schedule().entries
        local wt_name     = ["", "road", "Train", "Ship"]

        local f_src   = st_names[0].get_halt(our_player).get_factory_list()
        local f_dest  = st_names[st_names.len()-1].get_halt(our_player).get_factory_list()

        local freight = ""
        if ( f_src.len() == 1 && f_dest.len() == 1 ) {
          // list output goods
          local good_list_out = []
          foreach(good, islot in f_src[0].output) {
            good_list_out.append(good)
          }
          if ( good_list_out.len() == 1 ) {
            // one good output
            freight = good_list_out[0]
          } else {
            // more output goods - check good to f_dest
            //local good_list_in = []
            foreach(good, islot in f_dest[0].input) {
              //good_list_in.append(good)
              for ( local i = 0; i < good_list_out.len(); i++ ) {
                if ( good_list_out[i] == good && freight == "" ) {
                  freight = good
                }
              }
            }
            // todo more goods check

          }

        } else if ( f_src.len() == 0 || f_dest.len() == 0 ) {
          // combined line
          if ( f_src.len() == 0 && f_dest.len() == 1 ) {
            local connect_lines = st_names[0].get_halt(our_player).get_line_list()
            local cline_list = []
            foreach(line in connect_lines) {
              if ( line.get_name() != line_name ) {
                cline_list.append(line)
              }
            }
            if ( cline_list.len() == 1 ) {
              local cline_st_names = cline_list[0].get_schedule().entries
              f_src = cline_st_names[0].get_halt(our_player).get_factory_list()
              if ( f_src.len() == 1 ) {
                local good_list_out = []
                foreach(good, islot in f_src[0].output) {
                  good_list_out.append(good)
                }
                if ( good_list_out.len() == 1 ) {
                  // one good output
                  freight = good_list_out[0]
                } else {
                  // more output goods - check good to f_dest
                  //local good_list_in = []
                  foreach(good, islot in f_dest[0].input) {
                    ///good_list_in.append(good)
                    for ( local i = 0; i < good_list_out.len(); i++ ) {
                      if ( good_list_out[i] == good && freight == "" ) {
                        freight = good
                      }
                    }
                  }
                  // todo more goods check
                }
              }
            }


          }


        }// todo check by more factorys

        // rename standard line name '(x) Line'
        if ( line_name.find(str_search) != null && freight != "" ) {
          local new_name = translate(wt_name[ai_lines_missing[i].get_waytype()]) + " " + translate(freight) + " " + st_names[0].get_halt(our_player).get_name() + " - " + st_names[st_names.len()-1].get_halt(our_player).get_name()

          //gui.add_message_at(player_x(our_player.nr), our_player.get_name() + " ####### line rename: " + line_name, world.get_time())
          //gui.add_message_at(player_x(our_player.nr), our_player.get_name() + " ####### new name: " + new_name, world.get_time())

          ai_lines_missing[i].set_name(new_name)
        }

        if ( freight != "" ) {
          //missing line add ai line list
          industry_manager.set_link_state(f_src[0], f_dest[0], freight, industry_link_t.st_built)
          industry_manager.access_link(f_src[0], f_dest[0], freight).append_line(ai_lines_missing[i])
        }

      }
    }

    if ( line_list.get_count() > line_ai_count ) {
      if ( print_message_box == 1 ) {
        gui.add_message_at(player_x(our_player.nr), our_player.get_name() + " ####### line check: not all listet in ai lines ", world.get_time())
        gui.add_message_at(player_x(our_player.nr), " ####### line check: line_list.get_count() " + line_list.get_count() + " ## line_ai_count " + line_ai_count, world.get_time())
      }
      for ( local i = 0; i < ai_lines_missing.len(); i++ ) {
        gui.add_message_at(player_x(our_player.nr), "####### line missing " + ai_lines_missing[i].get_name(), world.get_time())
      }
    }

  }

  /**
   * Manages convoys of one line: withdraw if there are too many, build new ones, upgrade to newer vehicles
   */
  function check_link_line(link, line)
  {

    if ( debug ) ::debug.set_pause_on_error(true)

    // set first run as line build time
    /*if ( line.build_line == 0 ) {
      line.build_line = world.get_time()
    }*/

    // set next check
    line.next_check = world.get_time().ticks + world.get_time().ticks_per_month

    // 0 = off
    // 1
    // 2 = station expand
    // 3
    // 4 = cnv status retired / all to old
    // 5 = electrified
    // 6 and pl 4
    local print_message_box = 0

    local wt = line.get_waytype()
    // find route
    local nexttile = []
    if (wt != wt_water && wt != wt_air) {
      local entries = line.get_schedule().entries
      local start = null
      local end = null
      if ( entries.len() >= 2 ) {
        start = tile_x(entries[0].x, entries[0].y, entries[0].z)
        end = tile_x(entries[entries.len()-1].x, entries[entries.len()-1].y, entries[entries.len()-1].z)
      }

      local asf = astar_route_finder(wt)
      local result = asf.search_route([start], [end])
      // result is contains routes-array or error message
      // route is backward from end to start

      if ("err" in result) {
        gui.add_message_at(our_player, " ### no route found: " + result.err, start)
        gui.add_message_at(our_player, " ### line: " + line.get_name(), world.get_time())
        gui.add_message_at(our_player, " ### start: " + coord3d_to_string(start) + " ### end: " + coord_to_string(end), start)
        return //nexttile
      }
      else {
        //gui.add_message_at(our_player, " ### route found: length =  " +  result.routes.len(), start)
        // route found, mark tiles
        foreach(node in result.routes) {
          local tile = tile_x(node.x, node.y, node.z)
          nexttile.append(tile)
        }
        sleep()
      }
    }

    dbgprint("Check line " + line.get_name())
    if ( our_player.nr == 6 && print_message_box == 6 ) {
      gui.add_message_at(our_player, "Check line " + line.get_name(), world.get_time())
    }

    local line_new = 0
    local bilanz_year = 0
    if ( line.get_owner().nr == our_player.nr ) {
      // non profit in 5 months then destroy line //cnv.get_distance_traveled_total() > 3 &&
      local profit_count = line.get_profit()
      //if ( cnv.get_distance_traveled_total() < 3 ) { return }
      local month_check = 0
      for ( local i = 0; i < 12; i++ ) {
        bilanz_year += profit_count[i]
        if ( our_player.nr == 3 && print_message_box == 6 ) {
          //gui.add_message_at(our_player, "bilanz_year += profit_count["+i+"] " + bilanz_year, world.get_time())
        }
        if ( i >= 4 ) {
          month_check += profit_count[i]
        }
      }

      if ( bilanz_year < 0 ) { /*
        gui.add_message_at(our_player, "Check line " + line.get_name(), world.get_time())
        gui.add_message_at(our_player, "bilanz_year " + bilanz_year, world.get_time())
        gui.add_message_at(our_player, "month_check " + month_check, world.get_time())
        //::debug.pause()*/
      }

        //profit_count[4] <= 0 &&
      if ( profit_count[3] <= 0 && profit_count[2] <= 0 && profit_count[1] <= 0 && profit_count[0] <= 0 && month_check != 0) {
        //line.get_traveled_distance() > 1 && line.get_traveled_distance() < 25 && line.get_loading_level() == 0 &&
        local chk_f_link = check_factory_links(link.f_src, link.f_dest, link.freight.get_name())
            if ( bilanz_year <= 0 && print_message_box > 0 ) {
              gui.add_message_at(our_player, "(488) : Check line " + line.get_name(), world.get_time())
              gui.add_message_at(our_player, "(488) : chk_f_link " + chk_f_link, world.get_time())
              gui.add_message_at(our_player, "(488) : line.destroy_line_month " + line.destroy_line_month + " -#- " + (world.get_time().month+1), world.get_time())
              //::debug.pause()
            }
        if ( line.destroy_line_month != (world.get_time().month+1) && (chk_f_link > 1 || (chk_f_link == 1 && link.f_src.get_suppliers().len() == 0 && link.f_dest.get_consumers().len() == 0) || (bilanz_year <= 0 && chk_f_link > 1)) ) {
          local erreg = destroy_line(line, link.freight, link)
          //gui.add_message_at(our_player, "(495) : destroy_line(line, link.freight, link) = " + erreg, world.get_time())
          if ( erreg == false ) {
            line.destroy_line_month = world.get_time().month
          } else if ( erreg == true ) {
            link.state = 4
            return
          } else if ( erreg == null ) {
            link.state = 4
            return
          }


          // line not complete
          local entries = line.get_schedule().entries
          local start_l = null
          local end_l = null
          if ( wt == wt_water ) {
            if ( entries.len() >= 2 ) {
              local start_h = entries[0].get_halt(our_player)
              local end_h = entries[entries.len()-1].get_halt(our_player)
              local t = start_h.get_tile_list()
              start_l = tile_x(t[0].x, t[0].y, t[0].z)
              t = end_h.get_tile_list()
              end_l = tile_x(t[0].x, t[0].y, t[0].z)
            }
          } else {
            if ( entries.len() >= 2 ) {
              start_l = tile_x(entries[0].x, entries[0].y, entries[0].z)
              end_l = tile_x(entries[entries.len()-1].x, entries[entries.len()-1].y, entries[entries.len()-1].z)
            }
          }
          if ( test_halt_waytypes(end_l) == 1 && end_l.get_halt().get_factory_list().len() == 0 ) {
            local erreg = destroy_line(line, link.freight, link)
            /*if ( bilanz_year < 0 ) {
              gui.add_message_at(our_player, "line 490 : erreg = destroy_line " + erreg, world.get_time())
            }*/
            if ( erreg == false ) {
              line.destroy_line_month = world.get_time().month
            } else if ( erreg == true ) {
              link.state = 4
              return
            }
          }




        } else {
          //gui.add_message_at(our_player, "return cnv/line new " + line.get_name(), world.get_time())
        }

        if ( month_check == 0 ) {
          line_new = 1
        }
      }
    }

    // find convoy
    local cnv = null
    local cnv_count = 0
    local cnv_max_speed = 0
    local cnv_retired = []
    local new_cnv_add_line = false
    local stucked_cnv = []
    {
      local list = []
      // remove withdrawn convois
      foreach(cnv in line.get_convoy_list()) {
        if (cnv.is_valid() && !cnv.is_withdrawn()) {
          list.append(cnv);
        }
      }

      cnv_count = list.len()
      if (cnv_count == 0 || ( bilanz_year == 0 && cnv_count == 1 )) {
        // 0 convoy destroy line
        // 1 convoy and profit year 0
        if ( line.get_owner().nr == our_player.nr ) {
          destroy_line(line, link.freight, link)
          sleep()
        }
        /*if ( bilanz_year < 0 ) {
          gui.add_message_at(our_player, "line 536 : destroy_line ", world.get_time())
        }*/
        return
      }

      local entries = line.get_schedule().entries
      local start_h = entries[0].get_halt(our_player)
      local end_l = tile_x(entries[entries.len()-1].x, entries[entries.len()-1].y, entries[entries.len()-1].z)

      // remove stucked convoy
      // cnv_count > 10 then remove cnv_count/2
      local cnv_remove_count = cnv_count
      if ( cnv_count > 10 ) {
        cnv_remove_count = abs(cnv_count / 2)
        //gui.add_message_at(our_player, "(738) ####  cnv_remove_count " + cnv_remove_count, world.get_time())
      }
      local loading_cnv_stucked = 0

      for ( local i = 0; i < cnv_count; i++ ) {
        if ( !list[i].is_withdrawn() && cnv == null ) {
          cnv = list[i]
        }
        // stucked convoy destroy
        local d = list[i].get_traveled_distance()
        if ( list[i].get_distance_traveled_total() > 0 && d[0] == 0 && d[1] == 0 && list[i].is_loading() == false && cnv_count > 1 && list[i].get_loading_level() == 0 && stucked_cnv.len() <= cnv_remove_count ) {
          //gui.add_message_at(our_player, "####### destroy stucked road vehicles " + cnv_count, world.get_time())list[i].get_waytype() == wt_road &&
          stucked_cnv.append(list[i])
          //remove_cnv++
        }
        // destroy convoys in depots
        if ( list[i].is_in_depot() ) {
          stucked_cnv.append(list[i])
        }
        // destroy no waiting goods
        if ( start_h != null ) {
          local d = start_h.get_waiting()
          if ( d[0] == 0 && list[i].is_loading() == false && stucked_cnv.len() <= cnv_remove_count && list[i].get_loading_level() == 0 ) {
            //gui.add_message_at(our_player, "(605) ####### " + start_h.get_name() + " - destroy waiting road vehicles ", world.get_time())
            stucked_cnv.append(list[i])
            //remove_cnv++
          }
        }
        // stucked loaded convoy
        if ( d[0] == 0 && d[1] == 0 && list[i].get_loading_level() > 0 && loading_cnv_stucked == 0 && line.get_waytype() == wt_road ) {
          loading_cnv_stucked = 1
          //gui.add_message_at(our_player, "(768) ####  loading convoy stucked ", world.get_time())
          local traffic_obj = find_signal("traffic_light", line.get_waytype())

          if ( traffic_obj != null ) {
            // end_l
            local asf = astar_route_finder(wt)
            local result = asf.search_route([list[i].get_pos()], [end_l])
            local route_tile = []
            if ("err" in result) {

            }
            else {
              foreach(node in result.routes) {
                local tile = tile_x(node.x, node.y, node.z)
                route_tile.append(tile)
              }
              sleep()
            }
            local traffic_build = 0
            for ( local s = 0; s < route_tile.len(); s++ ) {
              local test_way = route_tile[s].find_object(mo_way)
              local test_traffic_light = route_tile[s].find_object(mo_roadsign)

              if ( test_traffic_light == null && dir.is_threeway(route_tile[s].get_way_dirs(wt_road)) && (test_way.get_owner().nr == our_player_nr || test_way.get_owner().nr == 1) ) {

                local err = command_x.build_sign_at(our_player, route_tile[s], traffic_obj)
                traffic_build++

              } else if ( test_traffic_light != null ) {
                traffic_build++
              }

              if ( traffic_build >= 2 ) {
                continue
              }

            }

          }

        }
      }

      // stucked vehicle remove, not last vehicle
      if ( stucked_cnv.len() > 0 ) {
        local j = stucked_cnv.len()
        if ( cnv_count == j ) { j-- }
        for ( local i = 0; i < j; i++ ) {
          if ( stucked_cnv[i].is_valid() ) {
            stucked_cnv[i].destroy(our_player)
          }
        }
        sleep()

        local road_car_rate = set_map_vehicles_counts(1)
        local next_time = road_line_check
        if ( road_car_rate < 10 ) {
          next_time = road_line_check / 2
        }
        if ( line.get_waytype() == wt_road ) {
          line.next_vehicle_check = world.get_time().ticks + next_time
          if ( cnv_remove_count < cnv_count ) {
            line.next_vehicle_check = world.get_time().ticks + (next_time * 4)
          }
          if ( line.stuck_check == 1 ) {
            // small check intervall
            line.next_vehicle_check = world.get_time().ticks + (next_time / 2)
          }
        } else {
          line.next_vehicle_check = world.get_time().ticks + (world.get_time().ticks_per_month * 2)

          // set longer time for add convoy by remove convoy in last 6 month
          local cnv_line = line.get_convoy_count()
          local a = 0
          for ( local x = 1; x < 8; x++ ) {
            if ( cnv_line[x] <= cnv_line[0] ) {
              a++
            }
          }
          line.add_convoy_time = line.next_vehicle_check + (world.get_time().ticks_per_month * a)
          //gui.add_message_at(our_player, "#656# line.add_convoy_time : " + line.add_convoy_time + " -- a : " + a, world.get_time())
        }

        local msgtext = format(translate("%s removes convoys from line: %s"), our_player.get_name(), line.get_name())
        gui.add_message_at(our_player, msgtext, world.get_time())

        return
      }

      if ( line_new == 1 ) { return }

      // check speed from convoys
      local a = convoy_max_speed(list[0])
      cnv_max_speed = a

      // check cnv is retired first cnv
      if ( list[0].has_obsolete_vehicles() && !list[0].is_withdrawn() ) {
        if ( print_message_box == 4 ) {
          gui.add_message_at(our_player, " retired convoy " + list[0].get_name(), world.get_time())
        }
        cnv_retired.append(list[0])
      }

      local veh_count_line = line.get_convoy_count()
      if ( veh_count_line[0] > veh_count_line[1] ) {
        // not remove cnv from line by add cnv this month
        new_cnv_add_line = true
        //gui.add_message_at(our_player, "####### cnv new this month ", world.get_time())
      }
      //&& !new_cnv_add_line

      local speed_count = 1
      for ( local i = 1; i < cnv_count; i++ ) {
        local s = convoy_max_speed(list[i])

        if ( a == s ) {
          speed_count++
        } else if ( a < s ) {
          cnv_max_speed = s
        }
        // check cnv is retired all cnv
        if ( list[i].has_obsolete_vehicles() && !list[i].is_withdrawn() ) {
          if ( print_message_box == 4 ) {
            gui.add_message_at(our_player, " retired convoy " + list[i].get_name(), world.get_time())
          }
          cnv_retired.append(list[i])
        }
      }
      //gui.add_message_at(our_player, " max speed " + cnv_max_speed, world.get_time())

      if ( speed_count < cnv_count && wt != wt_rail ) {
        // update convoys - retire slower convoys
        local k = 0
        for ( local i = 0; i < cnv_count; i++ ) {
          if ( convoy_max_speed(list[i]) < cnv_max_speed && !list[i].is_withdrawn() ) {
            list[i].toggle_withdraw(our_player)
            k++
          }
        }
        if ( k > 0 ) {
          local msgtext = format(translate("vehicles of the line %s were retired"), line.get_name())
          gui.add_message_at(our_player, msgtext, world.get_time())

          return

        }
      } else if ( cnv_count == cnv_retired.len() ) {
        // all cnv retired
        if ( print_message_box == 4 ) {
          gui.add_message_at(our_player, " all convoys to old -> upgrade_link_line", world.get_time())
        }
        upgrade_link_line(link, line, cnv_max_speed, nexttile)
        return
      }

      if ( cnv_retired.len() > 0 && cnv_retired.len() < cnv_count ) {
        // update convoys - retire retired convoys
        local k = 0
        for ( local i = 0; i < cnv_retired.len(); i++ ) {
          if ( !cnv_retired[i].is_withdrawn() ) {
            cnv_retired[i].toggle_withdraw(our_player)
            k++
          }
        }
        if ( k > 0 ) {
          local msgtext = format(translate("vehicles of the line %s were retired"), line.get_name())
          gui.add_message_at(our_player, msgtext, world.get_time())

          return

        }
      } else if ( cnv_retired.len() == 1 && cnv_retired.len() == cnv_count ) {
        //gui.add_message_at(our_player, "*** cnv_upgrade = 1 -> line " + line.get_name(), world.get_time())
        // create new convoy before retire retired convoy
        link.next_check = today_plus_months(1)
        if ( upgrade_link_line(link, line, cnv_max_speed, nexttile) ) {
          // update successful
          return
        }
      }
    }

    sleep()
    if ( cnv == null || !cnv.is_valid() ) { return }

    // find route
    //local nexttile = []
    local nexttile_r = []
    if (wt != wt_water && wt != wt_air) {
      local entries = cnv.get_schedule().entries
      local start = null
      local end = null
      if ( entries.len() >= 2 ) {
        start = tile_x(entries[0].x, entries[0].y, entries[0].z)
        end = tile_x(entries[entries.len()-1].x, entries[entries.len()-1].y, entries[entries.len()-1].z)
      }

      local asf = astar_route_finder(wt)
      if ( wt == wt_rail && line.double_ways_build > 0 ) {
        // return way for upgrade double ways
        local result = asf.search_route([end], [start])
        if ("err" in result) {

        }
        else {
          foreach(node in result.routes) {
            local tile = tile_x(node.x, node.y, node.z)
            nexttile_r.append(tile)
          }
          sleep()
        }
      }
    }

    // check bridges other player in route
    if ( line.line_bridges.len() > 0 ) {
      local g = line.line_bridges
      for ( local x = 0; 0 < g.len(); x++ ) {
        local gt = g[x]
        if ( gt[0].is_bridge == false && gt[1].is_bridge == false ) {
          local bridge_obj = find_object("bridge", wt, line.line_way_speed)
          command_x.build_bridge(our_player, gt[0], gt[1], bridge_obj)
          if ( debug ) gui.add_message_at(our_player, "**** line 800 - build missing bridge in route ## ", gt[0])
        }
      }
      //if ( debug ) gui.add_message_at(our_player, "**** line 816 - line_bridges_count = " + line.line_bridges_count + " ## ", g[0])

    }

    if ( wt == wt_road && line.stuck_check == 0 ) {
      local t = nexttile[2];
      local d = t.get_way_dirs(wt)
      if ( is_threeway(d) ) {
        line.stuck_check = 1
        gui.add_message_at(our_player, "**** line 997 - set line.stuck_check = 1 ", t)
      }
    }

    // count bridges from other player
    if ( wt == wt_road && line.line_bridges_count == 0 ) {
      local tiles = nexttile
      local bridge_start = null
      local bridge_end = null
      for ( local x = 0; x < tiles.len(); x++ ) {
        if ( tiles[x].is_bridge() && bridge_start != null ) {
          local test_bridge = tiles[x].find_object(mo_bridge) //.get_desc()
          if ( test_bridge.get_owner().nr != our_player_nr && bridge_start == null ) {
            bridge_start = tiles[x]
          }
        } else if ( bridge_start != null && tiles[x].is_bridge() == false ) {
          bridge_end = tiles[x-1]
          line.line_bridges.append([bridge_start, bridge_end])
          line.line_bridges_count++
          bridge_start = null
          bridge_end = null
        }
      }
      if ( line.line_bridges.len() > 0 ) {
        local g = line.line_bridges[0]
        if ( debug ) gui.add_message_at(our_player, "**** line 828 - line_bridges_count = " + line.line_bridges_count + " ## ", g[0])
      }
    }

    if (our_player.get_current_cash() > 50000 && wt != wt_water && wt != wt_air) {
      if ( line.optimize_way_line == 0 ) {
        // optimize way line befor build double ways
        local err = optimize_way_line(nexttile, wt, line.optimize_way_line, line)
        if ( err ) {
          line.optimize_way_line = 1
        }
      } else if ( line.optimize_way_line == 1 && our_player.get_current_cash() > 500000 ) {
        local err = optimize_way_line(nexttile, wt, line.optimize_way_line, line)
        if ( err ) {
          line.optimize_way_line = 2
        }
      } else if ( line.optimize_way_line == 2 && our_player.get_current_cash() > 1000000 ) {
        local err = optimize_way_line(nexttile, wt, line.optimize_way_line, line)
        if ( err ) {
          line.optimize_way_line = 3
        }
      } else if ( line.optimize_way_line == 3 && our_player.get_current_cash() > 1000000 && world.get_time().year >= 1950 ) {
        local err = optimize_way_line(nexttile, wt, line.optimize_way_line, line)
        if ( err ) {
          line.optimize_way_line = 4
        }
      }
    }

      if ( bilanz_year < 0 ) {
        //gui.add_message_at(our_player, " line 756 - bilanz_year < 0", world.get_time())
        //::debug.pause()
      }
    // way speed
    //gui.add_message_at(our_player, " -##- " + link.line_way_speed + " old speed save line " + line.get_name(), world.get_time())
    //gui.add_message_at(our_player, " cnv max speed " + cnv_max_speed, world.get_time())
    if ( cnv_max_speed > line.line_way_speed && nexttile.len() > 3 ) {

      local way_speed = 500
      local upgrade_tiles = 0
      for ( local i = 0; i < nexttile.len(); i++ ) {
        local tile = tile_x(nexttile[i].x, nexttile[i].y, nexttile[i].z)
        local tile_way = tile.find_object(mo_way)

        if ( tile_way != null && (tile_way.get_owner().nr == our_player_nr || tile_way.get_owner().nr == 1) && ( !tile.has_two_ways() && !tile.is_bridge() ) ) {
          upgrade_tiles++
          if ( tile_way.get_desc().get_topspeed() < way_speed ) {
            way_speed = tile_way.get_desc().get_topspeed()
          }
            if ( tile_way.get_desc().get_topspeed() == 45 && print_message_box == 1 ) {
              gui.add_message_at(our_player, way_speed + " way speed tile " + coord3d_to_string(tile_x(nexttile[i].x, nexttile[i].y, nexttile[i].z)), tile_x(nexttile[i].x, nexttile[i].y, nexttile[i].z))
            }
        }
      }
      // check double ways by rail
      if ( wt == wt_rail && line.double_ways_build > 0 ) {
        for ( local i = 0; i < nexttile_r.len(); i++ ) {
          local tile = tile_x(nexttile_r[i].x, nexttile_r[i].y, nexttile_r[i].z)
          local tile_way = tile.find_object(mo_way)
          if ( tile_way.get_owner().nr == our_player_nr ) {
            //upgrade_tiles++
            if ( tile_way.get_desc().get_topspeed() < way_speed && ( !tile.has_two_ways() && !tile.is_bridge() ) ) {
              way_speed = tile_way.get_desc().get_topspeed()
            }
          }
        }
      }

      line.line_way_speed = way_speed
      //gui.add_message_at(our_player, way_speed + " way speed line " + line.get_name(), world.get_time())
      //gui.add_message_at(our_player, upgrade_tiles + " possible tiles for upgrading ", world.get_time())
      //gui.add_message_at(our_player, " cnv max speed " + cnv_max_speed, world.get_time())

      // upgrade way
      local way_obj = find_object("way", wt, cnv_max_speed)
      //gui.add_message_at(our_player, " way max speed new " + way_obj.get_topspeed(), world.get_time())

      if ( cnv_max_speed >= way_speed && upgrade_tiles > 2 ) {
        local costs = (upgrade_tiles*(way_obj.get_cost()/100))
        local count_build = 0
        if ( wt == wt_rail && line.double_ways_build > 0 ) {
          costs += line.double_ways_count*8*(way_obj.get_cost()/100)
        }
        if ( our_player.get_current_cash() > costs ) {
          for ( local i = 1; i < nexttile.len(); i++ ) {
            local tile_way_1 = tile_x(nexttile[i-1].x, nexttile[i-1].y, nexttile[i-1].z)
            local tile_way_2 = tile_x(nexttile[i].x, nexttile[i].y, nexttile[i].z)
            if ( tile_way_2.find_object(mo_way) != null && (tile_way_2.find_object(mo_way).get_owner().nr == our_player_nr || tile_way_2.find_object(mo_way).get_owner().nr == 1) && tile_way_2.find_object(mo_way).get_desc().get_topspeed() < way_obj.get_topspeed() ) {
              command_x.build_way(our_player, tile_way_1, tile_way_2, way_obj, true)
              count_build++
            }
          }
          // build double ways by rail
          if ( wt == wt_rail && line.double_ways_build > 0 ) {
            for ( local i = 1; i < nexttile_r.len(); i++ ) {
              local tile_way_1 = tile_x(nexttile_r[i-1].x, nexttile_r[i-1].y, nexttile_r[i-1].z)
              local tile_way_2 = tile_x(nexttile_r[i].x, nexttile_r[i].y, nexttile_r[i].z)
              if ( tile_way_2.find_object(mo_way) != null && (tile_way_2.find_object(mo_way).get_owner().nr == our_player_nr || tile_way_2.find_object(mo_way).get_owner().nr == 1) && tile_way_2.find_object(mo_way).get_desc().get_topspeed() < way_obj.get_topspeed() ) {
                command_x.build_way(our_player, tile_way_1, tile_way_2, way_obj, true)
                count_build++
              }
            }
          }
        }

        local start_l = nexttile[nexttile.len()-1]
        local end_l = nexttile[0]

        if (count_build > 0 ) {
          local msgtext = format(translate("%s extends the route from %s (%s) to %s (%s)"), our_player.get_name(), start_l.get_halt().get_name(), coord_to_string(start_l), end_l.get_halt().get_name(), coord_to_string(end_l))
          gui.add_message_at(our_player, msgtext, start_l)
          //gui.add_message_at(our_player, " upgrade way ", world.get_time())
        }

        line.line_way_speed = way_obj.get_topspeed()
      } else {
        // set new way speed by not upgrade way
        line.line_way_speed = way_obj.get_topspeed()
      }

    }

      /*if ( bilanz_year < 0 ) {
        gui.add_message_at(our_player, " line 849 ", world.get_time())
        //::debug.pause()
      }*/

    if (cnv.is_valid() && cnv.is_withdrawn()) {
      // come back later
      if ( print_message_box > 0 ) {
        gui.add_message_at(our_player, "cnv.is_valid() && cnv.is_withdrawn() ", world.get_time())
      }
      return
    }

    // try to upgrade
/*    if (cnv.has_obsolete_vehicles() &&  link.next_check < world.get_time().ticks) {
      link.next_check = today_plus_months(1)
      if (upgrade_link_line(link, line)) {
        // update successful
        gui.add_message_at(our_player, "*** upgrade_link_line -> line " + line.get_name(), world.get_time())
        return
      }
    }
*/
    local lf = link.freight
    // capacity of convoy
    local capacity = 0
    {
      //local lf = link.freight
      foreach(v in cnv.get_vehicles()) {
        local f = v.get_freight()
        if (lf.is_interchangeable(f)) {
          capacity += v.get_capacity()
        }
      }
    }
    dbgprint("Capacity of convoy " + cnv.get_name() + " = " + capacity)
    dbgprint("Speed of convoy " + cnv.get_name() + " = " + cnv.get_speed())

    // iterate through schedule, check for available freight
    local freight_available = false
    local start_l = null
    local end_l = null
    {
      local entries = line.get_schedule().entries
      if ( entries.len() >= 2 ) {
        start_l = tile_x(entries[0].x, entries[0].y, entries[0].z)
        end_l = tile_x(entries[entries.len()-1].x, entries[entries.len()-1].y, entries[entries.len()-1].z)
      }

      local i = 0;

      while(i < entries.len()  &&  !freight_available) {
        local entry = entries[i]
        // stations on schedule
        local halt = entry.get_halt(our_player)
        if (halt == null) continue

        // next station on schedule
        local nexthalt = null
        i++
        while(i < entries.len()) {
          if (nexthalt = entries[i].get_halt(our_player)) break
          i++
        }
        if (nexthalt == null) {
          nexthalt = entries[0].get_halt(our_player)
        }
        // freight available ?
        local freight_on_schedule = halt.get_freight_to_halt(lf, nexthalt)
        local capacity_halt = halt.get_capacity(lf)
        dbgprint("Freight from " + halt.get_name() + " to " + nexthalt.get_name() + " = " + freight_on_schedule)
        // either start is 2/3 full or more good available as one cnv can transport
        freight_available = (3*freight_on_schedule > 2*capacity_halt)
          || (freight_on_schedule > capacity);
      }
    }


      /*if ( bilanz_year < 0 ) {
        gui.add_message_at(our_player, " line 927 ", world.get_time())
        //::debug.pause()
      }*/

    // calc gain per month of one convoy
    local gain_per_m = 0
    {
      local p = line.get_profit()
      gain_per_m = p.reduce(sum) / (p.len() * cnv_count)
      dbgprint("Gain pm = " + gain_per_m)
    }

    // check state if convoys (loading level, stopped, new)
    local cc_load  = 0
    local cc_stop  = 0
    local cc_new   = 0
    local cc_empty = 0
    local cnv_empty_stopped = null
    //local remove_cnv = 0
    {
      local list = line.get_convoy_list()
      //local halt_len = line.halt_length
      local first_cnv = null
      local message_show = 0
      foreach(c in list)
      {
        if (!c.is_valid()  ||  c.is_withdrawn()) {
          // will be withdrawn, ignore it
          continue
        }

        if ( first_cnv == null ) {
          first_cnv = c
        }
        // convoy empty?
        local is_empty = c.get_loading_level() == 0
        // convoy new? less than 2 months old, and not much transported
        local d = c.get_traveled_distance()

        local is_new = (d[0] + d[1] == c.get_distance_traveled_total())
        if (is_new) {
          local t = c.get_transported_goods();
          if (t.reduce(sum) >= 2*capacity) {
            is_new = false
          }
        }
        if (is_new) {
          cc_new ++
           is_empty = false
        }
        // new convoys do not count as empty
        if (is_empty) {
          cc_empty++
        }
        // convoy stopped? but not for loading
        local is_stopped = false
        if (c.get_speed() == 0) {
          if (c.get_loading_limit() > 0) {
            // loading
            cc_load ++
          }
          else {
            cc_stop ++
            is_stopped = true;
          }
        }

        if (is_empty  &&  is_stopped  &&  cnv_empty_stopped==null) {
          //gui.add_message_at(our_player, "### list.len() " + list.get_count() + " ### first_cnv.get_tile_length() " + first_cnv.get_tile_length() + " ### c.get_tile_length() " + c.get_tile_length(), world.get_time())
          if ( list.get_count() == 2 && first_cnv.get_tile_length() < c.get_tile_length() ) {
            first_cnv.toggle_withdraw(our_player)
          } else {
            cnv_empty_stopped = c
          }
        }
        /*
        // stucked road vehicles destroy
        if ( c.get_distance_traveled_total() > 0 && d[0] == 0 && d[1] == 0 && c.is_loading() == false && c.get_waytype() == wt_road && cnv_count > 1) {
          //gui.add_message_at(our_player, "####### destroy stucked road vehicles " + cnv_count, world.get_time())
          c.destroy(our_player)
          cnv_count--
          message_show++
          //remove_cnv++
        }*/
      }

      /*if ( message_show > 0 ) {
        local msgtext = format(translate("%s removes convoys from line: %s"), our_player.get_name(), line.get_name())
        gui.add_message_at(our_player, msgtext, world.get_time())
        return true
      }*/

    }
    dbgprint("Line:  loading = " + cc_load + ", stopped = " + cc_stop + ", new = " + cc_new + ", empty = " + cc_empty)
    dbgprint("")

    if ( print_message_box == 1 ) {
      gui.add_message_at(our_player, "freight_available " + freight_available + " cc_new " + cc_new + " cc_stop " + cc_stop + " cnv.is_valid() " + cnv.is_valid(), world.get_time())
    }
    if ( (freight_available  &&  cc_new == 0  &&  cc_stop < 2 && cnv.is_valid()) || bilanz_year < 0 ) {

      if ( bilanz_year < 0 ) {
        //gui.add_message_at(our_player, " line 1029 - bilanz_year < 0", world.get_time())
        //::debug.pause()
      }
      // stations distance
      local l = 0
      if ( wt == wt_water || wt == wt_air ) {
        l = abs(start_l.x - end_l.x) + abs(start_l.y - end_l.y)
      } else {
        l = check_way_line(start_l, end_l, wt, 0, 0, line)
      }

      local c = 0
      local t = l % 80
      //gui.add_message_at(our_player, "#### way len " + l + " % 80 = " + (l % 80), world.get_time())
      //gui.add_message_at(our_player, "#### way len " + l + " / 80 = " + (l / 80), world.get_time())
      if ( l > 30 && l <= 90 ) {
        c = 1
      } else if ( l > 70 && l <= 140 ) {
        c = 2
      } else if ( l > 140 && l <= 200 ) {
        c = 3
      } else if ( l > 200 && l <= 330 ) {
        c = 4
      } else if ( l > 330 && l < 460 ) {
        c = 5
      } else if ( l >= 460 ) {
        c = (l / 80)
      }


      // no signals and double tracks - limit 1 convoy for rail
      if (wt == wt_rail && cnv_count == 1 && c > 0 && line.double_ways_build == 0 ) {
        if ( print_message_box == 1 ) {
          gui.add_message_at(our_player, "####### cnv.get_waytype() " + cnv.get_waytype() + " cnv.name " + cnv.get_name(), world.get_time())
          gui.add_message_at(our_player, "####### lenght " + l + " double ways " + c, world.get_time())
        }
        //
        // check way for find fields for double track
        if ( line.optimize_way_line == 0 ) {
          local err = optimize_way_line(nexttile, wt, line.optimize_way_line, line)
          if ( err ) {
            line.optimize_way_line = 1
          }
        }
        local s_fields = check_way_line(start_l, end_l, wt, l, c, line)
        local cc = 1

        //gui.add_message_at(our_player, "####### type(s_fields) " + type(s_fields), world.get_time())
        if ( type(s_fields) == "array" ) {
          //gui.add_message_at(our_player, "####### s_fields.len() " + s_fields.len(), world.get_time())
          if ( s_fields.len() == 0 ) {
            s_fields = check_way_line(end_l, start_l, wt, l, c, line)
          }
        }


        /*
         * calculate build cost
         *
         */
        local build_double_ways = false
        if ( s_fields != true && c > 0 && s_fields.len() > 0 ) {
          local obj_sign = find_signal("is_signal", wt)
          local way_obj = s_fields[0].find_object(mo_way).get_desc()

          local build_cost = s_fields.len()*(obj_sign.get_cost()*2)
          build_cost += s_fields.len()*(way_obj.get_cost()*8)

          // terraform 4 fields
          build_cost = build_cost+(command_x.slope_get_price(82)*4)

          // count double ways
          build_cost = build_cost*c

          if ( build_cost/100 < (our_player.get_current_cash()+(our_player.get_current_maintenance()/100*5)) ) {
            build_double_ways = true
          } else {
            //gui.add_message_at(our_player, "calculate build cost double ways " + build_cost/100 + " < " + (our_player.get_current_cash()+(our_player.get_current_maintenance()/100*5)), world.get_time())
          }

        }

        //gui.add_message_at(our_player, "####### s_fields " + s_fields, world.get_time())
        if ( s_fields == true ) {
          cc += c
        } else if ( build_double_ways == true ) {
          //gui.add_message_at(our_player, "####### s_fields.len() " + s_fields.len(), world.get_time())
          local count_build = 0

          local build = false
          if ( s_fields.len() == c || s_fields.len() == c - 1 ) {
            if ( s_fields.len() == c - 1 ) {
              c--
            }


            // count station len start
            local station_len = 0
            for ( local h = 0; h < 50; h++ ) {
              if ( nexttile[h].find_object(mo_building) != null ) {
                station_len++
              } else {
                break
              }
            }
            station_len += 3
            if ( station_len < 8 ) {
              station_len = 8
            }

            for ( local i = 0; i < c; i++ ) {
              build = build_double_track(s_fields[i], wt_rail, station_len)
              if ( build ) {
                cc++
                count_build++
                build = false
              }
            }
          }

          if (count_build > 0 ) {
            local msgtext = format(translate("%s extends the route from %s (%s) to %s (%s)"), our_player.get_name(), start_l.get_halt().get_name(), coord_to_string(start_l), end_l.get_halt().get_name(), coord_to_string(end_l))
            gui.add_message_at(our_player, msgtext, start_l)
          }

        }

        if ( cc > 1 ) {
          line.double_ways_count = cc
          line.double_ways_build = 1
        } else {
          return
        }
      } else if (wt == wt_rail && (cnv_count > c || c == 0) ) {
        return null
      }

      //gui.add_message_at(our_player, " line 1384 " + line.get_name() + " - line.way_line_count : " + line.way_line_count + " - line.double_ways_count : " + line.double_ways_count, world.get_time())
      if ( wt == wt_rail ) {
        if ( line.way_line_count == 1 ) {
          cnv_count = line.double_ways_count + 1
        } else if ( line.way_line_count > 1 ) {
          if ( line.double_ways_count > 2 ) {
            cnv_count = line.double_ways_count - 1
          } else {
            cnv_count = line.double_ways_count
          }

        }
        //gui.add_message_at(our_player, " line set cnv_count " + cnv_count, world.get_time())
      }

      if ( bilanz_year < 0 ) {
        //gui.add_message_at(our_player, " line 1149 ", world.get_time())
        //gui.add_message_at(our_player, " gain_per_m " + gain_per_m, world.get_time())
        //::debug.pause()
      }

      // pak192.comic fix
      // line profit < 0 then not add cnv count > 3
      // wt_water
      local line_profit = line.get_profit()
      //local line_cnv_count = line.get_convoy_count()
      //if ( wt == wt_water ) gui.add_message_at(our_player, "#1390# test (line_profit[1] + line_profit[2]) " + (line_profit[1] + line_profit[2]), world.get_time())
      //if ( wt == wt_water ) gui.add_message_at(our_player, "#1390# test line.get_convoy_count() " + line_cnv_count[0], world.get_time())
      //if ( wt == wt_water ) gui.add_message_at(our_player, "#1390# test cnv_count " + cnv_count, world.get_time())
      if ( (line_profit[1] + line_profit[2]) < 0 && cnv_count >= 3 && wt == wt_water ) {
        //gui.add_message_at(our_player, "#1390# break add cnv ", world.get_time())
        line.next_vehicle_check = line.next_vehicle_check + (world.get_time().ticks_per_month * 2)
        return true
      }

      if ( (gain_per_m > 0 || bilanz_year < 0 ) && line.next_vehicle_check < world.get_time().ticks && line.add_convoy_time < world.get_time().ticks ) {
        // directly append
        // TODO put into report
        if ( bilanz_year < 0 ) {
          // gui.add_message_at(our_player, " line 1161 ", world.get_time())
          //::debug.pause()
        }
        local proto = cnv_proto_t.from_convoy(cnv, lf)

        if ( print_message_box == 1 ) {
          gui.add_message_at(our_player, "###---- check convoys line : " + line.get_name(), world.get_time())
        }

        // plan convoy prototype
        local freight = lf.get_name()
        local prototyper = prototyper_t(wt, freight)

        local proto_speed = cnv_max_speed - 30
        if ( proto_speed >= 100 ) { proto_speed = 60 }
        if ( proto_speed < 1 ) {
          prototyper.min_speed = 1
        } else {
          prototyper.min_speed = proto_speed
        }

        prototyper.max_vehicles = get_max_convoi_length(wt)

        local expand_station = []
        //local station_count = 0
        local station_exist = 0

        local expand_station_end_src = []
        local expand_station_end_dest = []

        if ( wt == wt_rail ) {
          for ( local i = 0; i < 6; i++ ) {
            //gui.add_message_at(our_player, "###---- nexttile[" + i + "].find_object(mo_building) : " + nexttile[i].find_object(mo_building), world.get_time())
            if ( nexttile[i].find_object(mo_building) != null ) {
              station_exist++
            }
          }
          expand_station = check_expand_station(nexttile, line)
          if ( print_message_box == 2 && expand_station.len() > 0 ) {
            gui.add_message_at(our_player, "#1262##---- station_exist : " + station_exist + " ## expand_station.len() : " + expand_station.len(), world.get_time())
          }

        }

        if ( expand_station.len() > 0 ) {
          prototyper.max_length = CARUNITS_PER_TILE * (station_exist + (expand_station.len()/2))
        } else if ( wt != wt_road && wt != wt_air && wt != wt_water ) {
          prototyper.max_length = CARUNITS_PER_TILE * station_exist
        } else {
          prototyper.max_length = CARUNITS_PER_TILE //prototyper.max_vehicles * 8
        }
          if ( print_message_box == 2 ) {
            //gui.add_message_at(our_player, "###---- station_exist : " + station_exist, world.get_time())
            //gui.add_message_at(our_player, "###---- prototyper.max_length : " + prototyper.max_length, world.get_time())
          }

        if ( wt == wt_road && check_good_quantity(start_l, end_l, lf, line) ) {
          line.next_vehicle_check = world.get_time().ticks + (road_line_check * 2)
          return true
        }

        local cnv_valuator = valuator_simple_t()
        cnv_valuator.wt = wt
        cnv_valuator.freight = freight

        local fd = link.f_dest
        local fs = link.f_src
        local freight_input = fd.input[freight].get_base_consumption()
        local freight_output = fs.output[freight].get_base_production()
        prototyper.volume = min(freight_input, freight_output)

        //cnv_valuator.volume = line.get_transported_goods().reduce(max)

        cnv_valuator.max_cnvs = 200
        // no signals and double tracks - limit 1 convoy for rail
        if (wt == wt_rail) {
          cnv_valuator.max_cnvs = cnv_count
        }

        local dist = 0
        if ( wt == wt_water || wt == wt_air ) {
          // through schedule to estimate distance
          local entries = cnv.get_schedule().entries
          dist = abs(entries[0].x - entries[1].x) + abs(entries[0].y - entries[1].y)
        } else {
          // distance real route
          dist = nexttile.len()
        }

        if (wt == wt_road ) {
          if ( dist <= 50) { cnv_valuator.max_cnvs = dist/3 }
          else if ( dist > 50 && dist <= 250 ) { cnv_valuator.max_cnvs = dist/2 }
          //else if ( dist > 250 ) { cnv_valuator.max_cnvs = dist - 50 }
          if ( print_message_box == 1 ) {
            gui.add_message_at(our_player, "### line : " + line.get_name() + " dist: " + dist + " cnv max: " + cnv_valuator.max_cnvs, world.get_time())
          }
        }

        // add 10% from distance
        //dist += dist / 100 * 10

        if ( wt == wt_rail ) {
          prototyper.electrified = check_catenary(line, dist, start_l, cnv_max_speed)
        }

        cnv_valuator.distance = dist

        local bound_valuator = valuator_simple_t.valuate_monthly_transport.bindenv(cnv_valuator)
        prototyper.valuate = bound_valuator

        if (prototyper.step().has_failed()) {
          if ( print_message_box == 1 ) {
            gui.add_message_at(our_player, "   ----> prototyper.step().has_failed() ", world.get_time())
          }
          return null
        }
        local proto = prototyper.best

        if ( print_message_box == 1 ) {
          gui.add_message_at(our_player, "   ----> proto : " + proto, world.get_time())
        }

        // build convoy
        local stations_list = line.get_schedule().entries
        local depot = null //cnv.get_home_depot()

        for (local i=0; i<stations_list.len(); i++) {
          local c = tile_x(stations_list[i].x, stations_list[i].y, stations_list[i].z)
          depot = search_depot(c, wt, (10+station_exist))
          if ( depot != null && depot != false ) {
            if ( print_message_box == 1 ) {
              gui.add_message_at(our_player, "####--> station " + coord_to_string(c), c)
              gui.add_message_at(our_player, "####--> depot " + depot, world.get_time())
            }
            break
          }
          if ( print_message_box == 1 ) {
              gui.add_message_at(our_player, "####--> not depot found", world.get_time())
          }

        }
        if ( depot == null || depot == false ) {
          //depot = cnv.get_home_depot()
          local c = cnv.get_home_depot()
          depot = tile_x(c.x, c.y, c.z)
          if ( depot.get_depot() == null ) {
            local way_tile = [null, null, null, null]
            way_tile[0] = tile_x(depot.x-1, depot.y, depot.z)
            way_tile[1] = tile_x(depot.x+1, depot.y, depot.z)
            way_tile[2] = tile_x(depot.x, depot.y-1, depot.z)
            way_tile[3] = tile_x(depot.x, depot.y+1, depot.z)
            for ( local i = 0; i < 4; i++ ) {
              if ( way_tile[i].find_object(mo_way) != null ) {
                local way_obj = find_object("way", wt, cnv.get_speed())
                local depot_obj = building_desc_x.get_available_stations(building_desc_x.depot, wt, {})
                local err = command_x.build_way(our_player, depot, way_tile[i], way_obj, true)
                if ( err == null ) {
                  err = command_x.build_depot(our_player, depot, depot_obj[0] )
                } else {
                  gui.add_message_at(our_player, "##-ERROR-##--> not depot found", depot)
                  return false
                }
                break
              }
            }

          }
        }

        local c = vehicle_constructor_t()

        c.p_depot  = depot_x(depot.x, depot.y, depot.z)
        c.p_line   = line
        c.p_convoy = proto
        local cnv_payload = c.p_convoy.capacity
        if ( wt == wt_road && cnv_payload < 30 ) {
          // by road add 3 vehicles
          c.p_count  = 3
        } else {
          c.p_count  = 1
          /*if ( int_li == cnv_count ) {
            c.p_count  = 0
          }*/
        }

        local int_li = 0
        if ( wt == wt_rail ) {
          local li = line.get_convoy_list()
          foreach(cnv in li) {
            if ( cnv.is_valid() ) { int_li++ }
          }

        }

        // input storage f_src = 0 or waiting good = 0 then not add convoys
        local entries = line.get_schedule().entries
        local start_h = entries[0].get_halt(our_player)
        local d = start_h.get_waiting()
        sleep()

        if ( (check_fsrc_input(link.f_src) || d[0] > 0) && int_li < cnv_count ) {
          append_child(c)
          if ( cnv_retired.len() == 1 && cnv_retired.len() == cnv_count ) {
            cnv_retired[0].toggle_withdraw(our_player)
          }

          dbgprint("==> build additional convoy")
          if ( print_message_box == 1 ) {
            gui.add_message_at(our_player, "####### cnv_count " + cnv_count, world.get_time())
            gui.add_message_at(our_player, "Line: " + line.get_name() + " ==> build additional convoy", world.get_time())
          }

          //if ( wt == wt_rail ) gui.add_message_at(our_player, "####### expand_station.len() " + expand_station.len(), world.get_time())
          if ( wt == wt_rail && expand_station.len() > 0 ) {
            // tiles for convoy
            local a = c.p_convoy.length
            local st_lenght = 0
            do {
              a -= 16
              st_lenght += 1
            } while(a > 0)

            // expand station
            local ret = build_expand_station(nexttile, expand_station, st_lenght, link.freight.get_name(), line)
            if ( ret ) {
              line.halt_length = st_lenght
              // set new line entries
              local entries = line.get_schedule().entries
              if ( entries.len() >= 2 ) {
                start_l = tile_x(entries[0].x, entries[0].y, entries[0].z)
                end_l = tile_x(entries[entries.len()-1].x, entries[entries.len()-1].y, entries[entries.len()-1].z)
              }
            }
          }

          if ( wt == wt_rail && proto.veh[0].needs_electrification() ) {
            build_catenary(start_l, end_l, depot, line)
          }

          local msgtext = format(translate("%s build additional convoy to line: %s"), our_player.get_name(), line.get_name())
          gui.add_message_at(our_player, msgtext, world.get_time())

          // save ticks for next check
          local road_car_rate = set_map_vehicles_counts(1)
          local next_time = road_line_check * 4
          if ( road_car_rate < 10 ) {
            next_time = road_line_check * 2
          }
          if ( wt == wt_road ) {
            line.next_vehicle_check = world.get_time().ticks + next_time
          } else {
            line.next_vehicle_check = world.get_time().ticks + (world.get_time().ticks_per_month * 2)
          }

          return true

        } else {
          line.next_vehicle_check = world.get_time().ticks + (world.get_time().ticks_per_month * 2)
        }
      }
    }
    /*
    local veh_count_line = line.get_convoy_count()
    local new_cnv_add_line = false
    if ( veh_count_line[0] > veh_count_line[1] ) {
      // not remove cnv from line by add cnv this month
      new_cnv_add_line = true
      //gui.add_message_at(our_player, "####### cnv new this month ", world.get_time())
    } //!new_cnv_add_line  &&
    */

    if ( !freight_available  &&  cnv_count>1  &&  2*cc_empty >= cnv_count  &&  cnv_empty_stopped && line.next_vehicle_check < world.get_time().ticks && !new_cnv_add_line ) {
      // freight, lots of empty and of stopped vehicles
      // -> something is blocked, maybe we block our own supply?
      // delete one convoy
      if ( cnv_empty_stopped.is_valid() ) {
        cnv_empty_stopped.destroy(our_player)
        dbgprint("==> destroy empty convoy")
        if ( print_message_box == 1 ) {
          gui.add_message_at(our_player, "####### cnv_count " + cnv_count, world.get_time())
          gui.add_message_at(our_player, "Line: " + line.get_name() + " ==> destroy empty convoy", world.get_time())
        }

        local msgtext = format(translate("%s removes convoys from line: %s"), our_player.get_name(), line.get_name())
        gui.add_message_at(our_player, msgtext, world.get_time())

        // save ticks for next check
        local road_car_rate = set_map_vehicles_counts(1)
        local next_time = road_line_check
        if ( road_car_rate < 10 ) {
          next_time = road_line_check / 2
        }
        if ( wt == wt_road ) {
          line.next_vehicle_check = world.get_time().ticks + next_time
        } else {
          line.next_vehicle_check = world.get_time().ticks + (world.get_time().ticks_per_month * 2)

          // set longer time for add convoy by remove convoy in last 6 month
          local cnv_line = line.get_convoy_count()
          local a = 0
          for ( local x = 1; x < 8; x++ ) {
            if ( cnv_line[x] <= cnv_line[0] ) {
              a++
            }
          }
          line.add_convoy_time = line.next_vehicle_check + (world.get_time().ticks_per_month * a)
          //gui.add_message_at(our_player, "#1511# line.add_convoy_time : " + line.add_convoy_time + " -- a : " + a, world.get_time())

        }
      }

    }
    dbgprint("")

    if ( wt == wt_road ) {
      line.next_vehicle_check = world.get_time().ticks + road_line_check
    } else {
      line.next_vehicle_check = world.get_time().ticks + world.get_time().ticks_per_month
    }

    return true

  }

  /**
   * Upgrade: plan a new convoy type with the prototyper, then
   * sell existing convoys, create new ones.
   */
  function upgrade_link_line(link, line, cnv_speed, route)
  {
    local print_message_box = 0


    if ( print_message_box > 0 ) {
      gui.add_message_at(our_player, "### upgrade_link_line " + line.get_name(), world.get_time())
    }
    //gui.add_message_at(our_player, "### cnv_speed " + cnv_speed, world.get_time())
    // find convoy
    local cnv = null
    {
      local list = line.get_convoy_list()
      if (list.get_count() == 0) {
        // no convois - strange
        return false
      }
      cnv = list[0]
    }
    // estimate transport volume
    local transported = line.get_transported_goods().reduce(max)

    // plan convoy prototype
    local wt = line.get_waytype()
    local prototyper = prototyper_t(wt, link.freight.get_name())

    // iterate through schedule to estimate distance
    local dist = 0
    /*if ( wt != wt_water && wt != wt_air ) {
      dist = route.len()
    } else */
    if ( route.len == 0 ) {
      local entries = line.get_schedule().entries
      local start_h = entries[0].get_halt(our_player)
      local end_h = entries[entries.len()-1].get_halt(our_player)
      local t = start_h.get_tile_list()
      start_l = tile_x(t[0].x, t[0].y, t[0].z)
      t = end_h.get_tile_list()
      end_l = tile_x(t[0].x, t[0].y, t[0].z)
      dist = abs(start_l.x - end_l.x) + abs(start_l.y - end_l.y)
    } else {
      dist = route.len()
      //dist = abs(route[0].x - route[route.len()-1].x) + abs(route[0].y - route[route.len()-1].y)
    }

    //local wt = wt
    // TODO do something smarter
    local proto_speed = cnv_speed - 25
    if ( proto_speed >= 100 ) { proto_speed = 60 }
    if ( proto_speed < 1 ) {
      prototyper.min_speed = 1
    } else {
      prototyper.min_speed = proto_speed
    }

    prototyper.max_vehicles = get_max_convoi_length(wt)
    prototyper.max_length = 1

    local expand_station = []
    if ( wt == wt_rail ) {
      expand_station = check_expand_station(route, line)
      if ( expand_station.len() > 0 ) {
        prototyper.max_length = CARUNITS_PER_TILE * (line.halt_length + (expand_station.len()/2))
      } else {
        prototyper.max_length = CARUNITS_PER_TILE * line.halt_length
      }
      if ( print_message_box > 0 ) {
        gui.add_message_at(our_player, "### line.halt_length = " + line.halt_length + " ## expand_station.len() : " + expand_station.len(), world.get_time())
        gui.add_message_at(our_player, "### prototyper.max_length = " + prototyper.max_length, world.get_time())
      }

      prototyper.electrified = check_catenary(line, dist, route[route.len()-1], cnv_speed)
    }
    if (wt == wt_water) {
      prototyper.max_length = 4
    }

    local cnv_valuator    = valuator_simple_t()
    cnv_valuator.wt       = wt
    cnv_valuator.freight  = link.freight.get_name()
    cnv_valuator.volume   = transported
    cnv_valuator.max_cnvs = 200
    // no signals and double tracks - limit 1 convoy for rail
    if (wt == wt_rail) {
      cnv_valuator.max_cnvs = line.double_ways_count + 1
    }
    cnv_valuator.distance = dist

    local bound_valuator = valuator_simple_t.valuate_monthly_transport.bindenv(cnv_valuator)
    prototyper.valuate = bound_valuator

    if (prototyper.step().has_failed()) {
      return false
    }

    local planned_convoy = prototyper.best
    // check whether different from current convoy
    local cnv_veh = cnv.get_vehicles()
    local pro_veh = planned_convoy.veh

    local different = cnv_veh.len() != pro_veh.len()
    for(local i=0; i<cnv_veh.len()  &&  !different; i++) {
      different = !cnv_veh[i].is_equal(pro_veh[i])
    }
    if (!different) {
      return false
    }

    dbgprint("Upgrade line "  + line.get_name())
    // build the new one, withdraw the old ones
    // directly append
    local depot  = cnv.get_home_depot()
    // TODO put into report
    local c = vehicle_constructor_t()
    c.p_depot    = depot_x(depot.x, depot.y, depot.z)
    c.p_line     = line
    c.p_convoy   = planned_convoy
    if ( wt == wt_rail || wt == wt_water ) {
      c.p_count    = min(planned_convoy.nr_convoys, 1)
    } else {
      c.p_count    = min(planned_convoy.nr_convoys, 3)
    }

    if ( wt == wt_rail && expand_station.len() > 0 ) {
      // tiles for convoy
      local a = c.p_convoy.length
      local st_lenght = 0
      do {
        a -= 16
        st_lenght += 1
      } while(a > 0)

      // expand station
      local ret = build_expand_station(route, expand_station, st_lenght, link.freight.get_name(), line)
      if ( ret ) {
        line.halt_length = st_lenght
      }
    }

    if ( wt == wt_rail && c.p_convoy.veh[0].needs_electrification() ) {
      build_catenary(route[0], route[route.len()-1], depot, line)
    }

    c.p_withdraw = true
    append_child(c)

    // save ticks for next check
    local road_car_rate = set_map_vehicles_counts(1)
    local next_time = road_line_check * 4
    if ( road_car_rate < 10 ) {
      next_time = road_line_check * 2
    }
    if ( wt == wt_road ) {
      line.next_vehicle_check = world.get_time().ticks + next_time
    } else {
      line.next_vehicle_check = world.get_time().ticks + (world.get_time().ticks_per_month * 2)
    }

    return true
  }

  // keys were broken for rotated maps, regenerate keys for all entries
  function repair_keys()
  {
    link_iterator = null
    local save_list = link_list
    link_list = {}

    foreach(link in save_list) {
      link_list[ key(link.f_src, link.f_dest, link.freight.get_name()) ] <- link
    }

  }

  function _save()
  {
    link_iterator = null // wont save
    return ::saveinstance("industry_manager_t", this)
  }

  function check_expand_station(nexttile, line) {
    // 0 = off
    // 1
    // 2 = station expand
    // 3
    local print_message_box = 0

    local wt = line.get_waytype()

    local expand_station = []
    local station_count = 0
    local station_exist = 0

    local expand_station_end_src = []
    local expand_station_end_dest = []

    // count exist station lenght
    for ( local i = 0; i < 6; i++ ) {
      if ( nexttile[i].find_object(mo_building) != null ) {
        station_count++
      }
    }
    if ( line.halt_length == 0 ) {
      line.halt_length = station_count
    }

    // check expand station -> tiles direction end from way
    local tiles_end_src = []
    local tiles_end_dest = []
    local t = null
    // line end dest
    for ( local i = 1; i < 4; i++ ) {
      t = check_tile_end_of_station(nexttile[0].get_way_dirs(wt), i, nexttile[0])
      if ( t != null ) {
        if ( print_message_box == 2 ) {
          gui.add_message_at(our_player, "###---- check tile expand_station_end_dest : " + coord3d_to_string(t), t)
        }
        expand_station_end_dest.append(t)
      } else {
        break
      }
    }
    // line end src
    for ( local i = 1; i < 4; i++ ) {
      t = check_tile_end_of_station(nexttile[nexttile.len()-1].get_way_dirs(wt), i, nexttile[nexttile.len()-1])
      if ( t != null ) {
        if ( print_message_box == 2 ) {
          gui.add_message_at(our_player, "###---- check tile expand_station_end_src : " + coord3d_to_string(t), t)
        }
        expand_station_end_src.append(t)
      } else {
        break
      }
    }

    //prototyper.max_length = station_count
    line.halt_length = station_count
    station_exist = station_count
    local add_tile_src = 0
    local add_tile_dest = 0
    if ( station_count < 6 ) {
      // check expand station -> direction of the existing route/way
      // built cnv to new length end expand station befor create cnv
      //station_count++
      for ( station_count; station_count < 6; station_count++ ) {
        //gui.add_message_at(our_player, "###---- nexttile[station_count-1] : " + coord3d_to_string(nexttile[station_count-1]) + " - " + nexttile[station_count-1].get_way_dirs(wt), nexttile[0])
        //gui.add_message_at(our_player, "###---- nexttile[station_count] : " + coord3d_to_string(nexttile[station_count]) + " - " + nexttile[station_count].get_way_dirs(wt), nexttile[0])
        //gui.add_message_at(our_player, "###---- nexttile[nexttile.len()-station_count-2] : " + coord3d_to_string(nexttile[nexttile.len()-station_count-1]) + " - " + nexttile[nexttile.len()-station_count-1].get_way_dirs(wt), nexttile[0])
        //gui.add_message_at(our_player, "###---- nexttile[nexttile.len()-station_count-1] : " + coord3d_to_string(nexttile[nexttile.len()-station_count]) + " - " + nexttile[nexttile.len()-station_count].get_way_dirs(wt), nexttile[0])
        /*if ( nexttile[station_count-1].get_way_dirs(wt) != nexttile[station_count].get_way_dirs(wt) ||
          nexttile[nexttile.len()-station_count-1].get_way_dirs(wt) != nexttile[nexttile.len()-station_count].get_way_dirs(wt) ||
          (nexttile[station_count].is_bridge() && nexttile[station_count].z == square_x(nexttile[station_count].x, nexttile[station_count].y).get_ground_tile().z) ||
          (nexttile[nexttile.len()-station_count-1].is_bridge() && nexttile[nexttile.len()-station_count-1].z == square_x(nexttile[station_count].x, nexttile[nexttile.len()-station_count-1].y).get_ground_tile().z) ) {
        */
        //if ( !test_field(our_player, nexttile[station_count], wt, nexttile[station_count].get_way_dirs(wt), square_x(nexttile[station_count].x, nexttile[station_count].y).get_ground_tile().z, 1) ) {
        local t1 = nexttile[station_count]
        local t2 = nexttile[(nexttile.len()-1)-station_count]

        local check_tile = 0
        local check_src = 0
        local check_dest = 0

        if ( t1.get_way_dirs(wt) == 5 || t1.get_way_dirs(wt) == 10 ) {
          if ( print_message_box == 2 ) {
                  gui.add_message_at(our_player, "###---- check tile direction : " + coord3d_to_string(t1), t1)
          }
          if ( (t1.z == nexttile[station_exist-1].z && !t1.is_bridge() && !t1.is_tunnel() && test_tile_is_empty(t1) && !t1.has_two_ways()) || (t1.z < nexttile[station_exist-1].z && t1.is_bridge()) ) { //t1.get_slope() == 0
            // dest
            if ( print_message_box == 2 ) {
              gui.add_message_at(our_player, "###---- check tile t1 z : " + t1.z, t1)
              gui.add_message_at(our_player, "###---- check tile nexttile[station_exist-1].z (" + coord3d_to_string(nexttile[station_exist-1]) + ") : " + nexttile[station_exist-1].z, nexttile[station_exist-1])
            }
            check_tile++
            check_src++
          } else {
            check_dest++
          }

        }

        if ( t2.get_way_dirs(wt) == 5 || t2.get_way_dirs(wt) == 10 ) {
          if ( print_message_box == 2 ) {
            gui.add_message_at(our_player, "###---- check tile direction : " + coord3d_to_string(t2), t2)
          }
          if ( (t2.z == nexttile[(nexttile.len()-1)-(station_exist-1)].z && !t2.is_bridge() && !t2.is_tunnel() && test_tile_is_empty(t2) && !t2.has_two_ways()) || (t2.z < nexttile[(nexttile.len()-1)-(station_exist-1)].z && t2.is_bridge()) ) { //t2.get_slope() == 0
            // src
            if ( print_message_box == 2 ) {
              gui.add_message_at(our_player, "###---- check tile t2 z : " + t2.z, t2)
              gui.add_message_at(our_player, "###---- check tile nexttile[(nexttile.len()-1)-(station_exist-1)].z (" + coord3d_to_string(nexttile[(nexttile.len()-1)-(station_exist-1)]) + ") : " + nexttile[(nexttile.len()-1)-(station_exist-1)].z, nexttile[(nexttile.len()-1)-(station_exist-1)])
            }
            check_tile++
            check_dest++
          } else {
            check_src++
          }
        }

        if ( print_message_box == 2 ) {
          gui.add_message_at(our_player, "###---- station_count-station_exist : " + (station_count-station_exist), world.get_time())
          gui.add_message_at(our_player, "###---- check_tile: " + check_tile + " check_dest: " + check_dest + " check_src: " + check_src, world.get_time())
          gui.add_message_at(our_player, "###---- add_tile_dest: " + add_tile_dest + " add_tile_src: " + add_tile_src, world.get_time())
        }


        if (check_tile == 2) {
          if ( print_message_box == 2 ) {
            gui.add_message_at(our_player, "###---- add tile nexttile[station_count]: " + coord3d_to_string(nexttile[station_count]), nexttile[station_count])
            gui.add_message_at(our_player, "###---- add tile nexttile[nexttile.len()-(station_count+1)]: " + coord3d_to_string(nexttile[nexttile.len()-(station_count+1)]), nexttile[nexttile.len()-(station_count+1)])
          }
          expand_station.append(nexttile[station_count])
          expand_station.append(nexttile[nexttile.len()-(station_count+1)])
        } else if (check_tile == 1 && check_dest >= 1 && expand_station_end_dest.len()-1 >= add_tile_dest) {
          // dest
          if ( print_message_box == 2 ) {
            gui.add_message_at(our_player, "###---- add tile expand_station_end_dest[add_tile_dest]: " + coord3d_to_string(expand_station_end_dest[add_tile_dest]), expand_station_end_dest[add_tile_dest])
            gui.add_message_at(our_player, "###---- add tile nexttile[nexttile.len()-(station_count+1)]: " + coord3d_to_string(nexttile[nexttile.len()-(station_count+1)]), nexttile[nexttile.len()-(station_count+1)])
          }
          expand_station.append(expand_station_end_dest[add_tile_dest])
          add_tile_dest++
          expand_station.append(nexttile[nexttile.len()-(station_count+1)])
        } else if (check_tile == 1 && check_src >= 1 && expand_station_end_src.len()-1 >= add_tile_src) {
          // src
          if ( print_message_box == 2 ) {
            gui.add_message_at(our_player, "###---- add tile nexttile[station_count]: " + coord3d_to_string(nexttile[station_count]), nexttile[station_count])
            gui.add_message_at(our_player, "###---- add tile expand_station_end_src[add_tile_src]: " + coord3d_to_string(expand_station_end_src[add_tile_src]), expand_station_end_src[add_tile_src])
          }
          expand_station.append(nexttile[station_count])
          expand_station.append(expand_station_end_src[add_tile_src])
          add_tile_src++
        } else if (check_tile == 0 && check_src <= 1 && check_dest <= 1 && expand_station_end_dest.len()-1 >= add_tile_dest && expand_station_end_src.len()-1 >= add_tile_src) {
          // src & dest
          if ( print_message_box == 2 ) {
            gui.add_message_at(our_player, "###---- add tile expand_station_end_dest[add_tile_dest]: " + coord3d_to_string(expand_station_end_dest[add_tile_dest]), expand_station_end_dest[add_tile_dest])
            gui.add_message_at(our_player, "###---- add tile expand_station_end_src[add_tile_src]: " + coord3d_to_string(expand_station_end_src[add_tile_src]), expand_station_end_src[add_tile_src])
          }
          expand_station.append(expand_station_end_dest[add_tile_dest])
          add_tile_dest++
          expand_station.append(expand_station_end_src[add_tile_src])
          add_tile_src++
        } else {
          station_count--
          break
        }

              /*
              } else {
                station_count--
                break
              }*/
      }

      if ( station_exist < station_count && print_message_box == 2 ) {
        gui.add_message_at(our_player, "###---- check stations field : " + station_exist, nexttile[0])
        gui.add_message_at(our_player, "###---- check stations field expand -> direction exist way : " + station_count, nexttile[0])
      }

    }

    return expand_station
  }

  function build_expand_station(nexttile, expand_station, st_lenght, freight, line) {

    // 0 = off
    // 1 = station expand
    // 2
    // 3
    local print_message_box = 0

    local wt = line.get_waytype()

    local start_l = nexttile[nexttile.len()-1]
    local end_l = nexttile[0]

    local station_exist = 0
    // count exist station lenght
    for ( local i = 0; i < 6; i++ ) {
      if ( nexttile[i].find_object(mo_building) != null ) {
        station_exist++
      }
    }

    local station_list = building_desc_x.get_available_stations(building_desc_x.station, wt, good_desc_x(freight))

    local station_s_obj = nexttile[nexttile.len()-1].find_object(mo_building).get_desc()
    if ( !station_s_obj.is_available(world.get_time()) ) {
      local capacity = station_s_obj.get_capacity()
      station_s_obj = null
      foreach(station in station_list) {
        if ( station_s_obj == null ) {
          station_s_obj = station
        }
        if ( station.get_capacity() >= capacity && station.get_capacity() < station_s_obj.get_capacity() ) {
          station_s_obj = station
        }
      }
    }

    local station_e_obj = nexttile[0].find_object(mo_building).get_desc()
    if ( !station_e_obj.is_available(world.get_time()) ) {
      station_e_obj = null
      foreach(station in station_list) {
        if ( station_e_obj == null ) {
          station_e_obj = station
        }
        if ( station.cost < station_e_obj.cost ) {
          station_e_obj = station
        }
      }
    }

    local way_obj = start_l.find_object(mo_way).get_desc() //way_list[0]
    if ( !way_obj.is_available(world.get_time()) ) {
      way_obj = find_object("way", wt, way_obj.get_topspeed())
    }

    local catenary_obj = null //find_object("catenary", wt, 100)
    local build_catenary = false
    if ( start_l.find_object(mo_wayobj) != null ) {
      catenary_obj = start_l.find_object(mo_wayobj).get_desc()
      if ( catenary_obj != null && !catenary_obj.is_available(world.get_time()) && !catenary_obj.is_overhead_line() ) {
        catenary_obj = find_object("catenary", wt, way_obj.get_topspeed())
      }
    } else {
      //catenary_obj = find_object("catenary", wt, way_obj.get_topspeed())
    }

    if ( print_message_box == 1 ) {
      gui.add_message_at(our_player, "####### st_lenght > station_count : " + st_lenght + " > " + station_exist, expand_station[0])
    }

    if ( st_lenght > station_exist ) {
      // expand station
      local sched = schedule_x(wt_rail, [])
      local sched_start = start_l
      local sched_end = end_l

      local b_count = st_lenght - station_exist
      switch(b_count) {
        case 2:
          b_count++
          break
        case 3:
          b_count += 2
          break
        case 4:
          b_count += 3
          break
        case 5:
          b_count += 4
          break
      }


      for ( local i = 0; i <= b_count; i++ ) {
        local station_obj = (i % 2) ? station_s_obj : station_e_obj
        if ( print_message_box == 1 ) {
          gui.add_message_at(our_player, "####### [i] " + i, expand_station[i])
        }

        if ( expand_station[i].find_object(mo_way) == null ) {
          local build_tile = (i % 2) ? start_l : end_l
          if ( terraform_tile(expand_station[i], build_tile.z) ) {
            command_x.build_way(our_player, build_tile, expand_station[i], way_obj, true)
            command_x.build_station(our_player, expand_station[i], station_obj)
            if ( catenary_obj != null ) {
              command_x.build_wayobj(our_player, build_tile, expand_station[i], catenary_obj)
            }

            local entries = line.get_schedule().entries
            if ( build_tile == start_l ) {
              sched_start = expand_station[i]
              if ( print_message_box == 1 ) {
                gui.add_message_at(our_player, "####### expand stations tile schedule start_l  ", expand_station[i])
              }
            } else {
              sched_end = expand_station[i]
              if ( print_message_box == 1 ) {
                gui.add_message_at(our_player, "####### expand stations tile schedule end_l  ", expand_station[i])
              }
            }
            //::debug.pause()

          }
          //::debug.pause()
        } else {
          command_x.build_station(our_player, expand_station[i], station_obj)
        }
        if ( print_message_box == 1 ) {
          gui.add_message_at(our_player, "####### expand stations tile " + coord3d_to_string(expand_station[i]), expand_station[i])
        }
      }

      sched.entries.append( schedule_entry_x(sched_start, 100, 0) );
      sched.entries.append( schedule_entry_x(sched_end, 0, 0) );
      line.change_schedule(our_player, sched)

      //line.halt_length = st_lenght
      if ( print_message_box == 1 ) {
        gui.add_message_at(our_player, "####### expand stations ", expand_station[0])
      }
      local message_text = format(translate("%s expand the station %s (%s) and station %s (%s)"), our_player.get_name(), start_l.get_halt().get_name(), coord_to_string(start_l), end_l.get_halt().get_name(), coord_to_string(end_l))
      gui.add_message_at(our_player, message_text, start_l)
      //::debug.pause()

      return true
    }

    return false

  }

  function check_catenary(line, dist, start_l, cnv_max_speed) {
    local catenary_obj = null //find_object("catenary", wt, 100)
    local catenary_cost = 0
    local int_return = 0
    local wt = line.get_waytype()

    local print_message_box = 0

    if ( start_l.find_object(mo_wayobj) != null ) {
      catenary_obj = start_l.find_object(mo_wayobj).get_desc()
      if ( catenary_obj != null && !catenary_obj.is_available(world.get_time()) && !catenary_obj.is_overhead_line() ) {
        catenary_obj = find_object("catenary", wt, cnv_max_speed)
      }

      if ( print_message_box == 1 ) {
        gui.add_message_at(our_player, "###---- set electrified convoys " + line.get_name(), world.get_time())
        gui.add_message_at(our_player, "###---- line has catenary ", world.get_time())
      }

      int_return = 1
    } else {
      catenary_obj = find_object("catenary", wt, cnv_max_speed)

      // build cost
      catenary_cost = (catenary_obj.get_cost() * (dist + (8 * line.double_ways_count))) + catenary_obj.get_cost()
      // + maintenance for 5 months
      catenary_cost += ((catenary_obj.get_maintenance() * (dist + (8 * line.double_ways_count))) + catenary_obj.get_maintenance()) * 5

      // set electrified for vehicle by way len > xx tiles
      if ( dist > 80 && world.get_time().year >= 1935 && our_player.get_current_cash() > catenary_cost ) {
        if ( print_message_box == 1 ) {
          gui.add_message_at(our_player, "###---- set electrified convoys " + line.get_name(), world.get_time())
          gui.add_message_at(our_player, "###---- line len " + nexttile.len(), world.get_time())
        }
        int_return = 1
      }
    }

    return int_return

  }

  function build_catenary(start_l, end_l, depot, line) {
    local print_message_box = 0
    local wt = line.get_waytype()

    local way_obj = start_l.find_object(mo_way).get_desc() //way_list[0]
    if ( !way_obj.is_available(world.get_time()) ) {
      way_obj = find_object("way", wt, way_obj.get_topspeed())
    }

    local catenary_obj = null //find_object("catenary", wt, 100)
    local build_catenary = false
    if ( start_l.find_object(mo_wayobj) != null ) {
      catenary_obj = start_l.find_object(mo_wayobj).get_desc()
      if ( catenary_obj != null && !catenary_obj.is_available(world.get_time()) && !catenary_obj.is_overhead_line() ) {
        catenary_obj = find_object("catenary", wt, way_obj.get_topspeed())
        build_catenary = true
      }
    } else {
      catenary_obj = find_object("catenary", wt, way_obj.get_topspeed())
      build_catenary = true
    }

    if ( build_catenary ) {
      command_x.build_wayobj(our_player, start_l, end_l, catenary_obj)
      command_x.build_wayobj(our_player, end_l, start_l, catenary_obj)
      command_x.build_wayobj(our_player, depot, start_l, catenary_obj)

      local msgtext = format(translate("%s electrified the line %s"), our_player.get_name(), line.get_name())
      gui.add_message_at(our_player, msgtext, world.get_time())

    }


    if ( print_message_box == 1 ) {
      //gui.add_message_at(our_player, "####### proto.veh[0].needs_electrification() : " + proto.veh[0].needs_electrification(), world.get_time())
      gui.add_message_at(our_player, "####### cnv needs electrified " + catenary_obj.get_name(), depot)
    }

    return build_catenary
  }

}

function convoy_max_speed(cnv) {
  // convoy_x::calc_max_speed ( integer power, integer weight, integer speed_limit )

  local pwr = 0
  local weight = 0
  local m_speed = 500

  local veh_list = cnv.get_vehicles()

  for (local i = 0; i < veh_list.len(); i++) {
    pwr += veh_list[i].get_power()
    weight += veh_list[i].get_weight() + (veh_list[i].get_capacity()*veh_list[i].get_freight().get_weight_per_unit())
    if ( m_speed > veh_list[i].get_topspeed() ) {
      m_speed = veh_list[i].get_topspeed()
    }
  }

  local a = cnv.calc_max_speed(pwr, weight, m_speed)
  return a
}

function convoy_is_retired(cnv) {

  local veh_list = cnv.get_vehicles()

  for (local i = 0; i < veh_list.len(); i++) {
    if ( veh_list[i].is_retired(world.get_time()) ) {
      return true
    }
  }

}

/*
 * check factory input + src station good quantity > max storage factory
 *
 *
 */
function check_good_quantity(start_l, end_l, good, line) {

  local f_dest  = end_l.get_halt().get_factory_list()
  if ( f_dest.len() == 1 ) {
    local freight = good.get_name()
    foreach(freight, islot in f_dest[0].input) {
      if ( good.get_name() == islot.good ) {
        local st = islot.get_storage()
        local it = islot.get_in_transit()
        local good_src = start_l.get_halt().get_freight_to_halt(good, end_l.get_halt())
        local max_storage = islot.max_storage

        //gui.add_message_at(our_player, "*** good halt src " + good_src + " " + line.get_name(), world.get_time())
        //gui.add_message_at(our_player, "*** islot.good " + translate(islot.good) + " factory " + f_dest[0].get_name() + " input storage [" + max_storage + "]", world.get_time())

        if ( st[0] < (max_storage/10) ) {
          return false
        } else if ( (st[0] + it[0] > max_storage && (max_storage/5) < st[0]) || it[0] > max_storage ) {
          //
          //gui.add_message_at(our_player, "*** good quantity [" + (st[0] + it[0]) + "] > factory " + f_dest[0].get_name() + " input storage [" + max_storage + "] " + line.get_name(), world.get_time())
          return true

        }
      }
    }
            // todo more goods check

  }

  return false
}
