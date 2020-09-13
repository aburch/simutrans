/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_control.cc script control and debug functions. */

#include "../api_function.h"
#include "../../squirrel/sq_extensions.h"

using namespace script_api;

// SQInteger sleep(HSQUIRRELVM vm)
// {
// 	if (const char* blocker = sq_get_suspend_blocker(vm)) {
// 		return sq_raise_error(vm, "Cannot call sleep from within `%s'.", blocker);
// 	}
// 	return sq_suspendvm(vm);
// }

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
	// register_function(vm, sleep, "sleep", 1, ".");

	// /**
	//  * @returns total amount of opcodes executed by vm
	//  */
	// register_function(vm, sq_get_ops_total, "get_ops_total", 1, ".");

	// /**
	//  * @returns amount of remaining opcodes until vm will be suspended
	//  */
	// register_function(vm, sq_get_ops_remaing, "get_ops_remaining", 1, ".");
}
