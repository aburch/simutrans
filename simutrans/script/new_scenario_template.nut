/**
 * Template file for scenarios
 * it contains a minimal interface
 */



/// specify the savegame to load
map.file = "savegame.sve"

/// short description to be shown in finance window
/// and in standard implementation of get_about_text
scenario.short_description = "Template Scenario"

scenario.author = "User"
scenario.version = "0.0"


/**
 * These functions should return text strings (or ttext instances)
 * the text is shown in the corresponding tab in the scenario info window
 * all text can be dynamic and change with progress in scenario
 *
 * These information can be customized per player.
 * Players are identified by their numbers: pl == 0 is the first player slot,
 * pl == 1 is the 'Public player'
 */
function get_info_text(pl)
{
	// General information
}
function get_rule_text(pl)
{
	// Rules to be obeyed
}
function get_result_text(pl)
{
	// Display current standing
}
function get_goal_text(pl)
{
	// Describe the goals to be achieved to win
}

// Commented out: Will use implementation of scenario_base.nut,
// which needs scenario.short_description, scenario.author, scenario.version
//
// function get_about_text(pl)
// {
//	// Information about author, version, etc
// }

// Commented out: Will use implementation of scenario_base.nut,
// which needs scenario.short_description
//
// function get_short_description(pl)
// {
//	// Short string will be displayed in finance window
// }

/**
 * Called upon start of scenario, initialize global variables here
 */
function start()
{
}

/**
 * Main function: returns percentage of completion
 * If returned value >= 100 then scenario is won
 * If less than zero scenarion is lost
 */
function is_scenario_completed(pl)
{
	return 100 // complete
}

/**
 * Called after loading a savegame of a played scenario
 */
function resume_game()
{
}

/**
 * Table that contains data that will be saved in savegame
 * only plain data is saved: no classes / instances / functions, no cyclic references
 */
persistent = {}


// Attention: do not call API functions here in global scope.
// If you do so, they will be called before the savegame is loaded,
// and result in undefined behavior.


/**
 * Called when user clicks to build.
 * Error messages are sent back over network to clients.
 * Does not work with waybuilding, use the rules.forbid_* functions in this case.
 *
 * @param pos is a table with coordinate { x=, y=, z=}
 * @return null if allowed, an error message otherwise
 */
function is_work_allowed_here(pl, tool_id, pos)
{
	return null
}
