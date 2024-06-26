/**
 * Main file of the AI player.
 */

// TODO obey construction speed setting
// TODO check allowed transport types

// some meta data - unused
ai <- {}
ai.short_description <- "AI player implementation road/ship/rail"

ai.author <- "dwachs/Andarix"
ai.version <- "0.9.5"

// includes
include("basic")  // .. definition of basic node classes
include("astar")  // .. route search for way building etc
include("save")   // .. routines to save class instances

include("factorysearcher")              // .. checks factories for available connections
include("industry_connection_planner")  // .. plans connection between 2 factories
include("combined_connections")         // .. plans connections using water + land transport
include("industry_manager")             // .. manages existing connection (buys, sells, upgrades convoys)
include("placefinder")                  // .. utility functions to find places for stations near factories etc
include("prototyper")                   // .. plans convoy-type for a connection
include("road_connector")               // .. builds road connection
include("rail_connector")               // .. builds rail connection
include("ship_connector")               // .. creates ship connection
include("station_manager")              // .. keeps information about freight station
include("vehicle_constructor")          // .. constructs convoy, assign to line, start

// basic functions
sum <- @(a,b) a+b
function abs(x) { return x>0 ? x : -x }

// global variables
our_player_nr <- -1
our_player    <- null // player_x instance
// for single run functions in month
month_count   <- false
month_count_ticks <- world.get_time().next_month_ticks
// build check for new lines
build_check_month <- world.get_time().ticks
// set factory strategie - 0 = traditional method, taken from C++ implementation
factory_strategie <- 0

/*
 *  different ticks per month for bits_per_month
 *
 *  bits_per_month = 16   ->   world.get_time().ticks_per_month = 65.536
 *  bits_per_month = 17   ->   world.get_time().ticks_per_month = 131.072
 *  bits_per_month = 18   ->   world.get_time().ticks_per_month = 262.144
 *  bits_per_month = 19   ->   world.get_time().ticks_per_month = 524.288
 *  bits_per_month = 20   ->   world.get_time().ticks_per_month = 1.048.576
 *  bits_per_month = 21   ->   world.get_time().ticks_per_month = 2.097.152
 *  bits_per_month = 22   ->   world.get_time().ticks_per_month = 4.194.304
 *  bits_per_month = 23   ->   world.get_time().ticks_per_month = 8.388.608
 *  bits_per_month = 24   ->   world.get_time().ticks_per_month = 16.777.216
 *
 *  set parameter for road lines check
 *
 */
road_line_check <- 500000


// convoy and citycar count
convoy_count <- null
road_convoy_count <- 0
citycar_count <- 0

// the AI is organized as a tree,
// all the work is done in the nodes of the tree
tree <- {}

// nodes with particular jobs
factorysearcher  <- null
industry_manager <- null
station_manager  <- null

// stepping info
s <- {}
s._step <- 0
s._next_construction_step <- 0

// the table 'persistent' will be saved in the savegame
persistent.s <- s

// 2.. 14 = 13 names
possible_names <- ["Petersil Cars", "Teumer Alp Dream Trucks", "Runk & Strunk Transports", "A. Nach B. Transports", "Interflug Fourwheelers",
  "Pfarnest International", "Suboptimal Transports", "Conveyor Belts & Braces", "Bucket Brigade Inc.",
  "Maggikraut AG", "Bugs & Eggs Unlimited", "S. Claus & R. Deer Worldwide", "Leiterwagn & Sons"
      ]

/**
 * Start-routine. Will be called when AI is initialized.
 * Parameter: the number of the AI player.
 */
function start(pl_nr)
{
  init(pl_nr)
  // set a funny name

  if (our_player_nr > 1  &&  our_player_nr-2 < possible_names.len()) {
    player_x(our_player_nr).set_name( possible_names[our_player_nr-2]);
  }

  gui.add_message_at(our_player, "### script version " + ai.version, world.get_time())
  print("Act as player no " + our_player_nr + " under the name " + our_player.get_name())

  local player_sai_count = 0
  for ( local i = 2; i < 16; i++ ) {
    if ( player_x(i).is_valid() ) {
      if ( player_x(i).get_type() == 4 ) {
        player_sai_count++
      }
    }
  }
  //build_check_month += abs(player_count / our_player_nr)
  build_check_month += (abs(player_sai_count / 2) * world.get_time().ticks_per_month)
  if ( ( player_sai_count % 2 ) == 0 ) {
    factory_strategie = 1
    build_check_month -= world.get_time().ticks_per_month
  } else {
    factory_strategie = 0
  }
  gui.add_message_at(our_player, "### player_sai_count " + player_sai_count + " #  build_check_month " + build_check_month + " # factory_strategie " + factory_strategie, world.get_time())
}

/**
 * Initialize the tree with basic nodes.
 */
function init_tree()
{
  if (factorysearcher == null) {
    factorysearcher = factorysearcher_t()
  }
  if (industry_manager == null) {
    industry_manager = industry_manager_t()
  }
  if (!("tree" in persistent)) {
    tree = manager_t()
    tree.append_child(factorysearcher)
    tree.append_child(industry_manager)
    persistent.tree <- tree
  }
  else {
    if (persistent.tree.getclass() != manager_t) {
      // upgrade
      tree = manager_t()
      foreach(i in ["nodes", "next_to_step"]) {
        tree[i] = persistent.tree[i]
      }
    }
    else {
      tree = persistent.tree
    }
  }

  if (!("station_manager" in persistent)) {
    persistent.station_manager <- freight_station_manager_t(1)
  }
}

/**
 * Called after savegame is loaded.
 */
