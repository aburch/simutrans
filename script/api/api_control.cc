#include "api.h"

/** @file api_control.cc script control and debug functions. */

#include "../api_function.h"
#include "../../squirrel/sq_extensions.h"

using namespace script_api;

void export_control(HSQUIRRELVM vm)
{
	// in global scope

	/**
	 * Suspends execution of the script. Upon resume, the script
	 * will have full count of opcodes to spent for fragile code,
	 * i.e. writing into @ref persistent.
	 * @typemask void()
	 */
	register_function(vm, sq_suspendvm, "sleep", 1, ".");

	/**
	 * @returns total amount of opcodes executed by vm
	 */
	register_function(vm, sq_get_ops_total, "get_ops_total", 1, ".");

	/**
	 * @returns amount of remaining opcodes until vm will be suspended
	 */
	register_function(vm, sq_get_ops_remaing, "get_ops_remaining", 1, ".");
}
