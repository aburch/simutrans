/** @file api_skeleton.cc Documents the necessary functions to be implemented of a scenario script */

// It is in a C++ file to be processed by Doxygen filters to get function typemasks right


/**
 * This function is called when the scenario starts. Do all the initializations here,
 * as you cannot initialize global variables with non-built-in squirrel types.
 * @typemask void()
 */
register_function("start");

/**
 * This function is called when a savegame with active scenario is loaded.
 * Do all the initializations and post-processing here.
 * @typemask void()
 */
register_function("resume_game");

/**
 * Text shown in the 'About' tab in the scenario info window.
 *
 * There is a default implementation, which returns concatenation
 * of #scenario author, version, short_description.
 * Html-like tags can be used, see @ref get_rule_text.
 *
 * @param pl player number of active player
 * @typemask string(integer)
 */
register_function("get_about_text");

/**
 * Text shown in the 'Rules' tab in the scenario info window.
 *
 * Text can contain several html-like tags:
 * - \<p\>, \<br\>: line break
 * - \<h1\>, \<em\>, \<it\>, \<st\>: text between start and end tag will be colored. It's a matter of taste, of course.
 * - \<a href="..."\>: insert hyper link, text between start and end tag will be colored blue.
 *       - link to another tab of scenario info window: href="tabname", where tabname is one of: info, goal, rules, result, about
 *       - link to position on the map: href="(x,y)", click on link will jump to the map position
 *
 * @code
 * <h1>Here is an example.</h1>
 * <br>
 * Do not build anything at the position <a href='(47,11)'>near Cologne</a>.
 * The mayor of <a href='(8,15)'>Berlin</a> seems to be frustrated with your airport building capabilities.
 * <br>
 * Your results can be found in the <a href='result'>results</a> tab.
 * @endcode
 *
 * @param pl player number of active player
 * @typemask string(integer)
 */
register_function("get_rule_text");

/**
 * Text shown in the 'Goal' tab in the scenario info window.
 *
 * Html-like tags can be used, see @ref get_rule_text.
 *
 * @param pl player number of active player
 * @typemask string(integer)
 */
register_function("get_goal_text");

/**
 * Text shown in the 'Info' tab in the scenario info window.
 *
 * Html-like tags can be used, see @ref get_rule_text.
 *
 * @param pl player number of active player
 * @typemask string(integer)
 */
register_function("get_info_text");

/**
 * Text shown in the 'Result' tab in the scenario info window.
 *
 * Html-like tags can be used, see @ref get_rule_text.
 *
 * @param pl player number of active player
 * @typemask string(integer)
 */
register_function("get_result_text");

/**
 * Core function of a scenario: It returns the completion percentage for the
 * specified player.
 *
 * If it returns a negative value the respective player has lost. This state will not be changed
 * even if the function later returns positive values again.
 *
 * If it returns a value >= 100 then the respective player has won. This state will not be changed
 * even if the function later returns lower values again.
 *
 * @param pl player number of active player
 * @typemask integer(integer)
 */
register_function("is_scenario_completed");


/**
 * Function is called when toolbars are filled with icons or when a tool should be activated.
 *
 * There is a default implementation, which forbids map-editing tools.
 *
 * @attention Results are not transfered over network, use the functions provided in #rules in this case.
 *
 * @param pl player number
 * @param tool_id see @ref tool_ids
 * @param wt waytype of tool
 * @returns true if tool is allowed.
 * @typemask bool(integer,integer,way_types)
 */
register_function("is_tool_allowed");

/**
 * Called when user clicks on a tile to build or remove something.
 *
 * This function is network-aware:
 * Error messages are sent back over network to clients.
 * @attention Does not work with waybuilding and all tools that need path-finding, use the functions provided in #rules in this case.
 *
 * @param pl player number
 * @param tool_id see @ref tool_ids
 * @param pos coordinate
 *
 * @return null if allowed, an error message otherwise
 * @typemask string(integer,integer,coord3d)
 */
register_function("is_work_allowed_here");
