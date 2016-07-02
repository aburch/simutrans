#include "api.h"

/** @file api_control.cc script control and debug functions. */

#include "../api_function.h"

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
}
