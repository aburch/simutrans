#ifndef _API_PARAM_H_
#define _API_PARAM_H_

/** @file api_param.h templates for transfer of function call parameters */

#include "../squirrel/squirrel.h"
#include "../simdings.h"
#include "../simtypes.h"
#include "../halthandle_t.h"
#include "../utils/cbuffer_t.h"

class baum_t;
class convoi_t;
class fabrik_t;
class gebaeude_t;
class grund_t;
class karte_t;
class koord;
class koord3d;
struct linieneintrag_t;
class obj_besch_std_name_t;
class planquadrat_t;
class plainstring;
class scenario_t;
class schedule_t;
class settings_t;
class spieler_t;
class stadt_t;
class ware_production_t;
class ware_besch_t;
class weg_t;

/**
 * @namespace script_api The namespace contains all functions necessary to communicate
 * between simutrans and squirrel: transfer parameters, export classes and functions.
 *
 * @brief API simutrans <-> squirrel
 */
namespace script_api {
	/// pointer to the world
	extern karte_t *welt;

	/**
	 * Cannot specialize templates by void, so use own void type
	 */
	struct void_t {};

	/**
	 * Templated interface to transfer variables from / to squirrel.
	 */
	template<class T> struct param {
		/**
		 * Gets parameters for calls to c++ functions from stack.
		 * @param index on stack
		 * @return value of parameter
		 */
#ifdef DOXYGEN
		static T get(HSQUIRRELVM vm, SQInteger index);
#endif

		/**
		 * Pushes parameters for calls to squirrel functions on stack.
		 * @return positive value for success, negative for failure
		 */
#ifdef DOXYGEN
		static SQInteger push(HSQUIRRELVM vm, T const& v);
#endif
		/// typemask: this squirrel type needs to be provided when calling c++ function
		static const char* typemask() { return "."; }

		/// squirrel_type corresponding to the c++ type/class
		static const char* squirrel_type() { return "any_x"; }

		/**
		 * Creates slot in table at top of the stack:
		 * it has the same effect as 'table.name <- value'.
		 * @pre there needs to be a table on the top of the stack
		 * @param name name of the slot to be created
		 * @param value value to be set
		 * @returns positive value on success, negative on failure
		 */
#ifdef DOXYGEN
		SQInteger create_slot(HSQUIRRELVM vm, const char* name, T value);
#endif
	};

	/**
	 * partial specialization for 'const T*' types
	 */
	template<class T> struct param<const T*> {
		/**
		 * Gets parameters for calls to c++ functions from stack.
		 * @param index on stack
		 * @return value of parameter
		 */
		static const T* get(HSQUIRRELVM vm, SQInteger index)
		{
			return param<T*>::get(vm, index);
		}
		/**
		 * Pushes parameters for calls to squirrel functions on stack.
		 * @return positive value for success, negative for failure
		 */
		static SQInteger push(HSQUIRRELVM vm, const T* const& v)
		{
			return param<T*>::push(vm, v);
		}

		/// typemask: this squirrel type needs to be provided when calling c++ function
		static const char* typemask()
		{
			return param<T*>::typemask();
		}
		/// squirrel_type corresponding to the c++ type/class
		static const char* squirrel_type()
		{
			return param<T*>::squirrel_type();
		}
	};

	/**
	 * partial specialization for 'const T' types
	 */
	template<class T> struct param<const T> {
		/**
		 * Gets parameters for calls to c++ functions from stack.
		 * @param index on stack
		 * @return value of parameter
		 */
		static const T get(HSQUIRRELVM vm, SQInteger index)
		{
			return param<T>::get(vm, index);
		}
		/**
		 * Pushes parameters for calls to squirrel functions on stack.
		 * @return positive value for success, negative for failure
		 */
		static SQInteger push(HSQUIRRELVM vm, T const& v)
		{
			return param<T>::push(vm, v);
		}

		/// typemask: this squirrel type needs to be provided when calling c++ function
		static const char* typemask()
		{
			return param<T>::typemask();
		}
		/// squirrel_type corresponding to the c++ type/class
		static const char* squirrel_type()
		{
			return param<T>::squirrel_type();
		}
	};

