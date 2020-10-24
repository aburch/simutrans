/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_command.cc exports the command_x class, which encodes the tools to manipulate a game */

#include "api_command.h"
#include "../api_param.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../simmenu.h"
#include "../../simworld.h"
#include "../../dataobj/environment.h"
#include "../../script/script.h"

#include <memory> // auto_ptr

using namespace script_api;


SQInteger command_constructor(HSQUIRRELVM vm)
{
	// create tool
	uint16 id = param<uint16>::get(vm, 2);

	if (tool_t *tool = create_tool(id)) {
		my_tool_t* mtool = new my_tool_t(tool);
		attach_instance(vm, 1, mtool);
		return 0;
	}
	return -1;
}


SQInteger param<call_tool_init>::push(HSQUIRRELVM vm, call_tool_init v)
{
	if (v.error) {
		return sq_raise_error(vm, *v.error ? v.error : "Strange error occured");
	}
	// create tool, if necessary, delete on exit
	std::auto_ptr<tool_t> our_tool;
	tool_t *tool = v.tool;
	if (tool == NULL) {
		our_tool.reset(v.create_tool());
		tool = our_tool.get();
	}
	if (tool == NULL) {
		return sq_raise_error(vm, "Called null tool");
	}
	// set correct flags
	tool->flags &= (tool_t::WFL_SHIFT | tool_t:: WFL_CTRL);
	tool->flags |= tool_t::WFL_SCRIPT;
	// get player parameter
	player_t *player = get_my_player(vm);
	if (player == NULL) {
		player = v.player;
		// must be scenario - set flag
		tool->flags |= tool_t::WFL_NO_CHK;
	}
	// sanity check
	if (player == NULL) {
		return sq_raise_error(vm, "Called tool with player == null");
	}
	// check if calling suspendable tools is blocked
	if (const char* blocker = env_t::networkmode ? sq_get_suspend_blocker(vm) : NULL) {
		return sq_raise_error(vm, "Cannot call this tool from within `%s'.", blocker);
	}

	// register this tool call for callback with this id
	uint32 callback_id = suspended_scripts_t::get_unique_key(tool);
	tool->callback_id = callback_id;

	// HACK call karte_t::set_tool
	welt->set_tool(tool, player);
	// in networkmode, call is suspended

	if (env_t::networkmode) {
		// register for wakeup
		suspended_scripts_t::register_suspended_script(callback_id, vm);
		// suspend vm for now, after wakeup it returns to the script
		return sq_suspendvm(vm);
	}
	else {
		bool res = true;
		return param<bool>::push(vm, res);
	}
}


call_tool_init script_api::command_rename(player_t *owner, char what, koord3d pos, const char* name)
{
	// build param string (see tool_rename_t::init)
	cbuffer_t buf;
	buf.printf( "%c%s,%s", what, pos.get_str(), name);

	// empty name? trigger error by setting id to zero
	uint16 tool_id = name ? TOOL_RENAME | SIMPLE_TOOL : 0;

	return call_tool_init(tool_id, (const char*)buf, 0, owner);
}

call_tool_init script_api::command_rename(player_t *owner, char what, uint32 id, const char* name)
{
	// build param string (see tool_rename_t::init)
	cbuffer_t buf;
	buf.printf( "%c%u,%s", what, id, name);

	// empty name? trigger error by setting id to zero
	uint16 tool_id = name ? TOOL_RENAME | SIMPLE_TOOL : 0;

	return call_tool_init(tool_id, (const char*)buf, 0, owner);
}


SQInteger command_work(HSQUIRRELVM vm)
{
	tool_t *tool = param<tool_t*>::get(vm, 1);

	player_t *player = param<player_t*>::get(vm, 2);

	koord3d pos1 = param<koord3d>::get(vm, 3);

	SQInteger top = sq_gettop(vm); // top >=3
	/* three possible calling conventions:
	 * (1) tool.work(pl, pos)
	 * (2) tool.work(pl, pos, param)
	 * (3) tool.work(pl, pos, pos2, param)
	 */
	const char* default_param = top>3 ? param<const char*>::get(vm, top) : NULL;
	koord3d     pos2          = top>4 ? param<koord3d>::get(vm, 4)       : koord3d::invalid;

	bool twoclick = top>4;

	// save & set default_param
	my_tool_t *mtool = get_attached_instance<my_tool_t>(vm, 1, (void*)param<tool_t*>::get);
	if (mtool == NULL) {
		return sq_raise_error(vm, "Called from an instance different to tool_x");
	}
	plainstring &pdefault_param = mtool->default_param;
	pdefault_param = default_param; // copy
	tool->set_default_param( (const char*)pdefault_param ); // .. and set param

	if (!twoclick) {
		return param<call_tool_work>::push(vm, call_tool_work(tool, player, pos1));
	}
	else {
		return param<call_tool_work>::push(vm, call_tool_work(tool, player, pos1, pos2));
	}
}

tool_t * call_tool_base_t::create_tool()
{
	tool_t *tool = ::create_tool(tool_id);
	if (tool) {
		tool->set_default_param(default_param);
		tool->flags = flags;
	}
	return tool;
}


