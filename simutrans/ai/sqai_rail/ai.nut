/**
 * Main file of the AI player.
 */

// TODO obey construction speed setting
// TODO check allowed transport types

// some meta data - unused
ai <- {}
ai.short_description <- "AI player implementation road/ship/rail"

ai.author <-"dwachs/Andarix"
ai.version <- "0.8.4"

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

	print("Act as player no " + our_player_nr + " under the name " + our_player.get_name())
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
			print("New report: expected construction cost: " + (r.cost_fix / 100.0))
			tree.append_child(r.action)
		} else {
			if ( r && r.action && r.retire_time < world.get_time() ) {
				gui.add_message_at(our_player, "####### report out of time ", world.get_time())
				return r_t(RT_TOTAL_FAIL)
			}
			if ( r && r.action && r.retire_obj <= world.get_time().raw ) {
					gui.add_message_at(our_player, "####### object out of time ", world.get_time())
					return r_t(RT_TOTAL_FAIL)
			}
		}
		s._next_construction_step += 1 + (s._step % 3)

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
	local s = pakset.find(" ")
	pakset = pakset.slice(0, s)
	pakset = pakset.tolower()

	return pakset
}