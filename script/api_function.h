/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SCRIPT_API_FUNCTION_H
#define SCRIPT_API_FUNCTION_H


#include "api_param.h"
#include "../squirrel/squirrel.h"
#include <string>
#include <string.h>

// mark class members static in doxygen documentation of api
#define STATIC

/** @file api_function.h templates to export c++ function to and call from squirrel */

namespace script_api {

	// forward declarations
	template<typename F> SQInteger generic_squirrel_callback(HSQUIRRELVM vm);
	template<typename F> struct embed_call_t;
	template<typename F> struct func_signature_t;

	/// @{
	/// @name Function to log squirrel-side type of exported functions
	void start_squirrel_type_logging(const char* suffix);

	void end_squirrel_type_logging();

	// sets current class name, does not work with nested classes
	void set_squirrel_type_class(const char* classname);

	void log_squirrel_type(std::string classname, const char* name, std::string squirrel_type);
	/// @}

	/**
	 * Registers custom SQFUNCTION @p funcptr.
	 *
	 * @pre Assumes that there is a table/class at the top of the stack to
	 *      attach the function to it.
	 *
	 * @param funcptr function pointer to custom implementation
	 * @param name name of the method as visible from squirrel
	 * @param nparamcheck for squirrel parameter checking
	 * @param typemask for squirrel parameter checking
	 * @param staticmethod if true then register as static method
	 */
	void register_function(HSQUIRRELVM vm, SQFUNCTION funcptr, const char *name, int nparamcheck, const char* typemask, bool staticmethod = false);

	/**
	 * Registers custom SQFUNCTION @p funcptr.
	 *
	 * Same as non-templated register_function. Typemask und paramcheck will be deduced from the template parameter.
	 *
	 * @tparam F function pointer signature of c++ method
	 * @param funcptr function pointer to custom implementation
	 * @param name name of the method as visible from squirrel
	 * @param staticmethod if true then register as static method
	 */
	template<class F>
	void register_function(HSQUIRRELVM vm, SQFUNCTION funcptr, const char *name, bool staticmethod = false)
	{
		std::string typemask = func_signature_t<F>::get_typemask(false);
		int nparamcheck = func_signature_t<F>::get_nparams();

		register_function(vm, funcptr, name, nparamcheck, typemask.c_str(), staticmethod);

		log_squirrel_type(func_signature_t<F>::get_squirrel_class(false), name, func_signature_t<F>::get_squirrel_type(false, 0));
	}


	/**
	 * Registers custom SQFUNCTION @p funcptr with templated free variables (default parameters).
	 *
	 * @pre Assumes that there is a table/class at the top of the stack to
	 *      attach the function to it.
	 *
	 * @tparam F type for free variables, @see freevariable
	 * @param funcptr function pointer to custom implementation
	 * @param name name of the method as visible from squirrel
	 * @param nparamcheck for squirrel parameter checking
	 * @param typemask for squirrel parameter checking
	 * @param freevariables contains values of free variables
	 * @param staticmethod if true then register as static method
	 */
	template<class F>
	void register_function_fv(HSQUIRRELVM vm, SQFUNCTION funcptr, const char *name, int nparamcheck, const char* typemask, F const& freevariables, bool staticmethod = false)
	{
		sq_pushstring(vm, name, -1);
		SQInteger count = freevariables.push(vm);
		sq_newclosure(vm, funcptr, count); //create a new function
		sq_setnativeclosurename(vm, -1, name);
		sq_setparamscheck(vm, nparamcheck, typemask);
		sq_newslot(vm, -3, staticmethod);
	}
	// auxiliary struct
	template <typename F>
	struct function_info_t {
		F funcptr; // pointer to c++ function
		bool discard_first;
		function_info_t(F f, bool d) : funcptr(f), discard_first(d) {}
	};