SQInteger param<call_tool_work>::push(HSQUIRRELVM vm, call_tool_work v)
{
	if (v.error) {
		return sq_raise_error(vm, *v.error ? v.error : "Strange error occured");
	}
	// create tool, if necessary, delete on exit
	std::auto_ptr<tool_t> our_tool;
	tool_t *tool = v.tool;
	if (tool == NULL) {
		our_tool.reset(v.create_tool());
		tool = our_tool.get();
	}
	if (tool == NULL) {
		return sq_raise_error(vm, "Called null tool");
	}
	// set correct flags
	tool->flags &= (tool_t::WFL_SHIFT | tool_t:: WFL_CTRL);
	tool->flags |= tool_t::WFL_SCRIPT;
	// get player parameter
	player_t *player = get_my_player(vm);
	if (player == NULL) {
		player = v.player;
		// must be scenario - set flag
		tool->flags |= tool_t::WFL_NO_CHK;
	}
	// sanity check
	if (player == NULL) {
		return sq_raise_error(vm, "Called tool with player == null");
	}
	uint8 flags = tool->flags; // might be reset by init()

	// call init before work (but check network safety)
	if (!tool->is_init_network_safe()) {
		return sq_raise_error(vm, "Initializing tool has side effects");
	}
	if (!tool->init(player)) {
		return sq_raise_error(vm, "Error during initializing tool");
	}
	// set flags
	tool->flags = flags;
	// test work
	if (tool->is_work_network_safe()  ||  (!v.twoclick  &&  tool->is_work_here_network_safe(player, v.start))) {
		return sq_raise_error(vm, "Tool has no effects");
	}
	// two-click tool
	if (v.twoclick) {
		if (dynamic_cast<two_click_tool_t*>(tool)==NULL) {
			return sq_raise_error(vm, "Cannot call this tool with two coordinates");
		}
		if (!tool->is_work_here_network_safe(player, v.start)) {
			return sq_raise_error(vm, "First click has side effects");
		}
	}
	// check if calling suspendable tools is blocked
	if (const char* blocker = env_t::networkmode ? sq_get_suspend_blocker(vm) : NULL) {
		return sq_raise_error(vm, "Cannot call this tool from within `%s'.", blocker);
	}


	bool suspended = false;
	const char* err = NULL;

	// first click of two_click_tool_t
	if (v.twoclick) {
		err = welt->call_work(tool, player, v.start, suspended);
		assert(!suspended);
	}
	// call the work that has effects
	if (err == NULL) {
		// register this tool call for callback with this id
		uint32 callback_id = suspended_scripts_t::get_unique_key(tool);
		tool->callback_id = callback_id;
		err = welt->call_work(tool, player, v.twoclick ? v.end : v.start, suspended);

		if (suspended) {
			// register for wakeup
			suspended_scripts_t::register_suspended_script(callback_id, vm);
			// suspend vm for now, after wakeup it returns to the script
			return sq_suspendvm(vm);
		}
	}
	// return error message or NULL if tool was called
	return param<const char*>::push(vm, err);
}


uint8 get_flags(tool_t *tool)
{
	return tool->flags  & (tool_t::WFL_SHIFT | tool_t:: WFL_CTRL);
}

// FIXME: For reasons beyond fathom, this will not compile.
void_t set_flags(tool_t *tool, uint8 flags)
{
	tool->flags = flags & (tool_t::WFL_SHIFT | tool_t:: WFL_CTRL);
	return void_t();
}


void* script_api::param<tool_t*>::tag()
{
	return (void*)param<tool_t*>::get;
}


void export_commands(HSQUIRRELVM vm)
{
	/**
	 * Proof-of-concept to make tools available to scripts.
	 *
	 * The default_param is not checked. Use at own risk.
	 * @ingroup game_cmd
	 */
	create_class(vm, "command_x");

	/**
	 * Constructor to obtain a tool.
	 * @param id id of the tool
	 * @typemask command_x(tool_ids)
	 */
	register_function(vm, command_constructor, "constructor", 2, "xi");
	// set type tag to custom getter
	sq_settypetag(vm, -1, param<tool_t*>::tag());

	/**
	 * @returns flags set for this tool
	 */
	register_method(vm, &get_flags, "get_flags", true);
	/**
	 * Sets flags for this tool.
	 * Simulates pressing shift or ctrl while clicking with mouse.
	 * @param flags bitmap, 1 = shift pressed, 2 = ctrl pressed
	 */
	//register_method(vm, &set_flags, "set_flags", true);
	/**
	 * Does the dirty work.
	 * @note In network games script will be suspended until the command is executed.
	 * @param pl player to pay for the work
	 * @param pos coordinate, where something should happen
	 * @returns null upon success, an error string otherwise
	 * @typemask string(player_x,coord3d)
	 */
	register_function(vm, command_work, "work", -3, "x t|x|y t|x|y");

#ifdef DOXYGEN
	// command_work works with different numbers of parameters.
	// Their documentation is included here.

	/**
	 * Does the dirty work.
	 * @note In network games script will be suspended until the command is executed.
	 * @param pl player to pay for the work
	 * @param pos coordinate, where something should happen
	 * @param param magic parameter string
	 * @returns null upon success, an error string otherwise
	 * @typemask string(player_x,coord3d,string)
	 */
	register_function(vm,, "work");
	const char* work(player_x player, coord3d pos, string param);
	/**
	 * Does the dirty work.
	 * Needs two positions for start and end of the work, for e.g., road building.
	 * @note In network games script will be suspended until the command is executed.
	 * @param pl player to pay for the work
	 * @param start coordinate, where work begins
	 * @param end   coordinate, where work ends
	 * @param param magic parameter string
	 * @returns null upon success, an error string otherwise
	 * @typemask string(player_x,coord3d,coord3d,string)
	 */
	register_function(vm,, "work");
#endif

	end_class(vm);
}
