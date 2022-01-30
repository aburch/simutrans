/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simtool-scripted.h"
#include "player/simplay.h"

#include "script/script_tool_manager.h"
#include "script/script.h"
#include "script/script_loader.h"
#include "script/api/api.h"
#include "script/api_class.h"
#include "script/api_param.h"

#include "simskin.h"
#include "simworld.h"
#include "obj/zeiger.h"

void export_scripted_tools(HSQUIRRELVM vm);

// -- callback to receive error messages from work/do_work calls
static bool exec_script_base_work_callback(exec_script_base_t *esb, player_t* player, const char* err)
{
	tool_t *tool = esb ? dynamic_cast<tool_t*>(esb) : NULL;
	if (tool  &&  player) {
		player->tell_tool_result(tool, koord3d::invalid, err);
	}
	if (tool_exec_two_click_script_t* tct = dynamic_cast<tool_exec_two_click_script_t*>(esb)) {
		tct->waiting_for_do_work = false;
		// we cannot call init() here directly, because it may destroy the vm that called us
		tct->needs_call_to_init = true;
	}
	return true;
}

// -- transfer pointers to tools between squirrel and c++
namespace script_api {
	/**
	 * Mechanic to transfer exec_script_base_t* pointers between squirrel and c++,
	 * used for callbacks.
	 */
	declare_specialized_param(exec_script_base_t*, "x", "scripted_tool_t");

	SQInteger param<exec_script_base_t*>::push(HSQUIRRELVM vm, exec_script_base_t* const& v)
	{
		return push_instance_up(vm, v);
	}
	exec_script_base_t* param<exec_script_base_t*>::get(HSQUIRRELVM vm, SQInteger index)
	{
		return get_attached_instance<exec_script_base_t>(vm, index, param<exec_script_base_t*>::tag());
	}
	void* param<exec_script_base_t*>::tag() { return (void*)&param<exec_script_base_t*>::get; }

	/**
	 * Mechanic to transfer tool_exec_two_click_script_t* pointers between squirrel and c++,
	 * used for marking tiles in two-click tools.
	 */
	template<> struct param<tool_exec_two_click_script_t*> {
		static tool_exec_two_click_script_t* get(HSQUIRRELVM vm, SQInteger index)
		{
			tool_t* tool = param<tool_t*>::get(vm, index);
			return tool ? dynamic_cast<tool_exec_two_click_script_t*>(tool) : NULL;
		}

		static SQInteger push(HSQUIRRELVM vm, tool_exec_two_click_script_t* const& tool)
		{
			SQInteger res = push_instance(vm, "command_x", (uint32)(TOOL_EXEC_TWO_CLICK_SCRIPT | GENERAL_TOOL) );
			if (SQ_SUCCEEDED(res)) {
				my_tool_t* mtool = new my_tool_t(tool);
				attach_instance(vm, -1, mtool);
			}
			return res;
		}
	};
};

// -- basic script handling --
exec_script_base_t::~exec_script_base_t()
{
	delete info;
	delete script;
}


void exec_script_base_t::set_info(const scripted_tool_info_t *i)
{
	delete info;
	info = i;
}


bool exec_script_base_t::init_vm(player_t* player)
{
	if (get_info() == NULL) {
		// tool probably read from menuconf.tab
		// path in default_param, initialize
		if (tool_t *tool = dynamic_cast<tool_t*>(this)) {
			script_tool_manager_t::load_tool(tool->get_default_param(), tool);
		}
	}
	if(  script==NULL  ||  (info  &&  info->restart)  ) {
		load_script(info->path, player);
	}
	return script != NULL;
}


void exec_script_base_t::load_script(const char* path, player_t* player)
{
	cbuffer_t buf;
	buf.printf("script-exec-%d.log", player->get_player_nr());
	if(  script  ) {
		// if vm already exists, delete it.
		delete script;
		script = NULL;
	}
	// start vm
	script = script_loader_t::start_vm("tool_base.nut", buf, path, false);
	if (script == NULL) {
		return;
	}
	// set my player number
	script->set_my_player(player->get_player_nr());
	// export tool-pointer handling
	export_scripted_tools(script->get_vm());
	// callback
	script->register_callback(exec_script_base_work_callback, "exec_script_base_work_callback");
	// call script to initialize it
	buf.clear();
	buf.printf( "%s/tool.nut", path );
	if (const char* err = script->call_script(buf)) {
		if (strcmp(err, "suspended")) {
			dbg->error("tool_exec_script_t::load_script", "error [%s] calling %s", err, (const char*)buf);
			delete script;
			script = NULL;
		}
	}
	// older versions did not support the flags parameter - correct with helper function
	if (const char* err = script->call_function(script_vm_t::QUEUE, "correct_missing_flags_argument")) {
		if (strcmp(err, "suspended")) {
			dbg->error("tool_exec_script_t::load_script", "error [%s] calling correct_missing_flags_argument", err);
			delete script;
			script = NULL;
		}
	}
}