	/**
	 * partial specialization for 'const T&' types
	 */
	template<class T> struct param<const T&> {
		/**
		 * Pushes parameters for calls to squirrel functions on stack.
		 * @return positive value for success, negative for failure
		 */
		static SQInteger push(HSQUIRRELVM vm, const T& v)
		{
			return param<T>::push(vm, v);
		}

		/// typemask: this squirrel type needs to be provided when calling c++ function
		static const char* typemask()
		{
			return param<T>::typemask();
		}
		/// squirrel_type corresponding to the c++ type/class
		static const char* squirrel_type()
		{
			return param<T>::squirrel_type();
		}
	};

	/**
	 * partial specialization for vector_tpl types
	 */
	template< template<class> class vector, class T> struct param< vector<T> > {
		/**
		 * Pushes parameters for calls to squirrel functions on stack.
		 * @return positive value for success, negative for failure
		 */
		static SQInteger push(HSQUIRRELVM vm, vector<T> const& v)
		{
			sq_newarray(vm, 0);
			for(uint32 i=0; i<v.get_count(); i++) {
				param<T>::push(vm, v[i]);
				sq_arrayappend(vm, -2);
			}
			return 1;
		}
		/// squirrel_type corresponding to the c++ type/class
		static const char* squirrel_type()
		{
			static cbuffer_t buf;
			buf.clear();
			buf.printf("array<%s>", param<T>::squirrel_type() );
			return buf;
		}
	};

#define declare_types(mask, sqtype) \
	static const char* typemask() { return mask; } \
	static const char* squirrel_type() \
	{ \
		return sqtype; \
	}

#define declare_create_slot(T)\
	static SQInteger create_slot(HSQUIRRELVM vm, const char* name, T const& value){ \
		sq_pushstring(vm, name, -1); \
		if (SQ_SUCCEEDED(param<T>::push(vm, value))) { \
			sq_newslot(vm, -3, false); \
			return SQ_OK; \
		} \
		else { \
			sq_pop(vm, 1); /* pop name */ \
			return SQ_ERROR; \
		} \
	}

	/// macro to declare specialized param template
#define declare_specialized_param(T, mask, sqtype) \
	template<> struct param<T> { \
		static T get(HSQUIRRELVM vm, SQInteger index); \
		static SQInteger push(HSQUIRRELVM vm, T const& v);\
		declare_create_slot(T); \
		declare_types(mask, sqtype); \
	};
	/// macro to only define typemask for specialized param template
	/// if only 'const class*' is defined then parameter mask must be defined for 'class *' too.
#define declare_param_mask(T, mask, sqtype) \
	template<> struct param<T> { \
		declare_types(mask, sqtype) \
	};

	declare_specialized_param(void_t, ".", "void");
	// no typemask, as we call to_bool
	declare_specialized_param(bool, ".", "bool");

	declare_specialized_param(uint8, "i", "integer");
	declare_specialized_param(sint8, "i", "integer");
	declare_specialized_param(uint16, "i", "integer");
	declare_specialized_param(sint16, "i", "integer");
	declare_specialized_param(uint32, "i", "integer");
	declare_specialized_param(sint32, "i", "integer");
	declare_specialized_param(uint64, "i", "integer");
	declare_specialized_param(sint64, "i", "integer");
	declare_specialized_param(waytype_t, "i", "way_types");
	declare_specialized_param(ding_t::typ, "i", "map_objects");

	declare_specialized_param(double, "f", "float");

	declare_specialized_param(const char*, ".", "string");
	// no string typemask, as we call to_string
	declare_specialized_param(plainstring, ".", "string");

	declare_specialized_param(koord, "t|x|y", "coord");
	declare_specialized_param(koord3d, "t|x|y", "coord3d");

