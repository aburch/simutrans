/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SCRIPT_SCRIPT_H
#define SCRIPT_SCRIPT_H


/** @file script.h handles the virtual machine, interface to squirrel */

#include "api_function.h"
#include "api_param.h"
#include "../simtypes.h"
#include "../squirrel/squirrel.h"
#include "../utils/plainstring.h"
#include <string>

class log_t;
template<class key_t, class value_t> class inthashtable_tpl;
void sq_setwakeupretvalue(HSQUIRRELVM v); //sq_extensions

/**
 * Class providing interface to squirrel's virtual machine.
 *
 * Logs output to script.log.
 * Opens error window in case of script errors.
 */
class script_vm_t {
public:
	script_vm_t(const char* include_path, const char* log_name);
	~script_vm_t();

	/**
	 * loads file, calls script
	 * @returns error msg (or NULL if succeeded)
	 */
	const char* call_script(const char* filename);

	/**
	 * compiles and executes given string
	 * @returns error msg (or NULL if succeeded)
	 */
	const char* eval_string(const char* squirrel_string);

	bool uses_vm(HSQUIRRELVM other) const { return vm == other  ||  thread == other; }

	const HSQUIRRELVM& get_vm() const { return vm; }

	const char* get_error() const { return error_msg.c_str(); }

	/**
	 * The script can only act as a certain player.
	 * @param player_nr the number of the player (PLAYER_UNOWNED for scenarios)
	 */
	void set_my_player(uint8 player_nr);

	/// priority of function call
	enum call_type_t {
		FORCE,   ///< function has to return, raise error if not
		FORCEX,  ///< function has to return, raise error if not, give more opcodes
		QUEUE,   ///< function call can be queued, return value can be propagated by call back
		TRY,     ///< function call will not be queued, if virtual machine is suspended just return
	};

	/**
	 * @param err error string returned by call_script
	 * @return whether the call was suspended
	 */
	static bool is_call_suspended(const char* err);

#	define prep_function_call() \
		HSQUIRRELVM job; \
		const char* err = intern_prepare_call(job, ct, function); \
		if (err) { \
			return err; \
		} \
		int nparam = 1;

#	define do_function_call() \
		err = intern_finish_call(job, ct, nparam, true); \
		if (err == NULL) { \
			ret = script_api::param<R>::get(job, -1); \
			sq_poptop(job); \
		} \
		return err;

	/**
	 * calls scripted function
	 * @param function function name of squirrel function
	 * @returns error msg (or NULL if succeeded)
	 */
	const char* call_function(call_type_t ct, const char* function) {
		prep_function_call();
		return intern_finish_call(job, ct, nparam, false);
	}

	/**
	 * calls scripted function
	 *
	 * @tparam R type of return value
	 * @param function function name of squirrel function
	 * @param ret return value of script function is stored here
	 * @returns error msg (or NULL if succeeded), if call was suspended ret is invalid
	 */
	template<class R>
	const char* call_function(call_type_t ct, const char* function, R& ret) {
		prep_function_call();
		do_function_call();
	}

	/**
	 * calls scripted function
	 *
	 * @tparam R type of return value
	 * @tparam A1 type of first argument
	 * @param function function name of squirrel function
	 * @param arg1 first argument passed to squirrel function
	 * @param ret return value of script function is stored here
	 * @returns error msg (or NULL if succeeded), if call was suspended ret is invalid
	 */
	template<class R, class A1>
	const char* call_function(call_type_t ct, const char* function, R& ret, A1 arg1) {
		prep_function_call();
		script_api::param<A1>::push(job, arg1); nparam++;
		do_function_call();
	}
	template<class R, class A1, class A2>
	const char* call_function(call_type_t ct, const char* function, R& ret, A1 arg1, A2 arg2) {
		prep_function_call();
		script_api::param<A1>::push(job, arg1); nparam++;
		script_api::param<A2>::push(job, arg2); nparam++;
		do_function_call();
	}
	template<class R, class A1, class A2, class A3>
	const char* call_function(call_type_t ct, const char* function, R& ret, A1 arg1, A2 arg2, A3 arg3) {
		prep_function_call();
		script_api::param<A1>::push(job, arg1); nparam++;
		script_api::param<A2>::push(job, arg2); nparam++;
		script_api::param<A3>::push(job, arg3); nparam++;
		do_function_call();
	}

