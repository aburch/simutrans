/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_scenario.cc exports scenario related functions. */

#include "../api_class.h"
#include "../api_function.h"
#include "../../dataobj/koord.h"
#include "../../dataobj/koord3d.h"
#include "../../dataobj/scenario.h"
#include "../../dataobj/translator.h"
#include "../../utils/simstring.h"

#include "../../simintr.h"

using namespace script_api;

static char buf[40];


static plainstring double_to_string(double f, sint32 decimals)
{
	number_to_string(buf, f, decimals);
	return buf;
}

static plainstring integer_to_string(sint64 f)
{
	number_to_string(buf, f, 0);
	return buf;
}

static plainstring money_to_string_intern(sint64 m)
{
	money_to_string(buf, m, false);

	return buf;
}

static plainstring difftick_to_string_intern(sint32 f )
{
	return difftick_to_string( f, false );
}

static plainstring koord_to_string_intern(koord k)
{
	return k.get_str();
}

static plainstring koord3d_to_string_intern(koord3d k)
{
	return k.get_str();
}

void export_string_methods(HSQUIRRELVM vm)
{
	/**
	 * Helper method to translate strings.
	 * @param text string to translate
	 * @returns translation of given string
	 */
	// need to identify the correct overloaded function
	register_method<const char* (*)(const char*)>(vm, &translator::translate, "translate");

	/**
	 * Pretty-print floating point numbers, use language specific separator for powers of thousands.
	 * @param value number to print
	 * @param decimals how many decimals should be printed
	 * @returns a nice string
	 */
	register_method(vm, &double_to_string, "double_to_string");

	/**
	 * Pretty-print integers, use language specific separator for powers of thousands.
	 * @param value number to print
	 * @returns a nice string
	 */
	register_method(vm, &integer_to_string, "integer_to_string");

	/**
	 * Pretty-print money values, use language specific separator for powers of thousands.
	 * @param value number to print
	 * @returns a nice string with trailing dollar-sign
	 */
	register_method(vm, &money_to_string_intern, "money_to_string");

	/**
	 * Print coordinates, does automatic rotation.
	 * @param c coordinate
	 * @returns a nice string
	 */
	register_method(vm, &koord_to_string_intern, "coord_to_string");

	/**
	 * Print coordinates, does automatic rotation.
	 * @param c coordinate
	 * @returns a nice string
	 */
	register_method(vm, &koord3d_to_string_intern, "coord3d_to_string");

	/**
	 * Get name of given month.
	 * @param month number between 0 (january) and 11 (december)
	 * @returns month name in language of server
	 */
	register_method(vm, &translator::get_month_name, "get_month_name");

	/**
	 * Translates difference of ticks to string.
	 */
	register_method(vm, &difftick_to_string_intern, "difftick_to_string");
}


