#include "api.h"

/** @file api_scenario.cc exports scenario related functions. */

#include "../api_class.h"
#include "../api_function.h"
#include "../../dataobj/scenario.h"
#include "../../dataobj/translator.h"

using namespace script_api;

#define begin_class(c,p) push_class(vm, c);
#define end_class() sq_pop(vm,1);
#define STATIC

void export_scenario(HSQUIRRELVM vm)
{
	/**
	 * Helper method to translate strings.
	 * @param text string to translate
	 * @returns translation of given string
	 */
	// need to identify the correct overloaded function
	register_method<const char* (*)(const char*)>(vm, &translator::translate, "translate");
	/**
	 * Helper method to load scenario-related translation files.
	 * Tries to load files in the following order relative to pakxx/scenario:
	 * -# scenario-name/iso/filename
	 * -# scenario-name/en/filename
	 * -# scenario-name/filename
	 *
	 * Here, iso refers to iso-abbreviation of currently active language.
	 * @param file name of txt-file
	 * @return content of loaded file
	 */
	register_method(vm, &scenario_t::load_language_file, "load_language_file");

	/**
	 * Table with methods to forbid and allow tools.
	 *
	 * Tools that are set to forbidden using the forbid_* methods can be allowed
	 * again by calls to the respective allow_* method with exact the same parameters.
	 */
	begin_class("rules", 0);

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
	 * @param err error message presented to user when trying to apply this tool
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

	end_class();
}
