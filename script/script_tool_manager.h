/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SCRIPT_SCRIPT_TOOL_MANAGER_H
#define SCRIPT_SCRIPT_TOOL_MANAGER_H


#include "../tpl/vector_tpl.h"

class exec_script_base_t;
struct scripted_tool_info_t;
class tool_t;
class tool_selector_t;
class tool_exec_script_t;
class tool_exec_two_click_script_t;

/**
 * There's no need to construct an instance since everything is static here.
 */
class script_tool_manager_t
{
private:
	/// All one-click script tools
	static vector_tpl<tool_exec_script_t*> one_click_script_tools;
	/// All two-click script tools
	static vector_tpl<tool_exec_two_click_script_t*> two_click_script_tools;

public:

	/// Looks for tool.nut in path
	static bool check_file(char const* path);

	/**
	 * Reads description.tab at path.
	 * If no description is found, use default values.
	 * @returns filled info structure.
	 */
	static const scripted_tool_info_t* get_script_info(const char* path);

	/**
	 * Loads tool from path.
	 * If @p tool is not NULL it is reinitialized with the scripted tool.
	 * @returns tool
	 */
	static tool_t* load_tool(char const* path, tool_t* tool = NULL);

	/**
	 * Reads all tools from directory @p path.
	 */
	static void load_scripts(char const* path);

	/**
	 * Fills toolbar with scripted tools.
	 * Select only tools with menu entry equal to @p arg.
	 */
	static void fill_menu(tool_selector_t* tool_selector, char const* arg, sint16 sound_ok);
};

#endif
