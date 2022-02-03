/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../api_function.h"
#include "../../../squirrel/sqstdio.h" // for loadfile
#include "../../../squirrel/sq_extensions.h" // for sq_call_restricted

/** @file api_include.cc exports include. */

using namespace script_api;


SQInteger include_aux(HSQUIRRELVM vm)
{
	const char* filename = param<const char*>::get(vm, 2);
	const char* include_path = param<const char*>::get(vm, 3);

	if (filename == NULL) {
		return sq_raise_error(vm, "Include got <NULL> filename");
	}

	// path to scenario files
	cbuffer_t buf;
	buf.printf("%s/%s.nut", include_path, filename);

	// load script
	if (!SQ_SUCCEEDED(sqstd_loadfile(vm, (const char*)buf, true))) {
		return sq_raise_error(vm, "Reading / compiling script %s failed", filename);
	}
	// call it
	sq_pushroottable(vm);
	if (!SQ_SUCCEEDED(sq_call_restricted(vm, 1, SQFalse, SQTrue, 100000))) {
		sq_pop(vm, 1); // pop script
		return sq_raise_error(vm, "Call script %s failed", filename);
	}
	if (sq_getvmstate(vm) == SQ_VMSTATE_SUSPENDED) {
		return sq_raise_error(vm, "Calling scriptfile %s took too long", filename);
	}
	sq_pop(vm, 1); // pop script
	return SQ_OK;
}


void export_include(HSQUIRRELVM vm, const char* include_path)
{
	sq_pushroottable(vm);

	/**
	 * Includes the given file.
	 * The file will be included in global scope.
	 * Inclusion happens when the include-command gets executed.
	 * No compile time inclusion!
	 * @param filename name of file to be included (relative to scenario directory, without .nut extension)
	 * @typemask void(string)
	 */
	register_function_fv(vm, &include_aux, "include", 2, ".s", freevariable<const char*>(include_path));

	sq_pop(vm, 1); // root table
}
