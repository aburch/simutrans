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
scenario.translation <- ""
scenario.api <- "112.3"

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
 * Results are not transferred over network, use the rules.forbid_* functions in this case
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
	+ "<br><br><em>Translation:</em> " +  scenario.translation
}

function get_rule_text(pl) { return "Do what you want." }
function get_goal_text(pl) { return "The way is the target." }
function get_info_text(pl) { return "Random scenario." }
function get_result_text(pl) { return "You are owned." }
function get_debug_text(pl)  { return debug.get_forbidden_text() }

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