	/**
	 * Registers native c++ method to be called from squirrel.
	 * Squirrel calls generic_squirrel_callback, the pointer to the actual
	 * c++ function is attached in form of userdata.
	 *
	 * @pre Assumes that there is a table/class at the top of the stack to
	 *      attach the function to it.
	 *
	 * @tparam F function pointer signature of c++ method
	 * @param funcptr pointer to the c++ method
	 * @param name name of the method as visible from squirrel
	 * @param discard_first if true then a global (non-member) function can be called
	 *                      as if it would be a member function of the class instance
	 *                      provided as first argument
	 * @param staticmethod if true then register as static method
	 */
	template<typename F>
	void register_method(HSQUIRRELVM vm, F funcptr, const char* name, bool discard_first = false, bool staticmethod = false)
	{
		sq_pushstring(vm, name, -1);
		// pointer to function info as free variable
		function_info_t<F> fi(funcptr, discard_first);
		SQUserPointer up = sq_newuserdata(vm, sizeof(function_info_t<F>));
		memcpy(up, &fi, sizeof(function_info_t<F>));
		// create function
		sq_newclosure(vm, generic_squirrel_callback<F>, 1);
		// ..associate its name for debugging
		sq_setnativeclosurename(vm, -1, name);
		std::string typemask = func_signature_t<F>::get_typemask(discard_first);
		sq_setparamscheck(vm, func_signature_t<F>::get_nparams() - discard_first, typemask.c_str());
		sq_newslot(vm, -3, staticmethod);

		log_squirrel_type(func_signature_t<F>::get_squirrel_class(discard_first), name, func_signature_t<F>::get_squirrel_type(discard_first, 0));
	}


	/**
	 * Registers native c++ method to be called from squirrel.
	 * Squirrel calls generic_squirrel_callback, the pointer to the actual
	 * c++ function is attached in form of userdata.
	 *
	 * @pre Assumes that there is a table/class at the top of the stack to
	 *      attach the function to it.
	 *
	 * @tparam F function pointer signature of c++ method
	 * @tparam V class to push default parameters as free variables
	 * @param funcptr pointer to the c++ method
	 * @param name name of the method as visible from squirrel
	 * @param freevariables values of default parameters to the c++ function (not the squirrel function)
	 * @param discard_first if true then a global (non-member) function can be called
	 *                      as if it would be a member function of the class instance
	 *                      provided as first argument
	 * @param staticmethod if true then register as static method
	 */
	template<typename F, class V>
	void register_method_fv(HSQUIRRELVM vm, F funcptr, const char* name, V const& freevariables, bool discard_first = false, bool staticmethod = false)
	{
		sq_pushstring(vm, name, -1);
		// pointer to function info as free variable
		function_info_t<F> fi(funcptr, discard_first);
		SQUserPointer up = sq_newuserdata(vm, sizeof(function_info_t<F>));
		memcpy(up, &fi, sizeof(function_info_t<F>));
		// more free variables
		SQInteger count = freevariables.push(vm);
		// create function
		sq_newclosure(vm, generic_squirrel_callback<F>, count+1);
		// ..associate its name for debugging
		sq_setnativeclosurename(vm, -1, name);
		std::string typemask = func_signature_t<F>::get_typemask(discard_first);
		sq_setparamscheck(vm, func_signature_t<F>::get_nparams() - discard_first - count, typemask.c_str());
		sq_newslot(vm, -3, staticmethod);

		log_squirrel_type(func_signature_t<F>::get_squirrel_class(discard_first), name, func_signature_t<F>::get_squirrel_type(discard_first, count));
	}


	/**
	 * Registers native c++ method to be called from squirrel.
	 * This allows to register a non-member function to be called as
	 * member function of squirrel classes: The c++ class is provided as
	 * first argument.
	 *
	 * @pre Assumes that there is a table/class at the top of the stack to
	 *      attach the function to it.
	 * @see register_method
	 *
	 * @tparam F function pointer signature of c++ method
	 * @param functptr pointer to the c++ method
	 * @param name name of the method as visible from squirrel
	 */
	template<typename F>
	void register_local_method(HSQUIRRELVM vm, F funcptr, const char* name)
	{
		register_method(vm, funcptr, name, true);
	}

	/**
	 * The general purpose callback method to be called from squirrel.
	 * @tparam F function pointer signature of c++ method
	 */
	template<typename F>
	SQInteger generic_squirrel_callback(HSQUIRRELVM vm)
	{
		SQUserPointer up = NULL;

		// get pointer to function
		function_info_t<F> fi(0,false);
		sq_getuserdata(vm, -1, &up, NULL);
		memcpy(&fi, up, sizeof(function_info_t<F>));

		// call the template that automatically fetches right number of parameters
		return embed_call_t<F>::call_function(vm, fi.funcptr, fi.discard_first);
	}

