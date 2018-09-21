/**
 * Base file for scenarios
 */

// declare arrays and slots
map <- {}
map.file <- ""

map.settings <- {}

// some meta info
scenario <- {}
scenario.short_description <- "This is a random scripted scenario"

scenario.author <- ""
scenario.version <- ""

// table to hold routines for gui access
gui <- {}

// table to hold routines for forbidding/allowing player tools
rules <- {}

// table containing all waytypes
all_waytypes <- [wt_road, wt_rail, wt_water, wt_monorail, wt_maglev, wt_tram, wt_narrowgauge, wt_air, wt_power]


// placeholder array for all the map editing tools
map.editing_tools <- [ tool_add_city, tool_change_city_size, tool_land_chain, tool_city_chain,
                       tool_build_factory, tool_link_factory, tool_lock_game, tool_build_cityroad,
		       tool_increase_industry, tool_step_year, tool_fill_trees, tool_set_traffic_level,
		       dialog_edit_factory, dialog_edit_attraction, dialog_edit_house, dialog_edit_tree, dialog_enlarge_map]

// forbidden tools
// default: map editing tools, switch player
scenario.forbidden_tools <- [tool_switch_player]
foreach(tool_id in map.editing_tools) {
	scenario.forbidden_tools.append(tool_id)
}

/**
 * Called when filling toolbars, activating tools
 * Results are not transfered over network, use the rules.forbid_* functions in this case
 *
 * @return 1 if allowed, null otherwise
 */
function is_tool_allowed(pl, tool_id, wt)
{
	if (pl == 1) return true
	return scenario.forbidden_tools.find( tool_id )==null; // null => not found => allowed
}

/**
 * Called when user clicks to build etc.
 * Error messages are sent back over network to clients.
 * Does not work with waybuilding etc use the rules.forbid_* functions in this case.
 *
 * @param pos is a table with coordinate { x=, y=, z=}
 * @return null if allowed, an error message otherwise
 */
function is_work_allowed_here(pl, tool_id, pos)
{
	return null
}


function is_schedule_allowed(pl, schedule)
{
	return null
}


// declare getter functions
function get_map_file()
{
	return map.file;
}
function get_short_description(pl)
{
	return scenario.short_description;
}
function get_api_version()
{
	return ("api" in scenario) ? scenario.api : "112.3"
}
function get_about_text(pl)
{
	return "<em>Scenario:</em>  " +  scenario.short_description
	+ "<br><br><em>Author:</em> " +  scenario.author
	+ "<br><br><em>Version:</em> " +  scenario.version
}

function get_rule_text(pl) { return "Do what you want." }
function get_goal_text(pl) { return "The way is the target." }
function get_info_text(pl) { return "Random scenario." }
function get_result_text(pl) { return "You are owned." }

function min(a, b) { return a < b ? a : b }
function max(a, b) { return a > b ? a : b }

/**
 * initialization, called when scenario starts
 */
function start()
{
}

/**
 * initialization, called when savegame is loaded
 * default behavior: calls start()
 */
function resume_game()
{
	start()
}

/**
 * Happy New Month and Year!
 */
function new_month()
{
}
function new_year()
{
}

/**
 * load / save support
 * the persistent table will be written / restored during save / load
 * only plain data is saved: no classes / instances / functions, no cyclic references
 */
persistent <- {}

/**
 * writes the persistent table to a string
 */
function save()
{
	if (typeof(persistent) != "table") {
		throw("error during saving: the variable persistent is not a table")
	}
	local str = "persistent = " + recursive_save(persistent, "\t", [ persistent ] )

	return str
}

