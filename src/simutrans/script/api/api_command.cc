/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_command.cc exports the command_x class, which encodes the tools to manipulate a game */

#include "api_command.h"
#include "api_obj_desc_base.h"
#include "api_simple.h"
#include "../api_param.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../tool/simmenu.h"
#include "../../tool/simtool.h"
#include "../../world/simworld.h"
#include "../../dataobj/environment.h"
#include "../../script/script.h"
#include "../../descriptor/bridge_desc.h"
#include "../../descriptor/building_desc.h"
#include "../../descriptor/roadsign_desc.h"
#include "../../descriptor/way_obj_desc.h"

#include <memory> // auto_ptr

using namespace script_api;


SQInteger command_constructor(HSQUIRRELVM vm)
{
	// create tool
	uint16 id = param<uint16>::get(vm, 2);

	if (id & GENERAL_TOOL) {
		// we do not want scripts to open dialogues or quitting the game etc

		if (id == (TOOL_EXEC_TWO_CLICK_SCRIPT | GENERAL_TOOL)) {
			// do not create & attach instance, will be done separately
			return 0;
		}
		else {
			if (tool_t *tool = create_tool(id)) {
				my_tool_t* mtool = new my_tool_t(tool);
				attach_instance(vm, 1, mtool);
				return 0;
			}
		}
	}
	return sq_raise_error(vm, "Invalid tool called (%d / 0x%x)", id & 0xfff, id);
}


SQInteger param<call_tool_init>::push(HSQUIRRELVM vm, call_tool_init v)
{
	if (v.error) {
		return sq_raise_error(vm, *v.error ? v.error : "Strange error occurred");
	}
	// create tool, if necessary, delete on exit
	std::unique_ptr<tool_t> our_tool;
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
	my_tool_t *mtool = get_attached_instance<my_tool_t>(vm, 1, param<tool_t*>::tag());
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
		return sq_raise_error(vm, *v.error ? v.error : "Strange error occurred");
	}
	// create tool, if necessary, delete on exit
	std::unique_ptr<tool_t> our_tool;
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


void set_flags(tool_t *tool, uint8 flags)
{
	tool->flags = flags & (tool_t::WFL_SHIFT | tool_t:: WFL_CTRL);
}


void* script_api::param<tool_t*>::tag()
{
	return (void*)param<tool_t*>::get;
}


const char* is_available(const obj_desc_timelined_t* desc)
{
	return desc->is_available(welt->get_timeline_year_month()) ? NULL : "Object not available (retired or future).";
}


call_tool_work build_way(player_t* pl, koord3d start, koord3d end, const way_desc_t* way, bool straight, bool keep_city_roads)
{
	if (way == NULL) {
		return call_tool_work("No way provided");
	}
	if (const char* err = is_available(way)) {
		return call_tool_work(err);
	}
	return call_tool_work(TOOL_BUILD_WAY | GENERAL_TOOL, way->get_name(), (straight ? 2 : 0) + (keep_city_roads ? 1 : 0), pl, start, end);
}


call_tool_work build_wayobj(player_t* pl, koord3d start, koord3d end, const way_obj_desc_t* wayobj)
{
	if (wayobj == NULL) {
		return call_tool_work("No wayobj provided");
	}
	if (const char* err = is_available(wayobj)) {
		return call_tool_work(err);
	}
	return call_tool_work(TOOL_BUILD_WAYOBJ | GENERAL_TOOL, wayobj->get_name(), 0, pl, start, end);
}


typedef call_tool_work(*bsr_type)(player_t*, koord3d, const building_desc_t*, my_ribi_t);

call_tool_work build_station_rotation(player_t* pl, koord3d pos, const building_desc_t* building, my_ribi_t rotation)
{
	// rotation: SENW -> 0123, see station_building_select_t
	int rot = -1;
	switch( (ribi_t::ribi)rotation )
	{
		case ribi_t::south: rot = 0; break;
		case ribi_t::east:  rot = 1; break;
		case ribi_t::north: rot = 2; break;
		case ribi_t::west:  rot = 3; break;
		default: ;
	}
	if (building == NULL  ||  !building->is_transport_building()) {
		return call_tool_work("No building provided");
	}
	if (const char* err = is_available(building)) {
		return call_tool_work(err);
	}
	static cbuffer_t buf;
	buf.clear();
	buf.printf("%s,%i", building->get_name(), rot);
	return call_tool_work(TOOL_BUILD_STATION | GENERAL_TOOL, buf, 0, pl, pos);
}