	declare_specialized_param(convoi_t*, "t|x|y", "convoy_x");
	declare_specialized_param(fabrik_t*, "t|x|y", "factory_x");
	declare_specialized_param(grund_t*, "t|x|y", "tile_x");
	declare_specialized_param(halthandle_t, "t|x|y", "halt_x");
	declare_specialized_param(const haltestelle_t*, "t|x|y", "halt_x");
	declare_param_mask(haltestelle_t*, "t|x|y", "halt_x");
	declare_specialized_param(karte_t*, ".", "world");
	declare_specialized_param(planquadrat_t*, "t|x|y", "square_x");
	declare_specialized_param(settings_t*, "t|x|y", "settings");
	declare_specialized_param(schedule_t*, "t|x|y", "schedule_x");
	declare_specialized_param(linieneintrag_t, "t|x|y", "schedule_entry_x");
	declare_specialized_param(scenario_t*, "t|x|y", "");
	declare_specialized_param(spieler_t*, "t|x|y", "player_x");
	declare_specialized_param(stadt_t*, "t|x|y", "city_x");
	declare_specialized_param(const obj_besch_std_name_t*, "t|x|y", "obj_desc_x"); // in api/export_besch.cc
	declare_param_mask(obj_besch_std_name_t*, "t|x|y", "obj_desc_x");
	declare_specialized_param(const ware_besch_t*, "t|x|y", "good_desc_x");
	declare_param_mask(ware_besch_t*, "t|x|y", "good_desc_x");
	declare_specialized_param(const ware_production_t*, "t|x|y", "factory_production_x");
	declare_param_mask(ware_production_t*, "t|x|y", "factory_production_x");

	// export of ding_t derived classes in api/map_objects.cc
	declare_specialized_param(ding_t*, "t|x|y", "map_object_x");
	declare_specialized_param(baum_t*, "t|x|y", "tree_x");
	declare_specialized_param(gebaeude_t*, "t|x|y", "building_x");
	declare_specialized_param(weg_t*, "t|x|y", "way_x");

	/**
	 * Templated interface to declare free variables for
	 * c++ function calls
	 */
	template<class A1> struct freevariable {
		A1 arg1;
		freevariable(A1 const& a1) : arg1(a1) {}

		/**
		 * Pushes the free variables
		 * @returns number of pushed parameters
		 */
		SQInteger push(HSQUIRRELVM vm) const {
			SQInteger count = 0;
			if (SQ_SUCCEEDED( param<A1>::push(vm, arg1) ) ) count++;
			return count;
		}
	};

	/**
	 * Templated interface to declare free variables for
	 * c++ function calls
	 */
	template<class A1,class A2> struct freevariable2 : public freevariable<A1> {
		A2 arg2;
		freevariable2(A1 const& a1, A2 const& a2) : freevariable<A1>(a1), arg2(a2) {}

		/**
		 * Pushes the free variables
		 * @returns number of pushed parameters
		 */
		SQInteger push(HSQUIRRELVM vm) const {
			SQInteger count = 0;
			if (SQ_SUCCEEDED( param<A2>::push(vm, arg2) ) ) count++;
			count += freevariable<A1>::push(vm);
			return count;
		}
	};

	template<class A1,class A2,class A3> struct freevariable3 : public freevariable2<A1,A2> {
		A3 arg3;
		freevariable3(A1 const& a1, A2 const& a2, A3 const& a3) : freevariable2<A1,A2>(a1,a2), arg3(a3) {}

		/**
		 * Pushes the free variables
		 * @returns number of pushed parameters
		 */
		SQInteger push(HSQUIRRELVM vm) const {
			SQInteger count = 0;
			if (SQ_SUCCEEDED( param<A3>::push(vm, arg3) ) ) count++;
			count += freevariable2<A1,A2>::push(vm);
			return count;
		}
	};



}; // end of namespace
#endif