function recursive_save(table, indent, table_stack)
{
	local isarray = typeof(table) == "array"
	local str = (isarray ? "[" : "{") + "\n"
	foreach(key, val in table) {
		str += indent
		if (!isarray) {
			if (typeof(key)=="string") {
				str += key + " = "
			}
			else {
				str += "[" + key + "] = "
			}
		}
		while( typeof(val) == "weakref" )
			val = val.ref

		switch( typeof(val) ) {
			case "null":
				str += "null"
				break
			case "integer":
			case "float":
			case "bool":
				str += val
				break
			case "string":
				str += "@\"" + val + "\""
				break
			case "array":
			case "table":
				if (!table_stack.find(val)) {
						table_stack.push( table )
						str += recursive_save(val, indent + "\t", table_stack )
						table_stack.pop()
				}
				else {
					// cyclic reference - good luck with resolving
					str += "null"
				}
				break
			case "instance":
				if ("_save" in val) {
					str += val._save()
					break
				}
			default:
				str += "\"unknown\""
		}
		if (str.slice(-1) != "\n") {
			str += ",\n"
		}
		else {
			str = str.slice(0,-1) + ",\n"
		}

	}
	str += indent.slice(0,-1) + (isarray ? "]" : "}") + "\n"
	return str
}


/////////////////////////////////////
/**
 * translatable texts
 */
class ttext {
	text_raw = "" // raw text
	text_tra = "" // translated text
	// tables with information to replace variables {key} by their values
	var_strings = null
	var_values  = null
	// these cannot be initialized here to {}, otherwise
	// all instances will refer to the same table!

	constructor(k) {
		if (type(k) != "string") {
			throw("ttext must be initialized with string")
		}
		text_raw = k
		text_tra = k
		var_strings = []
		var_values  = {}
		find_variables(text_raw)
	}

	function get_translated_text()
	{
		return translate(text_raw)
	}

	function find_variables(k) {
		var_strings = []
		// find variable tags
		local reg = regexp(@"\{([a-zA-Z_]+[a-zA-Z_0-9]*)\}");
		local start = 0;
		while (start < k.len()) {
			local res = reg.capture(k, start)
			if (res) {
				local vname = k.slice(res[1].begin, res[1].end)
				var_strings.append({ name = vname, begin = res[0].begin, end = res[0].end })
				start = res[0].end
			}
			else {
				start = k.len();
			}
		}
	}

	function _set(key, val)
	{
		var_values.rawset(key, val);
	}

	function _tostring()
	{
		// get translated text
		local text_tra_ = get_translated_text()
		if (text_tra_ != text_tra) {
			text_tra = text_tra_
			// new text, search for identifiers
			find_variables( text_tra )
		}
		// create array of values, sort it per index in string
		local values = []
		foreach(val in var_strings) {
			local key = val.name
			if ( key in var_values ) {
				local valx = { begin = val.begin, end = val.end, value = var_values.rawget(key) }
				values.append(valx)
			}
		}
		values.sort( @(a,b) a.begin <=> b.begin )
		// create the resulting text piece by piece
		local res = ""
		local ind = 0
		foreach( e in values) {
			res += text_tra.slice(ind, e.begin)
			res += e.value
			ind = e.end
		}
		res += text_tra.slice(ind)
		return res
	}
}

/**
 * text from files
 */
class ttextfile extends ttext {

	file = null

	constructor(file_) {
		file = file_
		base.constructor("")
	}

	function get_translated_text()
	{
		return load_language_file(file)
	}


}

/////////////////////////////////////

function _extend_get(index) {
	if (index == "rawin"  ||  index == "rawget") {
		throw null // invoke default delegate
		return
	}
	local fname = "get_" + index
	if (rawin(fname)) {
		local func = rawget(fname)
		if (typeof(func)=="function") {
			return func.call(this)
		}
	}
	throw null // invoke default delegate
}

/**
 * this class implements an extended get method:
 * everytime an index is not found it tries to call the method 'get_'+index
 */
class extend_get {

	_get = _extend_get

}

/**
 * class that contains data to get access to an in-game factory
 */
class factory_x extends extend_get {
	/// coordinates
	x = -1
	y = -1

	/// input / output slots, will be filled by constructor
	input = {}
	output = {}

	/// constructor will be overwritten by a native method
	constructor(x_, y_) {
		x = x_
		y = y_
	}
}


/**
 * class that contains data to get access to an production slot of a factory
 */
class factory_production_x extends extend_get {
	/// coordinates of factory
	x = -1
	y = -1
	good = ""  /// name of the good to be consumed / produced
	index = -1 /// index to identify an io slot
	max_storage = 0  /// max storage of this slot

	constructor(x_, y_, n_, i_) {
		x = x_
		y = y_
		good  = n_
		index = i_
	}
}


