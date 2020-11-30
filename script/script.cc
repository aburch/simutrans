/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "script.h"

#include <stdarg.h>
#include <string.h>
#include "../squirrel/sqstdaux.h" // for error handlers
#include "../squirrel/sqstdio.h" // for loadfile
#include "../squirrel/sqstdstring.h" // export for scripts
#include "../squirrel/sq_extensions.h" // for sq_call_restricted

#include "../utils/log.h"

#include "../tpl/vector_tpl.h"
// for error popups
#include "../gui/help_frame.h"
#include "../gui/simwin.h"
#include "../utils/cbuffer_t.h"
#include "../utils/plainstring.h"


// debug: store stack pointer
#define BEGIN_STACK_WATCH(v) int stack_top = sq_gettop(v);
// debug: compare stack pointer with expected stack pointer, raise warning if failure
#define END_STACK_WATCH(v, delta) if ( (stack_top+(delta)) != sq_gettop(v)) { dbg->warning( __FUNCTION__, "(%d) stack in %d expected %d out %d", __LINE__,stack_top,stack_top+(delta),sq_gettop(v)); }


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
		buf.printf("<st>Your script made an error!</st><br>\n");
	}
	if (strcmp(s, "</error>")==0) {
		help_frame_t *win = new help_frame_t();
		win->set_text(buf);
		win->set_name("Script error occurred");
		create_win( win, w_info, magic_none);
		// find failed script
		for(uint32 i=0; i<all_scripts.get_count(); i++) {
			if (all_scripts[i]->uses_vm(vm)) {
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

void export_include(HSQUIRRELVM vm, const char* include_path); // api_include.cc

// virtual machine
script_vm_t::script_vm_t(const char* include_path_)
{
	vm = sq_open(1024);
	sqstd_seterrorhandlers(vm);
	sq_setprintfunc(vm, printfunc, errorfunc);
	if (script_log == NULL) {
		script_log = new log_t("script.log", true, true, true, "script engine started.\n");
	}
	all_scripts.append(this);

	// create thread, and put it into registry-table
	sq_pushregistrytable(vm);
	sq_pushstring(vm, "thread", -1);
	thread = sq_newthread(vm, 100);
	sq_newslot(vm, -3, false);
	// create queue array
	sq_pushstring(vm, "queue", -1);
	sq_newarray(vm, 0);
	sq_newslot(vm, -3, false);
	// create queued-callbacks array
	sq_pushstring(vm, "queued_callbacks", -1);
	sq_newarray(vm, 0);
	sq_newslot(vm, -3, false);
	// pop registry
	sq_pop(vm, 1);

	error_msg = NULL;
	include_path = include_path_;
	// register libraries
	sq_pushroottable(vm);
	sqstd_register_stringlib(vm);
	sq_pop(vm, 1);
	// export include command
	export_include(vm, include_path);
}


script_vm_t::~script_vm_t()
{
	sq_close(vm); // also closes thread
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
	sq_call_restricted(vm, 1, SQFalse, SQTrue, 100000);
	sq_pop(vm, 1);
	return get_error();
}


bool script_vm_t::is_call_suspended(const char* err)
{
	return (err != NULL)  &&  ( strcmp(err, "suspended") == 0);
}


const char* script_vm_t::intern_prepare_call(HSQUIRRELVM &job, call_type_t ct, const char* function)
{
	const char* err = NULL;

	switch (ct) {
		case FORCE:
			job = vm;
			break;
		case TRY:
			if (sq_getvmstate(thread) == SQ_VMSTATE_SUSPENDED) {
				return "suspended";
			}
			// fall through
		case QUEUE:
			job = thread;
			break;
	}

	dbg->message("script_vm_t::intern_prepare_call", "ct=%d function=%s stack=%d", ct, function, sq_gettop(job));
	sq_pushroottable(job);
	sq_pushstring(job, function, -1);
	// fetch function from root
	if (SQ_SUCCEEDED(sq_get(job,-2))) {
		sq_remove(job, -2); // root table
		sq_pushroottable(job);
	}
	else {
		err = "Function not found";
		sq_poptop(job); // array, root table
	}
	return err;
	// stack: closure, root table (1st param)
}


const char* script_vm_t::intern_finish_call(HSQUIRRELVM job, call_type_t ct, int nparams, bool retvalue)
{
	BEGIN_STACK_WATCH(job);
	// stack: closure, nparams*objects
	const char* err = NULL;
	bool suspended = sq_getvmstate(job) == SQ_VMSTATE_SUSPENDED;
	// queue function call?
	if (suspended  &&  ct == QUEUE) {
		intern_queue_call(job, nparams, retvalue);
		err = "suspended";
		// stack: clean
	}
	if (suspended) {
		intern_resume_call(job);
	}
	if (!suspended  ||  ct == FORCE) {
		// set active callback if call could be suspended
		if (ct == QUEUE) {
			intern_make_pending_callback_active();
		}
		// mark as not queued
		sq_pushregistrytable(job);
		script_api::create_slot(job, "was_queued", false);
		sq_poptop(job);

		END_STACK_WATCH(job,0);
		err = intern_call_function(job, ct, nparams, retvalue);
	}
	return err;
}

/**
 * Calls function. If it was a queued call, also calls callbacks.
 * Stack(job): expects closure, nparam*objects, on exit: return value (or clean stack if failure).
 * @returns NULL on success, error message or "suspended" otherwise.
 */
const char* script_vm_t::intern_call_function(HSQUIRRELVM job, call_type_t ct, int nparams, bool retvalue)
{
	BEGIN_STACK_WATCH(job);
	dbg->message("script_vm_t::intern_call_function", "start: stack=%d nparams=%d ret=%d", sq_gettop(job), nparams, retvalue);
	const char* err = NULL;
	// call the script
	if (!SQ_SUCCEEDED(sq_call_restricted(job, nparams, retvalue, ct == FORCE))) {
		err = "Call function failed";
		retvalue = false;
	}
	if (sq_getvmstate(job) != SQ_VMSTATE_SUSPENDED) {
		// remove closure
		sq_remove(job, retvalue ? -2 : -1);
		if (ct == QUEUE  &&  retvalue) {

			sq_pushregistrytable(job);
			// notify call backs only if call was queued
			bool notify = false;
			script_api::get_slot(job, "was_queued", notify, -1);
			sq_pop(job, 1);

			if (notify) {
				intern_call_callbacks(job);
			}
		}
		END_STACK_WATCH(job, -nparams-1 + retvalue);
	}
	else {
		// call suspended: pop dummy return value
		if (retvalue) {
			sq_poptop(job);
		}
		// save retvalue flag, number of parameters
		sq_pushregistrytable(job);
		script_api::create_slot(job, "retvalue", retvalue);
		script_api::create_slot(job, "nparams", nparams);
		sq_poptop(job);
		err = "suspended";
	}
	dbg->message("script_vm_t::intern_call_function", "stack=%d err=%s", sq_gettop(job)-retvalue, err);
	return err;
}

void script_vm_t::intern_resume_call(HSQUIRRELVM job)
{
	BEGIN_STACK_WATCH(job);
	// stack: clean
	// get retvalue flag
	sq_pushregistrytable(job);
	bool retvalue = false;
	script_api::get_slot(job, "retvalue", retvalue, -1);
	int nparams = 0;
	script_api::get_slot(job, "nparams", nparams, -1);
	sq_pop(job, 1);
	END_STACK_WATCH(job, 0);

	// vm suspended, but not from call to our methods
	if (nparams < 0) {
		retvalue = false;
	}

	// resume v.m.
	if (!SQ_SUCCEEDED(sq_resumevm(job, retvalue))) {
		retvalue = false;
	}
	// if finished, clear stack
	if (sq_getvmstate(job) != SQ_VMSTATE_SUSPENDED) {

		if (nparams >=0 ) {
			BEGIN_STACK_WATCH(job);
			// remove closure & args
			for(int i=0; i<nparams+1; i++) {
				sq_remove(job, retvalue ? -2 : -1);
			}
			if (retvalue) {
				intern_call_callbacks(job);
				sq_poptop(job);
			}
			dbg->message("script_vm_t::intern_resume_call", "in between stack=%d", sq_gettop(job));
			END_STACK_WATCH(job, -1-nparams-retvalue);
		}
		else {
			// brute force clean stack
			sq_settop(job, 0);
		}

		// make nparams invalid
		sq_pushregistrytable(job);
		script_api::create_slot(job, "nparams", -1);
		sq_poptop(job);

		// proceed with next call in queue
		if (intern_prepare_queued(job, nparams, retvalue)) {
			const char* err = intern_call_function(job, QUEUE, nparams, retvalue);
			if (err == NULL  &&  retvalue) {
				// remove return value: call was queued thus remove return value from stack
				sq_poptop(job);
			}
		}
	}
	else {
		if (retvalue) {
			sq_poptop(job);
		}
	}

	dbg->message("script_vm_t::intern_resume_call", "stack=%d", sq_gettop(job));
}

/**
 * Stack(job): expects closure, nparams*objects, clean on exit.
 * Put call into registry.queue, callback into registry.queued_callbacks.
 * Cleans registry.pending_callback.
 * @param nparams number of parameter of the queued call
 * @param retvalue whether the call would return something
 */
void script_vm_t::intern_queue_call(HSQUIRRELVM job, int nparams, bool retvalue)
{
	BEGIN_STACK_WATCH(job);
	// stack: closure, nparams*objects
	// queue call: put closure and parameters in array
	script_api::param<bool>::push(job, retvalue);
	sq_newarray(job, 0); // to hold parameters for queued calls
	// stack: closure, nparams*objects, retvalue, array
	for(int i=nparams+2; i>0; i--) {
		sq_push(job, -2);
		sq_arrayappend(job, -2);
		sq_remove(job, -2);
	}
	// stack: array
	sq_pushregistrytable(job);
	sq_pushstring(job, "queue", -1);
	sq_get(job, -2);
	// stack: array, registry, queue
	// search queue whether our call is already there
	sint32 size = sq_getsize(job, -1);
	bool equal = false;
	for(sint32 i=0; (i<size) && !equal; i++) {
		sq_pushinteger(job, i);
		sq_get(job, -2);
		// stack: [...], queue[i]
		sint32 n = sq_getsize(job, -1);
		if (n != nparams+2) {
			continue; // different number of arguments
		}
		equal = true;
		// compare arguments of call (including closure and retvalue)
		for(sint32 j=0; (j<n)  &&  equal; j++) {
			sq_pushinteger(job, j);
			sq_get(job, -2);
			// stack: array, registry, queue, queue[i], queue[i][j]
			sq_pushinteger(job, j);
			sq_get(job, -6);
			equal = sq_cmp(job)==0;
			sq_pop(job, 2);
			// stack: array, registry, queue, queue[i]
		}
		// found identic call
		// add callback to queue
		if (equal) {
			sq_pushstring(job, "queued_callbacks", -1);
			sq_get(job, -4);
			sq_pushinteger(job, i);
			sq_get(job, -2);
			// stack: array, registry, queue, queue[i], queued_callbacks, queued_callbacks[i]
			sq_pushstring(job, "pending_callback", -1);
			// delete pending_callback slot and push it
			if (SQ_SUCCEEDED( sq_deleteslot(job, -6, true) )) {
				sq_arrayappend(job, -2);
			}
			sq_pop(job, 2);
		}
		sq_poptop(job);
	}
	// stack: array, registry, queue
	if (!equal) {
		sq_push(job, -3);
		sq_arrayappend(job, -2);
		// add callback to queue
		sq_pushstring(job, "queued_callbacks", -1);
		sq_get(job, -3);
		sq_newarray(job, 10);
		// stack: array, registry, queue, queued_callbacks, queued_callbacks[end]
		sq_pushstring(job, "pending_callback", -1);
		// delete pending_callback slot and push it
		if (SQ_SUCCEEDED( sq_deleteslot(job, -5, true) )) {
			sq_arrayappend(job, -2);
		}
		sq_arrayappend(job, -2);
		sq_poptop(job);
	}
	else {
		dbg->message("script_vm_t::intern_queue_call", "NOT QUEUED stack=%d", sq_gettop(job));
	}
	sq_pop(job, 3);

	END_STACK_WATCH(job, -nparams-1);
	dbg->message("script_vm_t::intern_queue_call", "stack=%d", sq_gettop(job));
	// stack: clean
}

/**
 * Removes queue(0) and puts it on stack.
 * Activates callback by intern_make_queued_callback_active().
 * Stack(job): on success - closure, nparams*objects, on failure - unchanged.
 * @param nparams number of parameters of the call on the stack
 * @param retvalue whether queued call returns something
 * @returns true if a new queued call is on the stack
 */
bool script_vm_t::intern_prepare_queued(HSQUIRRELVM job, int &nparams, bool &retvalue)
{
	BEGIN_STACK_WATCH(job);
	// queued calls
	sq_pushregistrytable(job);
	sq_pushstring(job, "queue", -1);
	sq_get(job, -2);
	sq_pushinteger(job, 0);
	if (SQ_SUCCEEDED(sq_get(job, -2))) {
		// there are suspended jobs
		// stack: registry, queue, array[ retvalue nparams*objects closure]
		nparams = -2;
		while( SQ_SUCCEEDED(sq_arraypop(job, -3-nparams, true))) {
			nparams++;
		}
		retvalue = script_api::param<bool>::get(job, -1);
		sq_poptop(job);

		intern_make_queued_callback_active();
		// stack: registry, queue, array, closure, nparams*objects
		END_STACK_WATCH(job, nparams+4);
		// mark this call as queued
		sq_pushstring(job, "was_queued", -1);
		script_api::param<bool>::push(job, true);
		sq_createslot(job, -6-nparams);
		// cleanup
		sq_remove(job, -2-nparams);
		sq_arrayremove(job, -2-nparams, 0);
		sq_remove(job, -2-nparams);
		sq_remove(job, -2-nparams);
		// stack: closure, nparams*objects
		dbg->message("script_vm_t::intern_prepare_queued", "stack=%d nparams=%d ret=%d", sq_gettop(job), nparams, retvalue);

		END_STACK_WATCH(job, nparams+1);
		return true;
	}
	else {
		// no suspended jobs: clean stack
		sq_pop(job, 2);
	}
	END_STACK_WATCH(job, 0);
	return false;
}


/**
 * Prepare function call.
 * Stack(vm): pushes registry, nret, closure, root.
 */
bool script_vm_t::intern_prepare_pending_callback(const char* function, sint32 nret)
{
	BEGIN_STACK_WATCH(vm);
	sq_pushregistrytable(vm);
	script_api::param<sint32>::push(vm, nret+1); // +1 to account for root table
	sq_pushstring(vm, function, -1);
	bool ok = SQ_SUCCEEDED(sq_get(vm, -3));
	if (ok) {
		// stack: registry, nret, closure, root
		sq_pushroottable(vm);
	}
	else {
		// stack: clear
		sq_pop(vm, 2);
	}
	END_STACK_WATCH(vm, ok ? 4 : 0);
	return ok;
}

/**
 * Sets registry.pending_callback.
 * Stack(vm): expects registry, nret, closure, nparams*objects; clears upon exit.
 */
void script_vm_t::intern_store_pending_callback(sint32 nparams)
{
	BEGIN_STACK_WATCH(vm);
	// stack: registry, nret, closure, nparams*objects
	// put closure and parameters in array
	sq_newarray(vm, 0);
	// stack: registry, nret, closure, nparams*objects, array
	for(int i=0; i<nparams+2; i++) {
		sq_push(vm, -2);
		sq_arrayappend(vm, -2);
		sq_remove(vm, -2);
	}
	// stack: registry, array
	sq_pushstring(vm, "pending_callback", -1);
	sq_push(vm, -2);
	// stack: registry, array, string, array
	sq_createslot(vm, -4);
	sq_pop(vm, 2);
	END_STACK_WATCH(vm,-nparams-3);
}

/**
 * Deletes registry.pending_callback.
 * Stack(vm): unchanged
 */
void script_vm_t::clear_pending_callback()
{
	BEGIN_STACK_WATCH(vm);

	sq_pushregistrytable(vm);
	sq_pushstring(vm, "pending_callback", -1);
	sq_deleteslot(vm, -2, false);
	sq_poptop(vm);
	END_STACK_WATCH(vm,0);
}

/**
 * Sets registry.active_callbacks.
 * Deletes registry.queued_callbacks[0].
 * Stack(vm) unchanged.
 */
void script_vm_t::intern_make_queued_callback_active()
{
	BEGIN_STACK_WATCH(vm);
	sq_pushregistrytable(vm);
	sq_pushstring(vm, "queued_callbacks", -1);
	sq_get(vm, -2);
	sq_pushstring(vm, "active_callbacks", -1);
	sq_pushinteger(vm, 0);
	if (SQ_SUCCEEDED(sq_get(vm, -3))) {
		// stack: registry, queued_callbacks, "..", queued_callbacks[0]
		// active_callbacks = queued_callbacks[0]
		sq_createslot(vm, -4);
		// remove queued_callbacks[0]
		sq_arrayremove(vm, -1, 0);
	}
	else {
		sq_poptop(vm);
	}
	sq_pop(vm,2);
	END_STACK_WATCH(vm,0);
}

/**
 * Sets registry.active_callbacks.
 * Deletes registry.pending_callback.
 * Stack(vm) unchanged.
 */
void script_vm_t::intern_make_pending_callback_active()
{
	BEGIN_STACK_WATCH(vm);
	sq_pushregistrytable(vm);
	sq_pushstring(vm, "active_callbacks", -1);
	sq_newarray(vm, 1);
	sq_pushstring(vm, "pending_callback", -1);
	if (SQ_SUCCEEDED( sq_deleteslot(vm, -4, true) ) ) {
		// stack: registry, "..", array[], pending_callback
		sq_arrayappend(vm, -2);
		// stack: registry, "..", array[ 0=>pending_callback ]
		sq_createslot(vm, -3);
	}
	else {
		// stack: registry, "..", array[]
		sq_poptop(vm);
		// delete active_callbacks slot
		sq_deleteslot(vm, -2, false);
	}
	sq_poptop(vm);
	END_STACK_WATCH(vm,0);
}

/**
 *
 *
 * Stack(job): return value, unchanged
 */
void script_vm_t::intern_call_callbacks(HSQUIRRELVM job)
{
	BEGIN_STACK_WATCH(job);
	sq_pushregistrytable(job);
	sq_pushstring(job, "active_callbacks", -1);
	if (SQ_SUCCEEDED( sq_deleteslot(job, -2, true) ) ) {
		BEGIN_STACK_WATCH(job);
		if (SQ_SUCCEEDED( sq_arraypop(job, -1, true))) {
			if (SQ_SUCCEEDED(sq_arraypop(job, -1, true))) {
				sint32 nret = script_api::param<sint32>::get(job, -1);
				sq_poptop(job);
				BEGIN_STACK_WATCH(job);
				// stack: retval, registry, active_cb, active_cb[end]=array[ nparams*objects closure]
				int nparams = -1;
				while( SQ_SUCCEEDED(sq_arraypop(job, -2-nparams, true))) {
					nparams++;
					// replace parameter by return value
					if (nret > 0  &&  nret == nparams) {
						sq_poptop(job);
						sq_push(job, -4-nparams);
					}
				}
				// stack: retval, registry, active_cb, active_cb[end], closure, nparams*objects
				sq_call_restricted(job, nparams, false, true, 100);
				// remove closure and finished callback
				sq_pop(job, 1);
				END_STACK_WATCH(job,0);
			}
			sq_poptop(job);
		}
		END_STACK_WATCH(job,0);
		sq_poptop(job);
	}
	sq_poptop(job);
	END_STACK_WATCH(job,0);
}