	/**
	 * Template to count the first parameter
	 * @tparam F count is one, except if F is void_t then it is zero
	 */
	template<typename F> struct count_first_param_t {
		static const uint16 count = 1;
	};
	template<> struct count_first_param_t<void_t> {
		static const uint16 count = 0;
	};

	/**
	 * Class to check first parameters to be not NULL.
	 * Actually do the check only for pointer types.
	 */
	template<typename T> struct param_chk_t {
		static bool is_null(T) { return false; }
	};
	template<typename T> struct param_chk_t<T*> {
		static bool is_null(T* t) { return t == NULL; }
	};

	/**
	 * Templates to generate squirrel typemasks,
	 * wrapped in a one-parameter template struct,
	 * which is specialized per function-signature.
	 */
	template<typename F> struct func_signature_t
	{
		/// @returns typemask taking into account class argument is taken from 0th or 1st parameter
		static std::string get_typemask(bool discard_first)
		{
			return (discard_first ? "" : param<typename embed_call_t<F>::sig_class>::typemask())
			+ get_param_typemask(get_nparams());
		}

		/// @returns typemask of parameters only
		static std::string get_param_typemask(uint16 nparams)
		{
			if (nparams > 0) {
				if (count_first_param_t<typename embed_call_t<F>::sig_first>::count > 0) {
					// typemask of first parameter if there is any
					return param<typename embed_call_t<F>::sig_first>::typemask() +
						func_signature_t<typename embed_call_t<F>::sig_reduced>::get_param_typemask(nparams-1);
				}
				else {
					return  func_signature_t<typename embed_call_t<F>::sig_reduced>::get_param_typemask(nparams-1);
				}
			}
			return "";
		}

		/// @returns number of parameters
		static uint16 get_nparams()
		{
			// number of parameters of reduced signature
			return func_signature_t<typename embed_call_t<F>::sig_reduced>::get_nparams() +
			       // plus first parameter if there is any
			       count_first_param_t<typename embed_call_t<F>::sig_first>::count;
		}

		/// @returns squirrel class name of class argument of F ("void", "" are allowed)
		static std::string get_squirrel_class(bool discard_first)
		{
			if (discard_first) {
				return param<typename embed_call_t<F>::sig_first>::squirrel_type();
			}
			else {
				return param<typename embed_call_t<F>::sig_class>::squirrel_type();
			}
		}
		/// @returns function signature if interpreted as squirrel function
		static std::string get_squirrel_type(bool discard_first, int freevars)
		{
			// return type
			std::string r = param<typename embed_call_t<F>::sig_return>::squirrel_type();
			// parameters
			uint16 nparams = get_nparams();
			// if first parameter is interpreted as class ignore 2 parameters: environment, and first parameter
			if (nparams > freevars + 1 + discard_first) {
				// there are non-default params
				if (discard_first) {
					// first parameter is interpreted as class and thus ignored here
					return r + "("+ func_signature_t<typename embed_call_t<F>::sig_reduced>::get_param_squirrel_type(nparams - freevars - 2) + ")";
				}
				else {
					return r + "("+ get_param_squirrel_type(nparams - freevars - 1) + ")";
				}
			}
			return r + "()";
		}
		/// @returns type list of arguments (without parentheses and return type)
		static std::string get_param_squirrel_type(uint16 nparams)
		{
			if (nparams > 1) {
				return std::string(param<typename embed_call_t<F>::sig_first>::squirrel_type()) + ", "
				     + func_signature_t<typename embed_call_t<F>::sig_reduced>::get_param_squirrel_type(nparams-1);
			}
			else if (nparams > 0) {
				return param<typename embed_call_t<F>::sig_first>::squirrel_type();
			}
			return "";
		}
	};
	template<>
	struct func_signature_t<void(*)()> {
		static uint16 get_nparams() {
			// first parameter is environment / root table
			return 1;
		}
		static std::string get_typemask(bool discard_first)
		{
			// first parameter is environment / root table if discard_first==false
			// otherwise first parameter is ignored
			return (discard_first ? "" : ".");
		}
		static std::string get_param_typemask(uint16)
		{
			return "";
		}
		static std::string get_param_squirrel_type(uint16)
		{
			return "";
		}
	};

