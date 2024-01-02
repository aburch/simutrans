/**
 * Classes for basic nodes in the tree, which represents our brain :)
 */

// return codes from step
// enum return_code {
const RT_DONE_NOTHING    = 1  // Done nothing.
const RT_PARTIAL_SUCCESS = 2  // Done something, want to continue next
const RT_SUCCESS         = 4  // Done something.
const RT_ERROR           = 8  // Some error occured.
const RT_KILL_ME         = 16 // Can be destroyed by parent.
const RT_TOTAL_SUCCESS   = 20 // RT_SUCCESS | RT_KILL_ME
const RT_TOTAL_FAIL      = 24 // RT_ERROR | RT_KILL_ME
const RT_READY           = 29 // RT_DONE_NOTHING | RT_SUCCESS | RT_ERROR | RT_KILL_ME
// }

/**
 * Class use as return value from the function step()
 * Signals success, contains additional data.
 */
class r_t
{
  // return code composed of return_code constants above
  code = 0
  // reports have to be handled
  report = null
  // run node immediately
  node = null

  constructor(c, r=null, n=null) { code = c; report = r; node = n }

  function is_ready()       { return (code & RT_READY);}
  function can_be_deleted() { return (code & RT_KILL_ME); }
  function has_failed()      { return (code & RT_ERROR);}
}

/**
 * Basic node class.
 */
class node_t {
  name = null
  debug = false

  /**
   * Main method of these nodes. Called from the global ::step() method.
   */
  function step() { return r_t(RT_TOTAL_FAIL)}

  constructor(n = "node_t")
  {
    name = n
  }
  function dbgprint(t)
  {
    if (debug) print(name + ": " + t)
  }
  function _save()
  {
    return ::saveinstance(name, this)
  }

}

/**
 * Has array of nodes that will be executed / stepped sequentially.
 */
class node_seq_t extends node_t
{
  nodes = null
  next_to_step = 0

  constructor(n = "node_seq_t")
  {
    base.constructor(n)
    nodes = []
  }

  function append_child(c) {
    nodes.append(c)
  }

  /// @returns value of type r_t
  function step()
  {
    if (nodes.len() == 0) return r_t(RT_SUCCESS);

    if (next_to_step >= nodes.len()) {
      next_to_step = 0;
      return r_t(RT_DONE_NOTHING);
    }

    local ret = nodes[next_to_step].step()
    if ( ret == null ) {
      gui.add_message_at(our_player, "####### nodes[next_to_step].step() is null; nodes.len() = " + nodes.len() + " # next_to_step " + next_to_step, world.get_time())
      return r_t(RT_ERROR);
    }
    // take nodes report
    if (ret.report) {
      append_report(ret.report)
    }
    // delete child if ready
    if (ret.can_be_deleted()) {
      nodes.remove(next_to_step);
    }
    else if (ret.is_ready()) {
      next_to_step ++
    }
    // successor node
    if (ret.node) {
      append_child(ret.node)
    }
    // our return code
    local rc = RT_PARTIAL_SUCCESS // want next call, too
    if (ret.has_failed()) {
      rc = RT_ERROR // propagate error
    }
    else if (next_to_step > nodes.len()) {
      rc = RT_SUCCESS // done with stepping all nodes

      build_check_month = world.get_time() + world.get_time().ticks_per_month
    }
    return r_t(rc)
  }
}

/**
 * Base class to report financial aspects of plans.
 */
class report_t
{
  // tree that will be executed
  action = null
  // costs
  cost_fix = 0
  cost_monthly = 0
  // expected gain per month
  gain_per_m = 0
  //
  distance = 0
  points = 0
  // retire plan
  retire_time = null
  // retire objects
  retire_obj = null

  function merge_report(r)
  {
    if (r.action) {
      action.append_child( r.action )
    }
    cost_fix     += r.cost_fix
    cost_monthly += r.cost_monthly
    gain_per_m   += r.gain_per_m
    distance     += r.distance
    points       += r.points
  }

  function _save()
  {
    return ::saveinstance("report_t", this)
  }
}

/**
 * Manager class.
 * Steps its child nodes, otherwise does some work(), generates reports.
 */
class manager_t extends node_seq_t
{
  reports = null

  constructor(n = "manager_t")
  {
    base.constructor(n)
    reports = []
  }

  function step()
  {

    month_check_message()

    if ( build_check_month > world.get_time().ticks ) {
      // not plan link
      //if (debug) gui.add_message_at(our_player, " not plan link : build_check_month = " + build_check_month, world.get_time())
      //if (debug) gui.add_message_at(our_player, " for " + freight + " from " + fsrc.get_name() + " at " + fsrc.x + "," + fsrc.y + " to "+ fdest.get_name() + " at " + fdest.x + "," + fdest.y, world.get_time())

      //return RT_DONE_NOTHING
    }


      dbgprint("stepping a child")
      local r = base.step()
      if (r.has_failed()) {
        if ("error_handler" in this) {
          return error_handler()
        }
      }
      if (r.code == RT_DONE_NOTHING  ||  r.code == RT_SUCCESS) {
        // all nodes were stepped
        dbgprint("doing some work")
        return work()
      }
      dbgprint("stepped a child")
      return r

  }

