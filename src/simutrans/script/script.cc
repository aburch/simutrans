/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "script.h"

#include <stdarg.h>
#include <string.h>
#include "../../squirrel/sqstdaux.h" // for error handlers
#include "../../squirrel/sqstdio.h" // for loadfile
#include "../../squirrel/sqstdstring.h" // export for scripts
#include "../../squirrel/sqstdmath.h" // export for scripts
#include "../../squirrel/sqstdsystem.h" // export for scripts
#include "../../squirrel/sq_extensions.h" // for sq_call_restricted

#include "../utils/log.h"

#include "../tpl/inthashtable_tpl.h"
#include "../tpl/vector_tpl.h"
// for error popups
#include "../gui/help_frame.h"
#include "../gui/simwin.h"
#include "../utils/cbuffer.h"
#include "../utils/plainstring.h"


namespace script_api {
	bool pause_game();  // api_control.cc
}

// debug: store stack pointer
#define BEGIN_STACK_WATCH(v) int stack_top = sq_gettop(v);
// debug: compare stack pointer with expected stack pointer, raise warning if failure
#define END_STACK_WATCH(v, delta) if ( (stack_top+(delta)) != sq_gettop(v)) { dbg->warning( __FUNCTION__, "(%d) stack in %d expected %d out %d", __LINE__,stack_top,stack_top+(delta),sq_gettop(v)); }


void script_vm_t::printfunc(HSQUIRRELVM vm, const SQChar *s, ...)
{
	va_list vl;
	va_start(vl, s);
	if (script_vm_t *script = (script_vm_t*)sq_getforeignptr(vm)) {
		script->log->vmessage("Script", "Print", s, vl);
	}
	va_end(vl);
}


void script_vm_t::errorfunc(HSQUIRRELVM vm, const SQChar *s_, ...)
{

	char *s = strdup(s_);
	char *s_dup = s;
	// remove these linebreaks
	while (s  &&  *s=='\n') s++;
	while (s  &&  s[strlen(s)-1]=='\n') s[strlen(s)-1]=0;

	// get failed script
	script_vm_t *script = (script_vm_t*)sq_getforeignptr(vm);

	if (script) {
		va_list vl;
		va_start(vl, s_);
		script->log->vmessage("Script", "Error", s, vl);
		va_end(vl);
	}

	// only one error message at a time - hopefully
	// collect into static buffer
	static cbuffer_t buf;
	if (strcmp(s, "<error>")==0) {
		// start of error message
		buf.clear();
		buf.printf("<st>An error occurred within a script!</st><br>\n");
	}
	else if (strcmp(s, "</error>")==0) {
		// end of error message
		help_frame_t *win = dynamic_cast<help_frame_t*>(win_get_magic(magic_script_error));
		if (win == NULL) {
			win = new help_frame_t();
			create_win( win, w_info, magic_script_error);
		}
		win->set_text(buf);
		win->set_name("Script error occurred");

		if (script) {
			script->set_error(buf);
			// pause game
			if (script->pause_on_error) {
				script_api::pause_game();
			}
		}
	}
	else {
		va_list vl;
		va_start(vl, s_);
		buf.vprintf(s, vl);
		va_end(vl);
		buf.append("<br>");
	}
	free(s_dup);
}

void export_include(HSQUIRRELVM vm, const char* include_path); // api_include.cc

// virtual machine
script_vm_t::script_vm_t(const char* include_path_, const char* log_name)
{
	pause_on_error = false;

	vm = sq_open(1024);
	sqstd_seterrorhandlers(vm);
	sq_setprintfunc(vm, printfunc, errorfunc);
	register_vm(vm);
	log = new log_t(log_name, true, true, true, "script engine started.\n");

	// store ptr to us in vm
	sq_setforeignptr(vm, this);

	// create thread, and put it into registry-table
	sq_pushregistrytable(vm);
	sq_pushstring(vm, "thread", -1);
	thread = sq_newthread(vm, 1024);
	sq_newslot(vm, -3, false);
	register_vm(thread);
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
	// store ptr to us in vm
	sq_setforeignptr(vm, this);
	sq_setforeignptr(thread, this);

	error_msg = NULL;
	include_path = include_path_;
	// register libraries
	sq_pushroottable(vm);
	sqstd_register_stringlib(vm);
	sqstd_register_mathlib(vm);
	sqstd_register_systemlib(vm);
	sq_pop(vm, 1);
	// export include command
	export_include(vm, include_path);
	// initialize coordinate and rotation handling
	script_api::coordinate_transform_t::initialize();
}


script_vm_t::~script_vm_t()
{
	unregister_vm(thread);
	unregister_vm(vm);
	// remove from suspended calls list
	suspended_scripts_t::remove_vm(thread);
	suspended_scripts_t::remove_vm(vm);
	// close vm, also closes thread
	sq_close(vm);

	delete log;
}