	/**
	 * Templates to call functions with automatically fetching the right parameters,
	 * wrapped in a one-parameter template struct,
	 * which is specialized per function-signature.
	 */
	template<typename F> struct embed_call_t;

	// 0 parameters
	template<typename R>
	struct embed_call_t<R (*)()> {
		static SQInteger call_function(HSQUIRRELVM vm, R (*func)(), bool)
		{
			return param<R>::push(vm, (*func)() );
		}

		typedef R         sig_return;  // return type
		typedef void_t    sig_class;   // type of class
		typedef void_t    sig_first;   // type of first parameter
		typedef void(*sig_reduced)();  // signature of function with without return type, class, and first parameter
	};

	template<class C, typename R>
	struct embed_call_t<R (C::*)()> {
		static SQInteger call_function(HSQUIRRELVM vm, R (C::*func)(), bool)
		{
			if (C* instance = param<C*>::get(vm, 1)) {
				return param<R>::push(vm, (instance->*func)() );
			}
			else {
				return -1;
			}
		}

		typedef R         sig_return;  // return type
		typedef C*        sig_class;   // type of class
		typedef void_t    sig_first;   // type of first parameter
		typedef void(*sig_reduced)();  // signature of function with without return type, class, and first parameter
	};
	template<class C, typename R>
	struct embed_call_t<R (C::*)() const> {
		static SQInteger call_function(HSQUIRRELVM vm, R (C::*func)() const, bool)
		{
			if (const C* instance = param<const C*>::get(vm, 1)) {
				return param<R>::push(vm, (instance->*func)() );
			}
			else {
				return -1;
			}
		}

		typedef R         sig_return;  // return type
		typedef C*        sig_class;   // type of class
		typedef void_t    sig_first;   // type of first parameter
		typedef void(*sig_reduced)();  // signature of function with without return type, class, and first parameter
	};
	template<class C>
	struct embed_call_t<void (C::*)()> {
		static SQInteger call_function(HSQUIRRELVM vm, void (C::*func)(), bool)
		{
			if (C* instance = param<C*>::get(vm, 1)) {
				(instance->*func)();
				return 0;
			}
			else {
				return -1;
			}
		}

		typedef void_t    sig_return;  // return type
		typedef C*        sig_class;   // type of class
		typedef void_t    sig_first;   // type of first parameter
		typedef void(*sig_reduced)();  // signature of function with without return type, class, and first parameter
	};
	// 1 parameter
	template<typename R, typename A1>
	struct embed_call_t<R (*)(A1)> {
		static SQInteger call_function(HSQUIRRELVM vm, R (*func)(A1), bool discard_first)
		{
			A1 a1 = param<A1>::get(vm, 2-discard_first);
			if (discard_first  &&  param_chk_t<A1>::is_null(a1)) {
				return -1;
			}
			else {
				return param<R>::push(vm, (*func)(a1) );
			}
		}

		typedef R         sig_return;  // return type
		typedef void_t    sig_class;   // type of class
		typedef A1        sig_first;   // type of first parameter
		typedef void(*sig_reduced)();  // signature of function with without return type, class, and first parameter
	};
	template<class C, typename A1>
	struct embed_call_t<void (C::*)(A1)> {
		static SQInteger call_function(HSQUIRRELVM vm, void (C::*func)(A1), bool)
		{
			if (C* instance = param<C*>::get(vm, 1)) {
				(instance->*func)( param<A1>::get(vm, 2) );
				return 0;
			}
			else {
				return -1;
			}
		}

		typedef void_t    sig_return;  // return type
		typedef C*        sig_class;   // type of class
		typedef A1        sig_first;   // type of first parameter
		typedef void(*sig_reduced)();  // signature of function with without return type, class, and first parameter
	};
	template<class C, typename R, typename A1>
	struct embed_call_t<R (C::*)(A1)> {
		static SQInteger call_function(HSQUIRRELVM vm, R (C::*func)(A1), bool)
		{
			if (C* instance = param<C*>::get(vm, 1)) {
				return param<R>::push(vm, (instance->*func)( param<A1>::get(vm, 2) ) );
			}
			else {
				return -1;
			}
		}

