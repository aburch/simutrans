#include "../api_function.h"
#include "../../squirrel/sqstdio.h" // for loadfile
#include "../../squirrel/sq_extensions.h" // for sq_call_restricted

/** @file api_include.cc exports include. */

using namespace script_api;


SQInteger include_aux(HSQUIRRELVM vm)
{
	const char* filename = param<const char*>::get(vm, 2);
	const char* include_path = param<const char*>::get(vm, 3);

	if (filename == NULL) {
		sq_raise_error(vm, "Include got <NULL> filename");
		return SQ_ERROR;
	}

	// path to scenario files
	cbuffer_t buf;
	buf.printf("%s/%s.nut", include_path, filename);

	// load script
	if (!SQ_SUCCEEDED(sqstd_loadfile(vm, (const char*)buf, true))) {
		sq_raise_error(vm, "Reading / compiling script %s failed", filename);
		return SQ_ERROR;
	}
	// call it
	sq_pushroottable(vm);
	if (!SQ_SUCCEEDED(sq_call_restricted(vm, 1, SQFalse, SQTrue))) {
		sq_pop(vm, 1); // pop script
		sq_raise_error(vm, "Call script %s failed", filename);
		return SQ_ERROR;
	}
	if (sq_getvmstate(vm) == SQ_VMSTATE_SUSPENDED) {
		sq_raise_error(vm, "Calling scriptfile %s took too long", filename);
		return SQ_ERROR;
	}
	sq_pop(vm, 1); // pop script
	return SQ_OK;
}


void export_include(HSQUIRRELVM vm, const char* include_path)
{
	script_api::start_squirrel_type_logging();
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
	script_api::end_squirrel_type_logging();
}
