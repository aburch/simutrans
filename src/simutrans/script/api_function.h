/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SCRIPT_API_FUNCTION_H
#define SCRIPT_API_FUNCTION_H


#include "api_param.h"
#include "../../squirrel/squirrel.h"
#include <string>
#include <string.h>
#include <utility>
#include <functional>

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
		bool act_as_member;
		function_info_t(F f, bool d) : funcptr(f), act_as_member(d) {}
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
	 * @param act_as_member if true then a global (non-member) function can be called
	 *                      as if it would be a member function of the class instance
	 *                      provided as first argument
	 * @param staticmethod if true then register as static method
	 */
	template<typename F>
	void register_method(HSQUIRRELVM vm, F funcptr, const char* name, bool act_as_member = false, bool staticmethod = false)
	{
		sq_pushstring(vm, name, -1);
		// pointer to function info as free variable
		function_info_t<F> fi(funcptr, act_as_member);
		SQUserPointer up = sq_newuserdata(vm, sizeof(function_info_t<F>));
		memcpy(up, &fi, sizeof(function_info_t<F>));
		// create function
		sq_newclosure(vm, generic_squirrel_callback<F>, 1);
		// ..associate its name for debugging
		sq_setnativeclosurename(vm, -1, name);
		std::string typemask = func_signature_t<F>::get_typemask(act_as_member);
		sq_setparamscheck(vm, func_signature_t<F>::get_nparams() - act_as_member, typemask.c_str());
		sq_newslot(vm, -3, staticmethod);

		log_squirrel_type(func_signature_t<F>::get_squirrel_class(act_as_member), name, func_signature_t<F>::get_squirrel_type(act_as_member, 0));
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
	 * @param act_as_member if true then a global (non-member) function can be called
	 *                      as if it would be a member function of the class instance
	 *                      provided as first argument
	 * @param staticmethod if true then register as static method
	 */
	template<typename F, class V>
	void register_method_fv(HSQUIRRELVM vm, F funcptr, const char* name, V const& freevariables, bool act_as_member = false, bool staticmethod = false)
	{
		sq_pushstring(vm, name, -1);
		// pointer to function info as free variable
		function_info_t<F> fi(funcptr, act_as_member);
		SQUserPointer up = sq_newuserdata(vm, sizeof(function_info_t<F>));
		memcpy(up, &fi, sizeof(function_info_t<F>));
		// more free variables
		SQInteger count = freevariables.push(vm);
		// create function
		sq_newclosure(vm, generic_squirrel_callback<F>, count+1);
		// ..associate its name for debugging
		sq_setnativeclosurename(vm, -1, name);
		std::string typemask = func_signature_t<F>::get_typemask(act_as_member);
		sq_setparamscheck(vm, func_signature_t<F>::get_nparams() - act_as_member - count, typemask.c_str());
		sq_newslot(vm, -3, staticmethod);

		log_squirrel_type(func_signature_t<F>::get_squirrel_class(act_as_member), name, func_signature_t<F>::get_squirrel_type(act_as_member, count));
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
	 * @param funcptr pointer to the c++ method
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
		return embed_call_t<F>::call_function(vm, fi.funcptr, fi.act_as_member);
	}

	/**
	 * Template to split parameter lists
	 */
	template<typename A1, typename... As> struct parameter_pack_t
	{
		using first = A1;
		using back = void (*)(As...);
	};
	/**
	 * Template to generate strings related to argument lists
	 */
	template<typename F> struct parameter_list_t;

	template<typename... As>
	struct  parameter_list_t< void (*)(As...) >
	{
		/// @returns get typemask of first @p nparams parameters only
		static std::string get_param_typemask()
		{
			std::string res = param<typename parameter_pack_t<As...>::first>::typemask()
			     + parameter_list_t<typename parameter_pack_t<As...>::back >::get_param_typemask();
			return res;
		}
		/// @returns type list of first @p nparams arguments (without parentheses)
		static std::string get_param_squirrel_type(int nparams)
		{
			if (nparams > 0) {
				std::string res = param<typename parameter_pack_t<As...>::first>::squirrel_type();
				std::string add = parameter_list_t< typename parameter_pack_t<As...>::back >::get_param_squirrel_type(nparams-1);
				return res + (add.empty() ? "" : ", " + add);
			}
			return "";
		}
	};
	// specialization for empty parameter list
	template<>
	struct  parameter_list_t< void (*)() >
	{
		static std::string get_param_typemask() { return ""; }
		static std::string get_param_squirrel_type(int) { return ""; }
	};
	/**
	 * Templates to generate squirrel typemasks,
	 * wrapped in a one-parameter template struct,
	 * which is specialized per function-signature.
	 */
	template<typename F> struct func_signature_t;

	/**
	 * Non-member functions.
	 * May act as member functions if @p act_as_member is true.
	 */
	template<typename R, typename... As>
	struct func_signature_t<R (*)(As...)>
	{
		/// @returns typemask taking into account class argument is taken from 0th or 1st parameter
		static std::string get_typemask(bool act_as_member)
		{
			return (act_as_member ? "" : "." ) + parameter_list_t<void(*)(As...)>::get_param_typemask();
		}
		/// @returns number of parameters
		static int get_nparams()
		{
			return sizeof...(As) + 1;
		}
		/// @returns squirrel class name of class argument of F ("" if not a member function)
		static std::string get_squirrel_class(bool act_as_member)
		{
			if (act_as_member) {
				// type of first parameter - if there is none then ""
				return  parameter_list_t<void(*)(As...)>::get_param_squirrel_type(1);
			}
			return "";
		}
		/// @returns function signature if interpreted as squirrel function
		static std::string get_squirrel_type(bool act_as_member, int freevars)
		{
			std::string res = get_return_squirrel_type() + "(";
			// remaining number of parameters (including class name if act_as_member == true)
			int nparams = get_nparams() - freevars - 1;

			std::string params = parameter_list_t< void (*)(As...) >::get_param_squirrel_type(nparams);

			if (act_as_member) {
				// ignore class parameter
				size_t n = params.find(", ");
				if (n == std::string::npos  ||  n+2 > params.size()) {
					// not found
					params = "";
				}
				else {
					// drop first parameter
					params = params.substr(n+2);
				}
			}

			return res += params + ")";
		}
		/// @return squirrel type of return value
		static std::string get_return_squirrel_type()
		{
			return param<R (*)()>::squirrel_type(); // will resolve R = void
		}
	};

	/**
	 * Member functions
	 */
	template<class C, typename R, typename... As>
	struct func_signature_t<R (C::*)(As...)>
	{
		// reduce it to the case of an non-member function with act_as_member == true
		using function = R (*)(C*,As...);

		static int get_nparams()
		{
			return func_signature_t<function>::get_nparams() - 1;
		}
		static std::string get_typemask(bool)
		{
			return func_signature_t<function>::get_typemask(true);
		}
		static std::string get_squirrel_class(bool)
		{
			return param<C*>::squirrel_type();
		}
		static std::string get_squirrel_type(bool, int freevars)
		{
			return func_signature_t<function>::get_squirrel_type(true, freevars);
		}
	};
	template<class C, typename R, typename... As>
	struct func_signature_t<R (C::*)(As...) const> : public func_signature_t<R (C::*)(As...)>
	{
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
	 * Classes to implement the actual calls.
	 * Calls to function pointers and member functions are turned into std::function calls.
	 */
	template<typename F> class call_std_function_t;

	// specialization for functions with at least one argument: we might have to check for null-pointers
	template<typename R, typename A1, typename... As>
	class call_std_function_t< R(A1,As...) >
	{
		template <size_t... is>
		static SQInteger call_function_helper(HSQUIRRELVM vm, std::function<R(A1,As...)> const& func, bool act_as_member, std::index_sequence<is...>)
		{
			A1 a1 = param<A1>::get(vm, 2-act_as_member);
			if (act_as_member  &&  param_chk_t<A1>::is_null(a1)) {
				return -1;
			}
			// non-void return value
			return param<R>::push(vm, func(a1, param<As>::get(vm, is+2 /* index on stack */ +1 /* correct for A1 */ -act_as_member)...) );
			// index on stack: at +1 `this` argument, then parameters starting from +2
			// if act_as_member then `this` is first argument to function, results in offset -1
		}
	public:

		static SQInteger call_function(HSQUIRRELVM vm, std::function<R(A1,As...)> const& func, bool act_as_member)
		{
			return call_function_helper(vm, func, act_as_member, std::index_sequence_for<As...>{});
		}
	};

	// specialization for void functions with at least one argument: we might have to check for null-pointers
	template<typename A1, typename... As>
	class call_std_function_t< void(A1,As...) >
	{
		template <size_t... is>
		static SQInteger call_function_helper(HSQUIRRELVM vm, std::function<void(A1,As...)> const& func, bool act_as_member, std::index_sequence<is...>)
		{
			A1 a1 = param<A1>::get(vm, 2-act_as_member);
			if (act_as_member  &&  param_chk_t<A1>::is_null(a1)) {
				return -1;
			}
			// void function
			func(a1, param<As>::get(vm, is+2 /* index on stack */ +1 /* correct for A1 */ -act_as_member)...);
			return 0;
		}
	public:

		static SQInteger call_function(HSQUIRRELVM vm, std::function<void(A1,As...)> const& func, bool act_as_member)
		{
			return call_function_helper(vm, func, act_as_member, std::index_sequence_for<As...>{});
		}
	};

	// functions with no parameters
	template<typename R>
	class call_std_function_t< R() >
	{
	public:
		static SQInteger call_function(HSQUIRRELVM vm, std::function<R()> const& func, bool)
		{
			return param<R>::push(vm, func());
		}
	};
	template<>
	class call_std_function_t< void() >
	{
	public:
		static SQInteger call_function(HSQUIRRELVM, std::function<void()> const& func, bool)
		{
			func();
			return 0;
		}
	};

	/**
	 * Templates to call functions with automatically fetching the right parameters,
	 * wrapped in a one-parameter template struct,
	 * which is specialized per function-signature.
	 */
	template<typename F> struct embed_call_t;

	// Non-member functions
	template<typename R, typename... As>
	struct embed_call_t<R (*)(As...)>
	{
		static SQInteger call_function(HSQUIRRELVM vm, R (*func_ptr)(As...), bool act_as_member)
		{
			std::function< R(As...)> func = func_ptr;
			return call_std_function_t< R(As...) >::call_function(vm, func, act_as_member);
		}
	};
	// Non-const member functions
	template<class C, typename R, typename... As>
	struct embed_call_t<R (C::*)(As...)>
	{
		static SQInteger call_function(HSQUIRRELVM vm, R (C::*member_func_ptr)(As...), bool)
		{
			std::function< R(C*,As...)> func = member_func_ptr;

			return call_std_function_t< R(C*,As...) >::call_function(vm, func, true);
		}
	};
	// Const member functions
	template<class C, typename R, typename... As>
	struct embed_call_t<R (C::*)(As...) const>
	{
		static SQInteger call_function(HSQUIRRELVM vm, R (C::*member_func_ptr)(As...) const, bool)
		{
			std::function< R(const C*,As...)> func = member_func_ptr;

			return call_std_function_t< R(const C*,As...) >::call_function(vm, func, true);
		}
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
