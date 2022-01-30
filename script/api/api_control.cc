/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_control.cc script control and debug functions. */

#include "../api_function.h"
#include "../api_class.h"
#include "../script.h"
#include "../../squirrel/sq_extensions.h"
#include "../../simtool.h"

namespace script_api {

	bool pause_game()
	{
		tool_pause_t t;
		if (!t.is_selected()) {
			t.init(NULL);
		}
		return t.is_selected();
	}

	bool is_game_paused()
	{
		tool_pause_t t;
		return t.is_selected();
	}
};

using namespace script_api;

SQInteger sleep(HSQUIRRELVM vm)
{
	if (const char* blocker = sq_get_suspend_blocker(vm)) {
		return sq_raise_error(vm, "Cannot call sleep from within `%s'.", blocker);
	}
	return sq_suspendvm(vm);
}

SQInteger set_pause_on_error(HSQUIRRELVM vm)
{
	if (script_vm_t *script = (script_vm_t*)sq_getforeignptr(vm)) {
		bool poe = param<bool>::get(vm, 2);
		script->pause_on_error = poe;
	}
	return SQ_OK;
}



void export_control(HSQUIRRELVM vm)
{
	// in global scope

	/**
	 * Suspends execution of the script. Upon resume, the script
	 * will have full count of opcodes to spent for fragile code,
	 * i.e. writing into @ref persistent.
	 * @typemask void()
	 */
	register_function(vm, sleep, "sleep", 1, ".");

	/**
	 * @returns total amount of opcodes executed by vm
	 */
	register_function<int(*)()>(vm, sq_get_ops_total, "get_ops_total");

	/**
	 * @returns amount of remaining opcodes until vm will be suspended
	 */
	register_function<int(*)()>(vm, sq_get_ops_remaing, "get_ops_remaining");


	begin_class(vm, "debug");
	/**
	 * Pauses game. Does not work in network games. Use with care.
	 * @returns true when successful.
	 */
	STATIC register_method(vm, &pause_game, "pause", false, true);
	/**
	 * Checks whether game is paused.
	 * Note that scripts only run for unpaused games.
	 * This can only be used in functions that are called by user actions (e.g., is_*_allowed).
	 * @returns true when successful.
	 */
	STATIC register_method(vm, &is_game_paused, "is_paused", false, true);

	/**
	 * Scripts can pause the game in case of error. Toggle this behavior by parameter @p p.
	 * Does not work in network games. Use with care.
	 * @param p true if script should make the game pause in case of error
	 */
	STATIC register_function<void(*)(bool)>(vm, set_pause_on_error, "set_pause_on_error", true);

	end_class(vm);
}