		typedef R         sig_return;  // return type
		typedef C*        sig_class;   // type of class
		typedef A1        sig_first;   // type of first parameter
		typedef void(*sig_reduced)();  // signature of function with without return type, class, and first parameter
	};
	template<class C, typename R, typename A1>
	struct embed_call_t<R (C::*)(A1) const> {
		static SQInteger call_function(HSQUIRRELVM vm, R (C::*func)(A1) const, bool)
		{
			if (const C* instance = param<const C*>::get(vm, 1)) {
				return param<R>::push(vm, (instance->*func)( param<A1>::get(vm, 2) ) );
			}
			else {
				return -1;
			}
		}

		typedef R         sig_return;  // return type
		typedef C*        sig_class;   // type of class
		typedef A1        sig_first;   // type of first parameter
		typedef void(*sig_reduced)();  // signature of function with without return type, class, and first parameter
	};
	// 2 parameters
	template<typename A1, typename A2>
	struct embed_call_t<void (*)(A1, A2)> {
		typedef A1          sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2);  // signature of function with without return type, class, and first parameter
	};

	template<typename R, typename A1, typename A2>
	struct embed_call_t<R (*)(A1, A2)> {
		static SQInteger call_function(HSQUIRRELVM vm, R (*func)(A1, A2), bool discard_first)
		{
			A1 a1 = param<A1>::get(vm, 2-discard_first);
			if (discard_first  &&  param_chk_t<A1>::is_null(a1)) {
				return -1;
			}
			return param<R>::push(vm, (*func)(a1,
				param<A2>::get(vm, 3-discard_first)
			) );
		}

		typedef R           sig_return;  // return type
		typedef void_t      sig_class;   // type of class
		typedef A1          sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2);  // signature of function with without return type, class, and first parameter
	};
	template<class C, typename A1, typename A2>
	struct embed_call_t<void (C::*)(A1, A2)> {
		static SQInteger call_function(HSQUIRRELVM vm, void (C::*func)(A1, A2), bool)
		{
			if (C* instance = param<C*>::get(vm, 1)) {
				(instance->*func)(
					param<A1>::get(vm, 2),
					param<A2>::get(vm, 3)
				);
				return 0;
			}
			else {
				return -1;
			}
		}

		typedef void_t      sig_return;  // return type
		typedef C*          sig_class;   // type of class
		typedef A1          sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2);  // signature of function with without return type, class, and first parameter
	};
	template<class C, typename R, typename A1, typename A2>
	struct embed_call_t<R (C::*)(A1, A2)> {
		static SQInteger call_function(HSQUIRRELVM vm, R (C::*func)(A1, A2), bool)
		{
			if (C* instance = param<C*>::get(vm, 1)) {
				return param<R>::push(vm, (instance->*func)(
					param<A1>::get(vm, 2),
					param<A2>::get(vm, 3)
				) );
			}
			else {
				return -1;
			}
		}

		typedef R           sig_return;  // return type
		typedef C*          sig_class;   // type of class
		typedef A1          sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2);  // signature of function with without return type, class, and first parameter
	};
	template<class C, typename R, typename A1, typename A2>
	struct embed_call_t<R (C::*)(A1, A2) const> {
		static SQInteger call_function(HSQUIRRELVM vm, R (C::*func)(A1, A2) const, bool)
		{
			if (const C* instance = param<const C*>::get(vm, 1)) {
				return param<R>::push(vm, (instance->*func)(
					param<A1>::get(vm, 2),
					param<A2>::get(vm, 3)
				) );
			}
			else {
				return -1;
			}
		}

		typedef R           sig_return;  // return type
		typedef C*          sig_class;   // type of class
		typedef A1          sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2);  // signature of function with without return type, class, and first parameter
	};

	// 3 parameters
	template<typename A1, typename A2, typename A3>
	struct embed_call_t<void (*)(A1, A2, A3)> {
		typedef A1             sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2,A3);  // signature of function with without return type, class, and first parameter
	};

	template<typename R, typename A1, typename A2, typename A3>
	struct embed_call_t<R (*)(A1, A2, A3)> {
		static SQInteger call_function(HSQUIRRELVM vm, R (*func)(A1, A2, A3), bool discard_first)
		{
			A1 a1 = param<A1>::get(vm, 2-discard_first);
			if (discard_first  &&  param_chk_t<A1>::is_null(a1)) {
				return -1;
			}
			return param<R>::push(vm, (*func)(a1,
				param<A2>::get(vm, 3-discard_first),
				param<A3>::get(vm, 4-discard_first)
			) );
		}

		typedef R              sig_return;  // return type
		typedef void_t         sig_class;   // type of class
		typedef A1             sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2,A3);  // signature of function with without return type, class, and first parameter
	};
	template<class C, typename A1, typename A2, typename A3>
	struct embed_call_t<void (C::*)(A1, A2, A3)> {
		static SQInteger call_function(HSQUIRRELVM vm, void (C::*func)(A1, A2, A3), bool)
		{
			if (C* instance = param<C*>::get(vm, 1)) {
				(instance->*func)(
					param<A1>::get(vm, 2),
					param<A2>::get(vm, 3),
					param<A3>::get(vm, 4)
				);
				return 0;
			}
			else {
				return -1;
			}
		}

		typedef void_t         sig_return;  // return type
		typedef C*             sig_class;   // type of class
		typedef A1             sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2,A3);  // signature of function with without return type, class, and first parameter
	};

	// 4 parameters
	template<typename A1, typename A2, typename A3, typename A4>
	struct embed_call_t<void (*)(A1, A2, A3, A4)> {
		typedef A1                sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2,A3,A4);  // signature of function with without return type, class, and first parameter
	};


	template<typename R, typename A1, typename A2, typename A3, typename A4>
	struct embed_call_t<R (*)(A1, A2, A3, A4)> {
		static SQInteger call_function(HSQUIRRELVM vm, R (*func)(A1, A2, A3, A4), bool discard_first)
		{
			A1 a1 = param<A1>::get(vm, 2-discard_first);
			if (discard_first  &&  param_chk_t<A1>::is_null(a1)) {
				return -1;
			}
			return param<R>::push(vm, (*func)(a1,
				param<A2>::get(vm, 3-discard_first),
				param<A3>::get(vm, 4-discard_first),
				param<A4>::get(vm, 5-discard_first)
			) );
		}

		typedef R              sig_return;  // return type
		typedef void_t         sig_class;   // type of class
		typedef A1             sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2,A3,A4);  // signature of function with without return type, class, and first parameter
	};

	template<class C, typename A1, typename A2, typename A3, typename A4>
	struct embed_call_t<void (C::*)(A1, A2, A3, A4)> {
		static SQInteger call_function(HSQUIRRELVM vm, void (C::*func)(A1, A2, A3, A4), bool)
		{
			if (C* instance = param<C*>::get(vm, 1)) {
				(instance->*func)(
					param<A1>::get(vm, 2),
					param<A2>::get(vm, 3),
					param<A3>::get(vm, 4),
					param<A4>::get(vm, 5)
				);
				return 0;
			}
			else {
				return -1;
			}
		}

		typedef void_t            sig_return;  // return type
		typedef C*                sig_class;   // type of class
		typedef A1                sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2,A3,A4);  // signature of function with without return type, class, and first parameter
	};

	// 5 parameters
	template<typename A1, typename A2, typename A3, typename A4, typename A5>
	struct embed_call_t<void (*)(A1, A2, A3, A4, A5)> {
		static SQInteger call_function(HSQUIRRELVM vm, void (*func)(A1, A2, A3, A4, A5), bool discard_first)
		{
			A1 a1 = param<A1>::get(vm, 2-discard_first);
			if (discard_first  &&  param_chk_t<A1>::is_null(a1)) {
				return -1;
			}
			(*func)(a1,
				param<A2>::get(vm, 3-discard_first),
				param<A3>::get(vm, 4-discard_first),
				param<A4>::get(vm, 5-discard_first),
				param<A5>::get(vm, 6-discard_first)
			);
			return 0;
		}

		typedef void_t         sig_return;  // return type
		typedef void_t         sig_class;   // type of class
		typedef A1             sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2,A3,A4,A5);  // signature of function with without return type, class, and first parameter
	};

	template<typename R, typename A1, typename A2, typename A3, typename A4, typename A5>
	struct embed_call_t<R (*)(A1, A2, A3, A4, A5)> {
		static SQInteger call_function(HSQUIRRELVM vm, R (*func)(A1, A2, A3, A4, A5), bool discard_first)
		{
			A1 a1 = param<A1>::get(vm, 2-discard_first);
			if (discard_first  &&  param_chk_t<A1>::is_null(a1)) {
				return -1;
			}
			return param<R>::push(vm, (*func)(a1,
				  param<A2>::get(vm, 3-discard_first),
				  param<A3>::get(vm, 4-discard_first),
				  param<A4>::get(vm, 5-discard_first),
				  param<A5>::get(vm, 6-discard_first)
			));
			return 0;
		}

		typedef R              sig_return;  // return type
		typedef void_t         sig_class;   // type of class
		typedef A1             sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2,A3,A4,A5);  // signature of function with without return type, class, and first parameter
	};

	template<class C, typename A1, typename A2, typename A3, typename A4, typename A5>
	struct embed_call_t<void (C::*)(A1, A2, A3, A4, A5)> {
		static SQInteger call_function(HSQUIRRELVM vm, void (C::*func)(A1, A2, A3, A4, A5), bool)
		{
			if (C* instance = param<C*>::get(vm, 1)) {
				(instance->*func)(
					param<A1>::get(vm, 2),
					param<A2>::get(vm, 3),
					param<A3>::get(vm, 4),
					param<A4>::get(vm, 5),
					param<A5>::get(vm, 6)
				);
				return 0;
			}
			else {
				return -1;
			}
		}

		typedef void_t               sig_return;  // return type
		typedef C*                   sig_class;   // type of class
		typedef A1                   sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2,A3,A4,A5);  // signature of function with without return type, class, and first parameter
	};

	// 6 parameters
	template<typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
	struct embed_call_t<R (*)(A1, A2, A3, A4, A5, A6)> {
		static SQInteger call_function(HSQUIRRELVM vm, R (*func)(A1, A2, A3, A4, A5, A6), bool discard_first)
		{
			A1 a1 = param<A1>::get(vm, 2-discard_first);
			if (discard_first  &&  param_chk_t<A1>::is_null(a1)) {
				return -1;
			}
			return param<R>::push(vm, (*func)(a1,
											  param<A2>::get(vm, 3-discard_first),
											  param<A3>::get(vm, 4-discard_first),
											  param<A4>::get(vm, 5-discard_first),
											  param<A5>::get(vm, 6-discard_first),
											  param<A6>::get(vm, 7-discard_first)
			));
			return 0;
		}

		typedef R              sig_return;  // return type
		typedef void_t         sig_class;   // type of class
		typedef A1             sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2,A3,A4,A5,A6);  // signature of function with without return type, class, and first parameter
	};
	template<class C, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
	struct embed_call_t<void (C::*)(A1, A2, A3, A4, A5, A6)> {
		static SQInteger call_function(HSQUIRRELVM vm, void (C::*func)(A1, A2, A3, A4, A5, A6), bool)
		{
			if (C* instance = param<C*>::get(vm, 1)) {
				(instance->*func)(
					param<A1>::get(vm, 2),
					param<A2>::get(vm, 3),
					param<A3>::get(vm, 4),
					param<A4>::get(vm, 5),
					param<A5>::get(vm, 6),
					param<A6>::get(vm, 7)
				);
				return 0;
			}
			else {
				return -1;
			}
		}

		typedef void_t                  sig_return;  // return type
		typedef C*                      sig_class;   // type of class
		typedef A1                      sig_first;   // type of first parameter
		typedef void(*sig_reduced)(A2,A3,A4,A5,A6);  // signature of function with without return type, class, and first parameter
	};


	/**
	 * Exports function to check whether pointer to in-game object is not null
	 */
	template<class P> SQInteger is_ptr_valid(HSQUIRRELVM vm)
	{
		P ptr = param<P>::get(vm, 1);
		sq_pushbool(vm, ptr != NULL);
		return 1;
	}
	template<class P> void export_is_valid(HSQUIRRELVM vm)
	{
		register_function(vm, is_ptr_valid<P>, "is_valid", 1, param<P>::typemask());
		log_squirrel_type(param<P>::squirrel_type(), "is_valid", "bool()");
	}

}; // end of namespace
#endif