void export_scripted_tools(HSQUIRRELVM vm)
{
	script_api::create_class<exec_script_base_t*>(vm, script_api::param<exec_script_base_t*>::squirrel_type());
	script_api::end_class(vm);
}


template<class R, class... As>
const char* exec_script_base_t::call_function(script_vm_t::call_type_t ct, const char* function, player_t* player, R& ret, const As &... as)
{
	if (!script) {
		dbg->warning("tool_exec_script_t::call_function", "script vm is not available.");
		return "";
	}
	const char* err = script->call_function(ct, function, ret, player, as...);
	if (err  &&  strcmp(err, "suspended")) {
		dbg->warning("tool_exec_script_t::call_function", "error calling %s: %s", function, err);
	}
	return err;
}


void exec_script_base_t::step(player_t* player)
{
	if (script) {
		script->call_function(script_vm_t::QUEUE, "step");

		if (tool_exec_two_click_script_t *tt = dynamic_cast<tool_exec_two_click_script_t*>(this)) {
			if (tt->needs_call_to_init) {
				tt->init(player);
			}
		}
	}
}


void exec_script_base_t::init_images(tool_t *tool) const
{
	if (info  &&  info->desc) {
		if (info->desc->get_image_id(0) != IMG_EMPTY) {
			tool->cursor = info->desc->get_image_id(0);
		}
		tool->set_icon(info->desc->get_image_id(1));
	}
}


tool_exec_script_t::tool_exec_script_t(const scripted_tool_info_t *info) : tool_t(TOOL_EXEC_SCRIPT | GENERAL_TOOL), exec_script_base_t(info)
{
	init_images(this);
}


bool tool_exec_script_t::init(player_t* player)
{
	bool res = false;
	cursor_area = get_cursor_area();
	cursor_offset = get_cursor_offset();
	// rotate area and offset
	for(uint8 i=0;  i<world()->get_settings().get_rotation();  i++) {
		koord old_area = cursor_area;
		koord old_offset = cursor_offset;
		cursor_area = koord(old_area.y, old_area.x);
		cursor_offset = koord(old_area.y-1-old_offset.y, old_offset.x);
	}
	return init_vm(player)  &&  call_function(script_vm_t::FORCE, "init", player, res)== NULL  &&  res;
}


bool tool_exec_script_t::exit(player_t* player)
{
	bool res, res2 = false;
	// exit script
	res = call_function(script_vm_t::FORCE, "exit", player, res2) == NULL;
	// shut down vm
	delete script;
	script = NULL;
	return res  &&  res2;
}


const char* tool_exec_script_t::work(player_t* player, koord3d pos)
{
	static plainstring res;
	// callback
	script->prepare_callback("exec_script_base_work_callback", 3, (exec_script_base_t*)this, player, (const char*)"");
	// now call
	uint8 keys = flags & (tool_t::WFL_SHIFT  |  tool_t::WFL_CTRL);
	const char* err = call_function(script_vm_t::QUEUE, "work", player, res, pos, keys);
	if (err  &&  strcmp(err, "suspended")==0) {
		// suspended
	}
	else {
		// no callback necessary
		script->clear_pending_callback();
	}
	return res.c_str();
}



tool_exec_two_click_script_t::tool_exec_two_click_script_t(const scripted_tool_info_t *info) : two_click_tool_t(TOOL_EXEC_TWO_CLICK_SCRIPT | GENERAL_TOOL), exec_script_base_t(info)
{
	set_marker(IMG_EMPTY);
	waiting_for_do_work = false;
	needs_call_to_init = true;

	init_images(this);

	if (info  &&  info->desc) {
		set_marker(info->desc->get_image(2) ? info->desc->get_image_id(2) : cursor);
	}
}


