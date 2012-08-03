#ifndef _SCRIPT_H_
#define _SCRIPT_H_

/** @file script.h handles the virtual machine, interface to squirrel */

#include "api_param.h"
#include "../simtypes.h"
#include "../squirrel/squirrel.h"
#include "../utils/plainstring.h"
#include <string>

/**
 * Class providing interface to squirrel's virtual machine.
 *
 * Logs output to script.log.
 * Opens error window in case of script errors.
 */
class script_vm_t {
public:
	script_vm_t();
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

	const HSQUIRRELVM& get_vm() const { return vm; }


	/// @return if script has failed, i.e. runtime error in script
	bool has_failed() const { return failed; }

	const char* get_error() const { return error_msg.c_str(); }


#	define prep_function_call() \
		const char* err = intern_start_function(function); \
		if (err) { \
			return err; \
		} \
		int nparam = 1;

#	define do_function_call() \
		err = intern_call_function(nparam, true); \
		if (err == NULL) { \
			ret = script_api::param<R>::get(vm, -1); \
			sq_poptop(vm); \
		} \
		return NULL;

	/**
	 * calls scripted function
	 * @param function function name of squirrel function
	 * @returns error msg (or NULL if succeeded)
	 */
	const char* call_function(const char* function) {
		prep_function_call();
		return intern_call_function(nparam, false);
	}

	/**
	 * calls scripted function
	 *
	 * @tparam R type of return value
	 * @param function function name of squirrel function
	 * @param ret return value of script function is stored here
	 * @returns error msg (or NULL if succeeded)
	 */
	template<class R>
	const char* call_function(const char* function, R& ret) {
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
	 * @returns error msg (or NULL if succeeded)
	 */
	template<class R, class A1>
	const char* call_function(const char* function, R& ret, A1 arg1) {
		prep_function_call();
		script_api::param<A1>::push(vm, arg1); nparam++;
		do_function_call();
	}
	template<class R, class A1, class A2>
	const char* call_function(const char* function, R& ret, A1 arg1, A2 arg2) {
		prep_function_call();
		script_api::param<A1>::push(vm, arg1); nparam++;
		script_api::param<A1>::push(vm, arg2); nparam++;
		do_function_call();
	}
	template<class R, class A1, class A2, class A3>
	const char* call_function(const char* function, R& ret, A1 arg1, A2 arg2, A3 arg3) {
		prep_function_call();
		script_api::param<A1>::push(vm, arg1); nparam++;
		script_api::param<A2>::push(vm, arg2); nparam++;
		script_api::param<A3>::push(vm, arg3); nparam++;
		do_function_call();
	}


private:
	HSQUIRRELVM vm;

	bool failed;
	plainstring error_msg;

	/// prepare function call, used in templated call_function()
	const char* intern_start_function(const char* function);

	/// actually call function, used in templated call_function()
	const char* intern_call_function(int nparams, bool retvalue);

	/// custom error handler for compile and runtime errors of squirrel scripts
	static void errorfunc(HSQUIRRELVM vm, const SQChar *s_,...);

	/// mark script as failed, used in errorhandlers
	void script_failed() { failed = true; }

	/// set error message, used in errorhandlers
	void set_error(const char* error) { error_msg = error; }
};

#endif