function resume_game(pl_nr)
{
  init(pl_nr)

  s = persistent.s
}

function init(pl_nr)
{
  debug.set_pause_on_error(true)

  our_player_nr = pl_nr
  our_player    = player_x(our_player_nr)

  srand(our_player_nr * 4711)

  annotate_classes() // sets class name as attribute for all known classes (save.nut)

  init_tree()
}

/**
 * The heart beat of the player.
 * If the routine will take too much time, execution is suspended and later resumed.
 * This should be completely transparent to the script.
 * Then the main program is still responsive.
 */
function step()
{
  if ( build_check_month <= world.get_time().ticks ) {

    tree.step()
    s._step++

    // connector node may fail, but may offer alternative connector node
    local report = tree.get_report()
    if (report) {
      factorysearcher.append_report(report)
    }

    if (s._step > s._next_construction_step) {
      local r = factorysearcher.get_report()

      if (r   &&  r.action) {
    /*
      gui.add_message_at(our_player, "####### r.retire_obj " + r.retire_obj, world.get_time())
      gui.add_message_at(our_player, "####### world.get_time().raw " + world.get_time().raw, world.get_time())
    */
        if ( r.retire_time != null && r.retire_time < world.get_time().ticks ) {
          //gui.add_message_at(our_player, "####### report out of time ", world.get_time())
          //gui.add_message_at(our_player, "####### r.retire_time " + r.retire_time, world.get_time())
          //gui.add_message_at(our_player, "####### world.get_time().ticks " + world.get_time().ticks, world.get_time())
          return
        }
        else if ( r.retire_obj != null && r.retire_obj <= world.get_time().raw ) {
          //gui.add_message_at(our_player, "####### object out of time ", world.get_time())
          return
        }
        else {
          print("New report: expected construction cost: " + (r.cost_fix / 100.0))
          //gui.add_message_at(our_player, "New report: expected construction cost: " + (r.cost_fix / 100.0), world.get_time())
          tree.append_child(r.action)
        }
      }

      s._next_construction_step += 1 + (s._step % 3)
    }

  }
  else {
    //gui.add_message_at(our_player, " ai step() break : build_check_month = " + build_check_month, world.get_time())

    month_check_message()

    sleep()
  }
}

/**
 * Helper routine: translate 3d-coordinate to string.
 * This can be used as key in tables.
 */
function coord3d_to_key(c)
{
  return ("coord3d_" + c.x + "_" + c.y + "_" + c.z).toalnum();
}

function coord_to_key(c)
{
  return ("coord_" + c.x + "_" + c.y).toalnum();
}

function equal_coord3d(a,b)
{
  return a.x == b.x  &&  a.y == b.y  &&  a.z == b.z
}


function is_cash_available(cost /* in 1/100 cr */)
{
  //gui.add_message_at(our_player, " ***** cash : " + our_player.get_current_cash(), world.get_time())
  //gui.add_message_at(our_player, " ***** cost : " + cost, world.get_time())
  return cost + 2*our_player.get_current_maintenance() < our_player.get_current_cash()*100
}

/**
 * Called to save into savegame.
 * Returns string that will be saved.
 * Here: we turn the persistent table into a string using recursive_save (from script/script_base.nut).
 */
function save()
{
  local str = ""
  local tic = get_ops_total()
  local rem = get_ops_remaining()

  str = "persistent = " + recursive_save(persistent, "\t", [ persistent ] )

  local toc = get_ops_total()
  print("save used " + (toc-tic) + " ops, remaining = " + rem)

  return str
}

/**
 * Returns random number rand with 0 <= rand < upper
 */
function myrand(upper)
{
  if (upper <= 1) {
    return upper-1
  }
  local rem = (RAND_MAX % upper) + 1
  local r
  do {
    r = rand()
  } while (r > RAND_MAX - rem)
  return r % upper
}

/**
 * Returns ticks for today + @p m months
 */
function today_plus_months(m)
{
  local time = world.get_time()
  return time.ticks + m * time.ticks_per_month
}

/**
 * returns pakset name (lower case)
 *
 *
 */
function get_set_name()
{
  local pakset = get_pakset_name()  // full string from ground.outside.pak
  if ( pakset != null ) {
    local s = pakset.find(" ")
    pakset = pakset.slice(0, s)
    pakset = pakset.tolower()
  } else {
    pakset = "unknown"
  }

  return pakset
}

/*
 * set global variables citicar and convoy counts
 *
 * output = 0 -> no return
 * output = 1 -> return road_car_rate
 */
function set_map_vehicles_counts(output = 0) {
  sleep()
  local citycar_array = world.get_year_citycars()
  local convoy_array = world.get_convoys()
  convoy_count = convoy_array[0]
  citycar_count = citycar_array[0]

  road_convoy_count = 0
  local list = world.get_convoy_list()
  foreach(cnv in list) {
    if ( cnv.is_valid() && cnv.get_waytype() == wt_road ) {
      road_convoy_count++
    }
  }
  /*
  gui.add_message_at(our_player, "####### citycars " + citycar_count, world.get_time())
  gui.add_message_at(our_player, "####### convoys " + convoy_count, world.get_time())
  gui.add_message_at(our_player, "####### road convoys " + road_convoy_count, world.get_time())
  */

  if ( output == 1 ) {
    // citycars and road-cnv on map - rate per tile
    //local citycar_count = world.get_year_citycars()
    local map_size_tiles = world.get_size().x * world.get_size().y
    local map_citizens = world.get_citizens()
    local road_car_rate = (map_citizens[0]/10)/ max((citycar_count + road_convoy_count), 1)
    return road_car_rate
  }

}