const char* script_vm_t::call_script(const char* filename)
{
	// load script
	if (!SQ_SUCCEEDED(sqstd_loadfile(vm, filename, true))) {
		return "Reading / compiling script failed";
	}
	// call it
	sq_pushroottable(vm);
	if (!SQ_SUCCEEDED(sq_call_restricted(vm, 1, SQFalse, SQTrue, 100000))) {
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
	if (squirrel_string == NULL) {
		return NULL;
	}
	HSQUIRRELVM &job = thread;
	// log string
	printfunc(job, "String to compile:\n%s\n<<<\n", squirrel_string);
	// compile string
	if (!SQ_SUCCEEDED(sq_compilebuffer(job, squirrel_string, strlen(squirrel_string), "userdefinedstringmethod", true))) {
		set_error("Error compiling string buffer");
		return get_error();
	}
	// execute
	sq_pushroottable(job);
	// stack: closure, root table (1st param)
	return intern_finish_call(job, QUEUE, 1, true);
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
		case FORCEX:
			job = vm;
			// block calls to suspendable functions
			sq_block_suspend(job, function);
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
	// only call the closure if vm is idle (maybe in RUN state)
	bool suspended = sq_getvmstate(job) != SQ_VMSTATE_IDLE;
	// check queue, if not empty resume first job in queue
	if (!suspended  &&  ct != FORCE  &&  ct != FORCEX) {
		sq_pushregistrytable(job);
		sq_pushstring(job, "queue", -1);
		sq_get(job, -2);
		// stack: registry, queue
		if (sq_getsize(job, -1) > 0) {
			suspended = true;
		}
		sq_pop(job, 2);
	}
	// queue function call?
	if (suspended  &&  ct == QUEUE) {
		intern_queue_call(job, nparams, retvalue);
		err = "suspended";
		// stack: clean
	}
	if (suspended  &&  ct != FORCE  &&  ct != FORCEX) {
		intern_resume_call(job);
	}
	if (!suspended  ||  ct == FORCE  ||  ct == FORCEX) {
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
	uint32 opcodes = ct == FORCEX ? 100000 : 10000;
	// call the script
	if (!SQ_SUCCEEDED(sq_call_restricted(job, nparams, retvalue, ct == FORCE  ||  ct == FORCEX, opcodes))) {
		err = "Call function failed";
		retvalue = false;
	}
	if (ct == FORCE  ||  ct == FORCEX) {
		sq_block_suspend(job, NULL);
	}
	if (sq_getvmstate(job) != SQ_VMSTATE_SUSPENDED  ||  ct == FORCE  ||  ct == FORCEX) {
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
	bool wait = false;
	script_api::get_slot(job, "wait_external", wait, -1);
	sq_pop(job, 1);
	END_STACK_WATCH(job, 0);

	if (wait) {
		dbg->message("script_vm_t::intern_resume_call", "waits for external call to be able to proceed");
		return;
	}
	if (!sq_canresumevm(job)) {
		// vm waits for return value to suspended call
		dbg->message("script_vm_t::intern_resume_call", "waiting for return value");
		return;
	}
	// vm suspended, but not from call to our methods
	if (nparams < 0) {
		retvalue = false;
	}

	// resume v.m.
	if (!SQ_SUCCEEDED(sq_resumevm(job, retvalue, 10000))) {
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
			sq_poptop(job);
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
		sq_newarray(job, 0);
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
	sq_newarray(vm, 0);
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


void script_vm_t::set_my_player(uint8 player_nr)
{
	sq_pushregistrytable(vm);
	script_api::create_slot(vm, "my_player_nr", player_nr);
	sq_poptop(vm);
}


/* -------- management of suspended scripts that wait for return value ----------- */

inthashtable_tpl<uint32,HSQUIRRELVM> suspended_scripts_t::suspended_scripts;


uint32 suspended_scripts_t::get_unique_key(void* ptr)
{
	uint32 key = (uint32)(size_t)ptr;
	while (key == 0  ||  suspended_scripts.get(key)) {
		key ++;
	}
	return key;
}


void suspended_scripts_t::register_suspended_script(uint32 key, HSQUIRRELVM vm)
{
	suspended_scripts.set(key, vm);

	// mark vm to wait for external call to allow for wake-up
	sq_pushregistrytable(vm);
	bool wait = true;
	script_api::create_slot(vm, "wait_external", wait);
	sq_poptop(vm);
}


HSQUIRRELVM suspended_scripts_t::remove_suspended_script(uint32 key)
{
	return suspended_scripts.remove(key);
}


void suspended_scripts_t::remove_vm(HSQUIRRELVM vm)
{
	inthashtable_tpl<uint32,HSQUIRRELVM>::iterator iter=suspended_scripts.begin(), end=suspended_scripts.end();
	for(; iter != end; ) {
		if ( (*iter).value == vm) {
			iter = suspended_scripts.erase(iter);
		}
		else {
			++iter;
		}
	}
}