/**
 * class to provide access to the game's list of all factories
 */
class factory_list_x {

	/// meta-method to be called in a foreach loop
	function _nexti(prev_index) {
	}

	/// meta method to retrieve factory by index in the global C++ array
	function _get(index) {
	}
}


/**
 * class that contains data to get access to an in-game player company
 */
class player_x extends extend_get {
	nr = 0 /// player number

	constructor(n_) {
		nr = n_
	}
}


/**
 * class that contains data to get access to an in-game halt
 */
class halt_x extends extend_get {
	id = 0 /// halthandle_t

	constructor(i_) {
		id = i_
	}
}


/**
 * class that contains data to get access to a line of convoys
 */
class line_x extends extend_get {
	id = 0 /// linehandle_t

	constructor(i_) {
		id = i_
	}
}

/**
 * class to provide access to line lists
 */
class line_list_x {

	halt_id = 0
	player_id = 0
}

/**
 * class that contains data to get access to a tile (grund_t)
 */
class tile_x extends extend_get {
	/// coordinates
	x = -1
	y = -1
	z = -1

	constructor(x_, y_, z_) {
		x = x_
		y = y_
		z = z_
	}

	function get_objects()
	{
		return tile_object_list_x(x,y,z)
	}
}

class tile_object_list_x {
	/// coordinates
	x = -1
	y = -1
	z = -1

	constructor(x_, y_, z_) {
		x = x_
		y = y_
		z = z_
	}
}

/**
 * class that contains data to get access to a grid square (planquadrat_t)
 */
class square_x extends extend_get {
	/// coordinates
	x = -1
	y = -1

	constructor(x_, y_) {
		x = x_
		y = y_
	}
}


/**
 * class to provide access to convoy lists
 */
class convoy_list_x {

	use_world = 0
	halt_id = 0
	line_id = 0
}


/**
 * class that contains data to get access to an in-game convoy
 */
class convoy_x extends extend_get {
	id = 0 /// convoihandle_t

	constructor(i_) {
		id = i_
	}
}


/**
 * class to provide access to the game's list of all cities
 */
class city_list_x {

	/// meta-method to be called in a foreach loop
	function _nexti(prev_index) {
	}

	/// meta method to retrieve city by index in the global C++ array
	function _get(index) {
	}
}


/**
 * class that contains data to get access to a city
 */
class city_x extends extend_get {
	/// coordinates
	x = -1
	y = -1

	constructor(x_, y_) {
		x = x_
		y = y_
	}
}

/**
 * class to access in-game settings
 */
class settings {
}

/**
 * base class of map objects (ding_t)
 */
class map_object_x extends extend_get {
	/// coordinates
	x = -1
	y = -1
	z = -1
}

class schedule_x {
	/// waytype
	waytype = 0
	/// the entries
	entries = null

	constructor(w, e)
	{
		waytype = w
		entries = e
	}
}

class schedule_entry_x {
	/// coordinate
	x = -1
	y = -1
	z = -1
	/// load percentage
	load = 0
	/// waiting
	wait = 0

	constructor(pos, l, w)
	{
		x = pos.x
		y = pos.y
		z = pos.z
		load = l
		wait = w
	}
}

class dir {
	static none           = 0
	static north          = 1
	static east           = 2
	static northeast      = 3
	static south          = 4
	static northsouth     = 5
	static southeast      = 6
	static northsoutheast = 7
	static west           = 8
	static northwest      = 9
	static eastwest       = 10
	static northeastwest  = 11
	static southwest      = 12
	static northsouthwest = 13
	static southeastwest  = 14
	static all            = 15

	static nsew = [1, 4, 2, 8]
}

class time_x {
	raw = 1
	year = 0
	month = 1
}

class time_ticks_x extends time_x {
	ticks = 0
	ticks_per_month = 0
	next_month_ticks = 0
}

class coord {
	x = -1
	y = -1
}

class coord3d extends coord {
	z = -1
}

/**
 * The same metamethod magic as in the class extend_get.
 */
table_with_extend_get <- {

	_get = _extend_get

}

/**
 * table to hold routines to access the world
 */
world <- {}
world.setdelegate(table_with_extend_get)