  function work()
  {
    /*if ( month_count ) {
        month_count = false
        month_count_ticks = world.get_time().next_month_ticks

        if ( our_player.is_valid() && our_player.get_type() == 4 ) {
          local operating_profit = player_x(our_player.nr).get_operating_profit()
          local net_wealth = player_x(our_player.nr).get_net_wealth()
          local message_text = format(translate("%s - last month: operating profit %d net wealth %d"), our_player.get_name(), operating_profit[1], net_wealth[1])
          gui.add_message_at(our_player, message_text, world.get_time())

          if ( operating_profit[1] < 0 ) {
            build_check_month = world.get_time().month + 1
            if ( build_check_month > 11 ) { build_check_month = build_check_month - 11 }
          }

          local yt = world.get_time().year.tostring()

          // check all 5 years ( year xxx0 and xxx5 )
          if ( world.get_time().month == 3 ) { #(yt.slice(-1) == "0" || yt.slice(-1) == "5") &&
            // in april
            industry_manager.check_pl_lines()
          }

          // check all 5 years ( year xxx2 and xxx7 )
          if ( (yt.slice(-1) == "2" || yt.slice(-1) == "7") && world.get_time().month == 4 ) {
            // in may
            // check unused halts
            check_stations_connections()
          }

        }

    } else if ( world.get_time().ticks > month_count_ticks ) {
      month_count = true
    }

    set_map_vehicles_counts()*/
    month_check_message()
  }

  function append_report(r) { reports.append(r); }

  /**
   * Returns the best report available.
   */
  function get_report()
  {
    if (reports.len() == 0) return null
    // find report that maximizes gain_per_m / cost_fix
    local i = -1
    local best = null

    // [0] = cost_fix / 13
    // [1] = gain_per_m * ( cost_fix / 13 ) * points
    local p_test = [0, 0]
    local p_best = [0, 0]

    for(local j=0; j<reports.len(); j++) {
      local test = reports[j]
      // check account balance
      if (!is_cash_available(test.cost_fix + test.cost_monthly) ) {
        // too expensive
        continue
      }

      if ( test.points <= 30 ) {
        continue
      }

      // calculate evaluation points
      if ( best != null ) {

        p_test[0] = test.cost_fix / 15
        p_best[0] = best.cost_fix / 15

        p_test[1] = test.gain_per_m * p_best[0] * test.points
        p_best[1] = best.gain_per_m * p_test[0] * best.points
      }

      if ( best == null
        || ( p_best[1] < p_test[1] )
        || (test.cost_fix == 0  &&  best.cost_fix == 0  &&   best.gain_per_m < test.gain_per_m) )
      {
        best = test
        i = j
      }
    }

    if (best) {
      reports.remove(i)
    }
    return best
  }
}

/*
 * monthly message profit and net wealth
 *
 */
function month_check_message() {

    if ( month_count ) {
        month_count = false
        month_count_ticks = world.get_time().next_month_ticks

        if ( our_player.is_valid() && our_player.get_type() == 4 ) {
          local operating_profit = player_x(our_player.nr).get_operating_profit()
          local net_wealth = player_x(our_player.nr).get_net_wealth()
          local message_text = format(translate("%s - last month: operating profit %d net wealth %d"), our_player.get_name(), operating_profit[1], net_wealth[1])
          gui.add_message_at(our_player, message_text, world.get_time())
          //gui.add_message_at(our_player, "### get_current_net_wealth() " + (player_x(our_player.nr).get_current_net_wealth()/100), world.get_time())
          //gui.add_message_at(our_player, "### get_current_cash() " + player_x(our_player.nr).get_current_cash(), world.get_time())
          //gui.add_message_at(our_player, "### net_wealth - cash " + ((player_x(our_player.nr).get_current_net_wealth()/100) - player_x(our_player.nr).get_current_cash()), world.get_time())
          //gui.add_message_at(our_player, "### world.get_time().month " + world.get_time().month, world.get_time())

          if ( operating_profit[1] < 0 ) {
            build_check_month = world.get_time().ticks + world.get_time().ticks_per_month
          }

          local yt = world.get_time().year.tostring()

          // check all 5 years ( year xxx0 and xxx5 )
          if ( world.get_time().month == 3 ) { #(yt.slice(-1) == "0" || yt.slice(-1) == "5") &&
            // in april
            industry_manager.check_pl_lines()
          }

          // check all 5 years ( year xxx2 and xxx7 )
          if ( (yt.slice(-1) == "2" || yt.slice(-1) == "7") && world.get_time().month == 4 ) {
            // in may
            // check unused halts
            check_stations_connections()
          }

        }

    } else if ( world.get_time().ticks > month_count_ticks ) {
      month_count = true
    }

    set_map_vehicles_counts()
}

/*
 *
 *
 */
function build_check_time(build_cost) {
  local operating_profit = player_x(our_player.nr).get_operating_profit()
  local net_wealth = player_x(our_player.nr).get_current_net_wealth()/100
  local cash = player_x(our_player.nr).get_current_cash()
  local maintenance = player_x(our_player.nr).get_current_maintenance()/100

  local k = build_cost

  if ( cash > 0 ) { k -= cash + (net_wealth / 2) }

  //gui.add_message_at(our_player, "### k -= cash " + k, world.get_time())

  local t = 0
  if ( operating_profit[1] > 0 ) {
    t = abs(k / operating_profit[1]).tointeger()
  } else {
    t = 12
  }

  //gui.add_message_at(our_player, "### build_cost " + build_cost, world.get_time())
  //gui.add_message_at(our_player, "### operating_profit[1] " + operating_profit[1], world.get_time())
  //gui.add_message_at(our_player, "### build_check_time(build_cost) " + t, world.get_time())

  if ( t > 24 ) {
    t = 24
  }

  return t
}