SQInteger command_build_station(HSQUIRRELVM vm)
{
	/* possible calling conventions:
	 *
	 * build_station(player, pos, desc)           - top == 4
	 * build_station(player, pos, desc, rotation) - top == 5
	 */
	if (sq_gettop(vm) == 4) {
		// rotation parameter missing, push default value
		sq_pushinteger(vm, ribi_t::all);
	}
	return embed_call_t<bsr_type>::call_function(vm, build_station_rotation, false);
}


call_tool_work build_depot(player_t* pl, koord3d pos, const building_desc_t* building)
{
	if (building == NULL  ||  !building->is_depot()) {
		return call_tool_work("No depot provided");
	}
	if (const char* err = is_available(building)) {
		return call_tool_work(err);
	}
	return call_tool_work(TOOL_BUILD_DEPOT | GENERAL_TOOL, building->get_name(), 0, pl, pos);
}

call_tool_work build_bridge(player_t* pl, koord3d start, koord3d end, const bridge_desc_t* bridge)
{
	if (bridge == NULL) {
		return call_tool_work("No bridge provided");
	}
	if (const char* err = is_available(bridge)) {
		return call_tool_work(err);
	}
	return call_tool_work(TOOL_BUILD_BRIDGE | GENERAL_TOOL, bridge->get_name(), 2, pl, start, end);
}

call_tool_work build_bridge_at(player_t* pl, koord3d start, const bridge_desc_t* bridge)
{
	if (bridge == NULL) {
		return call_tool_work("No bridge provided");
	}
	if (const char* err = is_available(bridge)) {
		return call_tool_work(err);
	}
	return call_tool_work(TOOL_BUILD_BRIDGE | GENERAL_TOOL, bridge->get_name(), 0, pl, start);
}

call_tool_work set_slope(player_t* pl, koord3d start, my_slope_t slope)
{
	// communicate per default_param
	static char buf[8];
	sprintf(buf, "%2d", (uint8)slope);
	static tool_setslope_t tool;
	// we do not want our slopes translated to double-height (even for single-height paksets), they are already in the double-height system
	tool.old_slope_compatibility_mode = false;
	tool.set_default_param(buf);
	return call_tool_work(&tool, pl, start);
}

call_tool_work restore_slope(player_t* pl, koord3d start)
{
	return call_tool_work(TOOL_RESTORESLOPE | GENERAL_TOOL, "", 0, pl, start);
}

const char* can_set_slope(player_t* pl, koord3d pos, my_slope_t slope)
{
	return tool_setslope_t::tool_set_slope_work(pl, pos, slope, false /* compatibility */, true /* check */);
}

sint64 set_slope_get_price(my_slope_t slope)
{
	return slope == RESTORE_SLOPE ? -welt->get_settings().cst_alter_land : -welt->get_settings().cst_set_slope;
}

call_tool_work build_sign_at(player_t* pl, koord3d start, const roadsign_desc_t* sign)
{
	if (sign == NULL) {
		return call_tool_work("No sign provided");
	}
	if (const char* err = is_available(sign)) {
		return call_tool_work(err);
	}
	return call_tool_work(TOOL_BUILD_ROADSIGN | GENERAL_TOOL, sign->get_name(), 0, pl, start);
}