SQInteger script_mark_tile(HSQUIRRELVM vm); // see below

bool tool_exec_two_click_script_t::init(player_t* player)
{
	// remove marker images
	bool res = two_click_tool_t::init(player);
	if (waiting_for_do_work) {
		return res;
	}

	needs_call_to_init = false;

	res = res  &&  init_vm(player);
	if (res) {
		HSQUIRRELVM vm = script->get_vm();
		// put pointer to this tool into registry
		sq_pushregistrytable(vm);
		script_api::create_slot(vm, "my_two_click_tool", this);
		sq_poptop(vm);
		// export marker function
		sq_pushroottable(vm);
		script_api::register_function(vm, script_mark_tile, "mark_tile", 2, ". t|x|y", false /* static */);
		sq_poptop(vm);
	}
	return res  &&   call_function(script_vm_t::FORCE, "init", player, res)== NULL  &&  res;
}


bool tool_exec_two_click_script_t::exit(player_t* player)
{
	bool res, res2 = false;
	// exit script
	res = two_click_tool_t::exit(player)  &&  call_function(script_vm_t::FORCE, "exit", player, res2) == NULL;
	// shut down vm
	delete script;
	script = NULL;
	return res  &&  res2;
}


const char* tool_exec_two_click_script_t::do_work(player_t* player, const koord3d &start, const koord3d &end)
{
	if (waiting_for_do_work) {
		return "";
	}
	static plainstring res;
	// callback
	script->prepare_callback("exec_script_base_work_callback", 3, (exec_script_base_t*)this, player, (const char*)"");
	// now call
	uint8 keys = flags & (tool_t::WFL_SHIFT  |  tool_t::WFL_CTRL);
	const char* err = call_function(script_vm_t::QUEUE, "do_work", player, res, start, end, keys);
	if (err  &&  strcmp(err, "suspended")==0) {
		// suspended
		waiting_for_do_work = true;
	}
	else {
		// no callback necessary
		script->clear_pending_callback();
	}
	return res.c_str();
}


void  tool_exec_two_click_script_t::mark_tiles(player_t* player, const koord3d &start, const koord3d &end)
{
	if (waiting_for_do_work) {
		return;
	}
	bool dummy;
	// try to mark; if script is busy, do nothing
	uint8 keys = flags & (tool_t::WFL_SHIFT  |  tool_t::WFL_CTRL);
	call_function(script_vm_t::TRY, "mark_tiles", player, dummy, start, end, keys);
}


uint8 tool_exec_two_click_script_t::is_valid_pos(player_t* player, const koord3d &pos, const char *&error, const koord3d &start)
{
	error = NULL;
	uint8 res = 2; // allow dragging
	const char* err = call_function(script_vm_t::FORCEX, "is_valid_pos", player, res, pos, start);
	// script error? signal 'start dragging is allowed here'
	if (err) {
		res = 2; // allow dragging
	}
	return res;
}

// mark_tile(pos)
SQInteger script_mark_tile(HSQUIRRELVM vm)
{
	koord3d pos = script_api::param<koord3d>::get(vm, 2);
	player_t* player = script_api::get_my_player(vm);
	// tool
	sq_pushregistrytable(vm);
	tool_exec_two_click_script_t* tool = NULL;
	if (!SQ_SUCCEEDED(script_api::get_slot<tool_exec_two_click_script_t*>(vm, "my_two_click_tool", tool))  ||  tool == NULL) {
		return SQ_ERROR;
	}
	sq_poptop(vm);
	// now call the method
	return script_api::param<bool>::push(vm, tool->mark_tile(player, pos) );
}


bool tool_exec_two_click_script_t::mark_tile(player_t* player, const koord3d &pos)
{
	grund_t *gr = welt->lookup(pos);
	if (gr) {
		zeiger_t *mark = new zeiger_t(pos, player );
		gr->obj_add(mark);
		mark->set_image(get_marker_image());
		marked.insert(mark);
	}
	return gr != NULL;
}


image_id tool_exec_two_click_script_t::get_marker_image() const
{
	return marker != IMG_EMPTY ? marker : (cursor != IMG_EMPTY ? cursor : skinverwaltung_t::bauigelsymbol->get_image_id(0));
}
