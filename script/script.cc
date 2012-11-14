#include "script.h"

#include <stdarg.h>
#include <string.h>
#include "../squirrel/sqstdaux.h" // for error handlers
#include "../squirrel/sqstdio.h" // for loadfile
#include "../squirrel/sqstdstring.h" // export for scripts
#include "../squirrel/sq_extensions.h" // for loadfile

#include "../utils/log.h"

#include "../tpl/vector_tpl.h"
// for error popups
#include "../gui/help_frame.h"
#include "../simwin.h"
#include "../utils/cbuffer_t.h"
#include "../utils/plainstring.h"

// log file
static log_t* script_log = NULL;
// list of active scripts (they share the same log-file, error and print-functions)
static vector_tpl<script_vm_t*> all_scripts;

static void printfunc(HSQUIRRELVM, const SQChar *s, ...)
{
	va_list vl;
	va_start(vl, s);
	script_log->vmessage("Script", "Print", s, vl);
	va_end(vl);
}


void script_vm_t::errorfunc(HSQUIRRELVM vm, const SQChar *s_, ...)
{

	char *s = strdup(s_);
	char *s_dup = s;
	// remove these linebreaks
	while (s  &&  *s=='\n') s++;
	while (s  &&  s[strlen(s)-1]=='\n') s[strlen(s)-1]=0;

	va_list vl;
	va_start(vl, s_);
	script_log->vmessage("[Script]", "ERROR", s, vl);
	va_end(vl);

	static cbuffer_t buf;
	if (strcmp(s, "<error>")==0) {
		buf.clear();
		buf.printf("<st>Your script made an error!</st>\n");
	}
	if (strcmp(s, "</error>")==0) {
		help_frame_t *win = new help_frame_t();
		win->set_text(buf);
		win->set_name("Script error occured");
		create_win( win, w_info, magic_none);
		// find failed script
		for(uint32 i=0; i<all_scripts.get_count(); i++) {
			if (all_scripts[i]->get_vm() == vm) {
				all_scripts[i]->set_error(buf);
				break;
			}
		}
	}

	va_start(vl, s_);
	buf.vprintf(s, vl);
	va_end(vl);
	buf.append("<br>");

	free(s_dup);
}


// virtual machine
script_vm_t::script_vm_t()
{
	vm = sq_open(1024);
	sqstd_seterrorhandlers(vm);
	sq_setprintfunc(vm, printfunc, errorfunc);
	if (script_log == NULL) {
		script_log = new log_t("script.log", true, true, true, "script engine started.\n");
	}
	all_scripts.append(this);
	error_msg = NULL;
	// register libraries
	sq_pushroottable(vm);
	sqstd_register_stringlib(vm);
	sq_pop(vm, 1);
}


script_vm_t::~script_vm_t()
{
	sq_close(vm);
	all_scripts.remove(this);
	if (all_scripts.empty()) {
		delete script_log;
		script_log = NULL;
	}
}

const char* script_vm_t::call_script(const char* filename)
{
	// load script
	if (!SQ_SUCCEEDED(sqstd_loadfile(vm, filename, true))) {
		return "Reading / compiling script failed";
	}
	// call it
	sq_pushroottable(vm);
	if (!SQ_SUCCEEDED(sq_call_restricted(vm, 1, SQFalse, SQTrue))) {
		sq_pop(vm, 1); // pop script
		return "Call script failed";
	}
	if (sq_getvmstate(vm) == SQ_VMSTATE_SUSPENDED) {
		set_error("Calling scriptfile took too long");
		return get_error();
	}
	sq_pop(vm, 1); // pop script
	return NULL;
}


const char* script_vm_t::eval_string(const char* squirrel_string)
{
	// compile string
	if (!SQ_SUCCEEDED(sq_compilebuffer(vm, squirrel_string, strlen(squirrel_string), "userdefinedstringmethod", true))) {
		set_error("Error compiling string buffer");
		return get_error();
	}
	// execute
	sq_pushroottable(vm);
	sq_call_restricted(vm, 1, false, true);
	sq_pop(vm, 1);
	return get_error();
}


const char* script_vm_t::intern_start_function(const char* function)
{
	const char* err = NULL;
	sq_pushroottable(vm);
	sq_pushstring(vm, function, -1);
	// fetch function from root
	if (SQ_SUCCEEDED(sq_get(vm,-2))) {
		sq_pushroottable(vm);
	}
	else {
		err = "Function not found";
		sq_pop(vm, 1);
	}
	return err;
}


const char* script_vm_t::intern_call_function(int nparams, bool retvalue)
{
	const char* err = NULL;
	// call the script
	if (!SQ_SUCCEEDED(sq_call_restricted(vm, nparams, retvalue, SQTrue))) {
		err = "Call function failed";
		retvalue = false;
	}
	// remove closure and root table
	sq_remove(vm, retvalue ? -2 : -1);
	sq_remove(vm, retvalue ? -2 : -1);
	return err;
}