	/**
	 * Registers a c++ function to be available as callback.
	 * A callback is called when a function call got suspended, resumed, and returned something.
	 * @tparam F function signature
	 * @param F function pointer
	 * @param name register callback under this name
	 */
	template<typename F>
	void register_callback(F funcptr, const char* name)
	{
		sq_pushregistrytable(vm);
		script_api::register_method(vm, funcptr, name);
		sq_poptop(vm);
	}

	/**
	 * Prepares a callback call.
	 * @param function name of function to be called as callback
	 * @param nret the nret-th parameter will be replaced by return value of suspended function.
	 */
	template<class A1, class A2>
	void prepare_callback(const char* function, int nret, A1 arg1, A2 arg2) {
		if (intern_prepare_pending_callback(function, nret))  {
			script_api::param<A1>::push(vm, arg1);
			script_api::param<A2>::push(vm, arg2);
			intern_store_pending_callback(3);
		}
	}

	template<class A1, class A2, class A3>
	void prepare_callback(const char* function, int nret, A1 arg1, A2 arg2, A3 arg3) {
		if (intern_prepare_pending_callback(function, nret))  {
			script_api::param<A1>::push(vm, arg1);
			script_api::param<A2>::push(vm, arg2);
			script_api::param<A3>::push(vm, arg3);
			intern_store_pending_callback(4);
		}
	}

	/**
	 * Clears pending callback initialized by prepare_callback.
	 */
	void clear_pending_callback();

private:
	/// virtual machine running everything
	HSQUIRRELVM vm;

	/// thread in the virtual machine, used to run functions that can be suspended
	HSQUIRRELVM thread;

	/// our log file
	log_t* log;

	plainstring error_msg;

	/// path to files to #include
	plainstring include_path;

public:
	bool pause_on_error;

private:
	/// @{
	/// @name Helper functions to call, suspend, queue calls to scripted functions

	/// prepare function call, used in templated call_function(), sets job to vm that should run function
	const char* intern_prepare_call(HSQUIRRELVM &job, call_type_t ct, const char* function);

	/// actually call function, used in templated call_function(),
	/// does also: resume suspended call, queue current call
	const char* intern_finish_call(HSQUIRRELVM job, call_type_t ct, int nparams, bool retvalue);

	/// queues current call
	static void intern_queue_call(HSQUIRRELVM job, int nparams, bool retvalue);

	/// resumes a suspended call. calls callbacks.
	void intern_resume_call(HSQUIRRELVM job);

	/// calls function. If it was a queued call, also calls callbacks.
	static const char* intern_call_function(HSQUIRRELVM job, call_type_t ct, int nparams, bool retvalue);

	/// pops an queued call and puts it on the stack, also activates corresponding callbacks
	bool intern_prepare_queued(HSQUIRRELVM job, int &nparams, bool &retvalue);

	/// prepare function call to callback, used in templated prepare_callback()
	bool intern_prepare_pending_callback(const char* function, sint32 nret);

	/// saves the callback call (with arguments), used in templated prepare_callback()
	void intern_store_pending_callback(sint32 nparams);

	/// pops callback from queued callbacks, and makes it active
	void intern_make_queued_callback_active();

	/// takes pending callback, and makes it active
	void intern_make_pending_callback_active();

	/// calls all active callbacks
	static void intern_call_callbacks(HSQUIRRELVM job);

	/// @}

	/// custom error handler for compile and runtime errors of squirrel scripts
	static void errorfunc(HSQUIRRELVM vm, const SQChar *s_,...);

	/// set error message, used in errorhandlers
	void set_error(const char* error) { error_msg = error; }

	/// custom print handler
	static void printfunc(HSQUIRRELVM, const SQChar *s, ...);
};

/**
 * Class to manage all vm's that are suspended and waiting for the return of
 * a call to a tool.
 */
class suspended_scripts_t {
private:
	static inthashtable_tpl<uint32,HSQUIRRELVM> suspended_scripts;

	static HSQUIRRELVM remove_suspended_script(uint32 key);

public:
	// generates key from a pointer
	static uint32 get_unique_key(void* key);

	static void register_suspended_script(uint32 key, HSQUIRRELVM vm);

	// remove any reference to given vm
	static void remove_vm(HSQUIRRELVM vm);

	template<class R>
	static void tell_return_value(uint32 key, R& ret)
	{
		HSQUIRRELVM vm = remove_suspended_script(key);
		if (vm) {
			script_api::param<R>::push(vm, ret);
			sq_setwakeupretvalue(vm);
			// this vm can be woken up now
			sq_pushregistrytable(vm);
			bool wait = false;
			script_api::create_slot(vm, "wait_external", wait);
			sq_poptop(vm);
		}
	}
};

#endif