call_tool_work change_climate_at(player_t* pl, koord3d start, int climate)
{
	if (climate < water_climate || climate >= MAX_CLIMATES) {
		return call_tool_work("Invalid climate number provided");
	}
	// communicate per default_param
	static cbuffer_t param;
	param.clear();
	param.printf("%d", climate);
	return call_tool_work(TOOL_SET_CLIMATE | GENERAL_TOOL, param, 0, pl, start, start);
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
	register_method(vm, &set_flags, "set_flags", true);
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
	/**
	 * Build a way.
	 * @param pl player to pay for the work
	 * @param start coordinate, where work begins
	 * @param end   coordinate, where work ends
	 * @param way type of way to be built
	 * @param straight force building of straight ways, similar as building way with control key pressed
	 */
	STATIC register_method_fv(vm, build_way, "build_way", freevariable<bool>(false), false, true);
	/**
	 * Build a road.
	 * @param pl player to pay for the work
	 * @param start coordinate, where work begins
	 * @param end   coordinate, where work ends
	 * @param way type of way to be built
	 * @param straight force building of straight ways, similar as building way with control key pressed
	 * @param keep_city_roads if true city roads will not be replaced
	 */
	STATIC register_method(vm, build_way, "build_road", false, true);
	/**
	 * Build a depot.
	 * @param pl player to pay for the work
	 * @param pos position to place the depot
	 * @param depot type of depot to be built
	 */
	STATIC register_method(vm, build_depot, "build_depot", false, true);
	/**
	 * Build a station or station extension building.
	 * @param pl player to pay for the work
	 * @param pos position to place the depot
	 * @param station type of station to be built
	 * @param rotaton (optional parameter) rotation of building (only used for flat docks, put direction from land to water here)
	 */
	STATIC register_function(vm, command_build_station, "build_station", -4 /* at least 4 parameters */,
							 func_signature_t<bsr_type>::get_typemask(false).c_str(), false /* static */);

	log_squirrel_type(func_signature_t<bsr_type>::get_squirrel_class(false), "build_station", func_signature_t<bsr_type>::get_squirrel_type(false, 0));
	/**
	 * Build a bridge.
	 * Similar to drag-and-build of bridges in-game.
	 * @param pl player to pay for the work
	 * @param start coordinate, where bridge begins
	 * @param end   coordinate, where bridge ends
	 * @param bridge type of bridge to be built
	 */
	STATIC register_method(vm, build_bridge, "build_bridge", false, true);
	/**
	 * Build a bridge.
	 * Similar to one click with mouse on suitable start tile: program will figure out bridge span itself.
	 * @param pl player to pay for the work
	 * @param start coordinate, where bridge begins, the end point will be automatically determined
	 * @param bridge type of bridge to be built
	 */
	STATIC register_method(vm, build_bridge_at, "build_bridge_at", false, true);
	/**
	 * Modify the slope of one tile.
	 * @param pl player to pay for the work
	 * @param pos position of tile
	 * @param slope new slope, can also be one of @ref slope::all_up_slope or @ref slope::all_down_slope.
	 */
	STATIC register_method(vm, set_slope, "set_slope", false, true);
	/**
	 * Restore natural slope of one tile.
	 * @param pl player to pay for the work
	 * @param pos position of tile
	 */
	STATIC register_method(vm, restore_slope, "restore_slope", false, true);
	/**
	 * Checks whether player @p pl can do this terraforming.
	 * @param pl player
	 * @param pos position
	 * @param slope new slope, can also be one of @ref slope::all_up_slope or @ref slope::all_down_slope
	 * @returns null (if allowed) or an error message otherwise
	 */
	STATIC register_method(vm, can_set_slope, "can_set_slope", false, true);
	/**
	 * Costs of using @ref set_slope.
	 * @returns cost
	 */
	STATIC register_method(vm, set_slope_get_price, "slope_get_price", false, true);
	/**
	 * Build signal / road-sign. If such a sign already exists then change its direction.
	 * @param pl player to pay for the work
	 * @param pos position of tile
	 * @param sign type of road-sign or signal to be built
	 */
	STATIC register_method(vm, build_sign_at, "build_sign_at", false, true);
	/**
	 * Build way-object.
	 * @param pl player to pay for the work
	 * @param start coordinate, where work begins
	 * @param end   coordinate, where work ends
	 * @param wayobj type of wayobj to be built
	 */
	STATIC register_method(vm, build_wayobj, "build_wayobj", false, true);
	/**
	 * Change climate of tile
	 * @param pl player to pay for the work
	 * @param pos coordinate of tile
	 * @param climate new climate, possible values see @ref climates
	 */
	STATIC register_method(vm, change_climate_at, "change_climate_at", false, true);

	end_class(vm);
}