void export_scenario(HSQUIRRELVM vm)
{
	/**
	 * Helper method to load scenario-related translation files.
	 * Tries to load files in the following order relative to pakxx/scenario:
	 * -# scenario-name/iso/filename
	 * -# scenario-name/en/filename
	 * -# scenario-name/filename
	 *
	 * Here, iso refers to iso-abbreviation of currently active language.
	 *
	 * The content of the files is cached. The cache is cleared upon reloading of savegame.
	 * @param file name of txt-file
	 * @return content of loaded file
	 * @note Only available in scenario mode.
	 * @ingroup scen_only
	 */
	register_method(vm, &scenario_t::load_language_file, "load_language_file");

	/**
	 * Table with methods to forbid and allow tools.
	 *
	 * Tools that are set to forbidden using the forbid_* methods can be allowed
	 * again by calls to the respective allow_* method with exact the same parameters.
	 *
	 * @note Only available in scenario mode.
	 * @ingroup scen_only
	 */
	begin_class(vm, "rules", 0);

	/**
	 * Forbid tool.
	 *
	 * @param player_nr number of player this rule applies to,
	 *                  if this is set to player_all then this acts for all players except public player
	 * @param tool_id id of tool
	 * @see tool_ids way_types player_all
	 */
	STATIC register_method(vm, &scenario_t::forbid_tool, "forbid_tool");

	/**
	 * Allow tool.
	 *
	 * @param player_nr number of player this rule applies to,
	 *                  if this is set to player_all then this acts for all players except public player
	 * @param tool_id id of tool
	 * @see tool_ids way_types player_all
	 */
	STATIC register_method(vm, &scenario_t::allow_tool,  "allow_tool");

	/**
	 * Forbid tool with certain waytype.
	 *
	 * @param player_nr number of player this rule applies to,
	 *                  if this is set to player_all then this acts for all players except public player
	 * @param tool_id id of tool
	 * @param wt waytype
	 * @see tool_ids way_types player_all
	 */
	STATIC register_method(vm, &scenario_t::forbid_way_tool, "forbid_way_tool");

	/**
	 * Allow tool with certain waytype.
	 *
	 * @param player_nr number of player this rule applies to,
	 *                  if this is set to player_all then this acts for all players except public player
	 * @param tool_id id of tool
	 * @param wt waytype
	 * @see tool_ids way_types player_all
	 */
	STATIC register_method(vm, &scenario_t::allow_way_tool,  "allow_way_tool");

	/**
	 * Forbid tool with certain waytype within rectangular region on the map.
	 *
	 * @param player_nr number of player this rule applies to,
	 *                  if this is set to player_all then this acts for all players except public player
	 * @param tool_id id of tool
	 * @param wt waytype
	 * @param pos_nw coordinate of north-western corner of rectangle
	 * @param pos_se coordinate of south-eastern corner of rectangle
	 * @param err error message presented to user when trying to apply this tool, see also @ref is_work_allowed_here
	 * @see tool_ids way_types player_all
	 */
	STATIC register_method(vm, &scenario_t::forbid_way_tool_rect, "forbid_way_tool_rect");

	/**
	 * Allow tool with certain waytype within rectangular region on the map.
	 *
	 * @param player_nr number of player this rule applies to,
	 *                  if this is set to player_all then this acts for all players except public player
	 * @param tool_id id of tool
	 * @param wt waytype
	 * @param pos_nw coordinate of north-western corner of rectangle
	 * @param pos_se coordinate of south-eastern corner of rectangle
	 * @see tool_ids way_types player_all
	 */
	STATIC register_method(vm, &scenario_t::allow_way_tool_rect,  "allow_way_tool_rect");

	/**
	 * Forbid tool with certain waytype within cubic region on the map.
	 *
	 * @param player_nr number of player this rule applies to,
	 *                  if this is set to player_all then this acts for all players except public player
	 * @param tool_id id of tool
	 * @param wt waytype
	 * @param pos_nw 3d-coordinate of north-western corner of cube
	 * @param pos_se 3d-coordinate of south-eastern corner of cube
	 * @param err error message presented to user when trying to apply this tool, see also @ref is_work_allowed_here
	 * @see tool_ids way_types player_all
	 */
	STATIC register_method(vm, &scenario_t::forbid_way_tool_cube, "forbid_way_tool_cube");

	/**
	 * Allow tool with certain waytype within cubic region on the map.
	 *
	 * @param player_nr number of player this rule applies to,
	 *                  if this is set to player_all then this acts for all players except public player
	 * @param tool_id id of tool
	 * @param wt waytype
	 * @param pos_nw 3d-coordinate of north-western corner of cube
	 * @param pos_se 3d-coordinate of south-eastern corner of cube
	 * @see tool_ids way_types player_all
	 */
	STATIC register_method(vm, &scenario_t::allow_way_tool_cube,  "allow_way_tool_cube");

	/**
	 * Clear all forbidding rules, effectively allowing all tools again that were forbidden using functions of the table @ref rules.
	 *
	 * Only effects tools forbidden by rules::forbid_tool, rules::forbid_way_tool, rules::forbid_way_tool_cube, rules::forbid_way_tool_rect.
	 * The result of ::is_tool_allowed and ::is_work_allowed_here is not influenced.
	 */
	STATIC register_method(vm, &scenario_t::clear_rules,  "clear");

	/**
	 * Signals that toolbars and active tools need to be checked against scenario rules again.
	 */
	STATIC register_method(vm, &scenario_t::gui_needs_update,  "gui_needs_update");

	end_class(vm);


	/**
	 * Table with methods help debugging.
	 * @note Only available in scenario mode.
	 */
	begin_class(vm, "debug", 0);

	/**
	 * @returns text containing all active rules, can be used in @ref get_debug_text
	 * @note Only available in scenario mode.
	 */
	STATIC register_method(vm, &scenario_t::get_forbidden_text,  "get_forbidden_text");

	end_class(vm);
}
