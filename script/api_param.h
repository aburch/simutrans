/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SCRIPT_API_PARAM_H
#define SCRIPT_API_PARAM_H


/** @file api_param.h templates for transfer of function call parameters */

#include "../squirrel/squirrel.h"
#include "../obj/simobj.h"
#include "../simtypes.h"
#include "../tpl/quickstone_tpl.h"
#include "../utils/cbuffer_t.h"

class baum_t;
class convoi_t;
class fabrik_t;
class factory_supplier_desc_t;
class factory_product_desc_t;
class field_t;
class gebaeude_t;
class grund_t;
class haltestelle_t;
class karte_t;
class karte_ptr_t;
class koord;
class koord3d;
class label_t;
class leitung_t;
class loadsave_t;
struct schedule_entry_t;
struct my_ribi_t;
struct my_slope_t;
class planquadrat_t;
class plainstring;
class scenario_t;
class schedule_t;
class settings_t;
class simline_t;
class player_t;
class stadt_t;
class tool_t;
class ware_production_t;
class weg_t;
class wayobj_t;
class way_builder_t;

/**
 * @namespace script_api The namespace contains all functions necessary to communicate
 * between simutrans and squirrel: transfer parameters, export classes and functions.
 *
 * @brief API simutrans <-> squirrel
 */
namespace script_api {
	/// pointer to the world
	extern karte_ptr_t welt;

	// forward declaration
	struct mytime_t;
	struct mytime_ticks_t;

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
	};

	/**
	 * Create slots in table/class on the stack:
	 * it has the same effect as 'table.name <- value'.
	 *
	 * @tparam T type of the new value
	 * @param name name of the slot to be created
	 * @param value value to be set
	 * @param static_ true if this should be a static class member
	 * @param index of table/instance/etc on the stack
	 * @returns positive value on success, negative on failure
	 */
	template<class T>
	SQInteger create_slot(HSQUIRRELVM vm, const char* name, T const& value, bool static_ = false, SQInteger index = -1)
	{
		sq_pushstring(vm, name, -1);
		if (SQ_SUCCEEDED(param<T>::push(vm, value))) {
			SQInteger new_index = index > 0 ? index : index-2;
			return sq_newslot(vm, new_index, static_);
		}
		else {
			sq_pop(vm, 1); /* pop name */
			return SQ_ERROR;
		}
	}

	/**
	 * Sets value to existing variable in table, instance etc at index @p index.
	 * @tparam T type of the new value
	 * @param name name of the slot to be created
	 * @param value value to be set
	 * @param index of table/instance/etc on the stack
	 * @returns positive value on success, negative on failure
	 */
	template<class T>
	SQInteger set_slot(HSQUIRRELVM vm, const char* name, T const& value, SQInteger index = -1)
	{
		sq_pushstring(vm, name, -1);
		if (SQ_SUCCEEDED(param<T>::push(vm, value))) {
			SQInteger new_index = index > 0 ? index : index-2;
			return sq_set(vm, new_index);
		}
		else {
			sq_pop(vm, 1); /* pop name */
			return SQ_ERROR;
		}
	}

	/**
	 * Gets value to existing variable in table, instance etc at index @p index.
	 * @tparam T type of the new value
	 * @param name name of the slot to be created
	 * @param value will be set upon success
	 * @param index of table/instance/etc on the stack
	 * @returns positive value on success, negative on failure
	 */
	template<class T>
	SQInteger get_slot(HSQUIRRELVM vm, const char* name, T& value, SQInteger index = -1)
	{
		sq_pushstring(vm, name, -1);
		SQInteger new_index = index > 0 ? index : index-1;
		if (SQ_SUCCEEDED(sq_get(vm, new_index))) {
			value = param<T>::get(vm, -1);
			sq_pop(vm, 1);
			return SQ_OK;
		}
		return SQ_ERROR;
	}

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
	 * partial specialization for container types
	 */
	template< template<class> class vector, class T> struct param< vector<T> > {
		/**
		 * Pushes parameters for calls to squirrel functions on stack.
		 * @return positive value for success, negative for failure
		 */
		static SQInteger push(HSQUIRRELVM vm, vector<T> const& v)
		{
			sq_newarray(vm, 0);

			FORT(const vector<T>, const&i, v) {
				param<T>::push(vm, i);
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

	/**
	 * partial specialization for *handle_t types
	 */
	// declared here, implementation in api_class.h,
	// which has to be included if necessary
	template<class T> struct param< quickstone_tpl<T> >;

	/**
	 * partial specialization for function pointers,
	 * used in api_function.h
	 */
	template<class T> struct param<T (*)()> {
		// return type of return value
		static const char* squirrel_type()
		{
			return param<T>::squirrel_type();
		}
		static const char* typemask()
		{
			return param<T>::typemask();
		}
	};
	template<> struct param<void (*)()> {
		static const char* squirrel_type()
		{
			return "void";
		}
		static const char* typemask()
		{
			return "";
		}
	};

#define declare_types(mask, sqtype) \
	static const char* typemask() { return mask; } \
	static const char* squirrel_type() \
	{ \
		return sqtype; \
	}

	/// macro to declare specialized param template
#define declare_specialized_param(T, mask, sqtype) \
	template<> struct param<T> { \
		static T get(HSQUIRRELVM vm, SQInteger index); \
		static SQInteger push(HSQUIRRELVM vm, T const& v);\
		static void* tag(); \
		declare_types(mask, sqtype); \
	};
	/// macro to only define typemask for specialized param template
	/// if only 'const class*' is defined then parameter mask must be defined for 'class *' too.
#define declare_param_mask(T, mask, sqtype) \
	template<> struct param<T> { \
		declare_types(mask, sqtype) \
	};
	/// macro to declare fake types
	/// for documentation purposes
#define declare_fake_param(T, sqtype) \
	class T { public: T() {};   };  \
	template<> struct param<T> { \
		static T get(HSQUIRRELVM, SQInteger) { return T(); } \
		declare_types(".", sqtype); \
	};
	// macro to declare enums
#define declare_enum_param(T, inttype, sqtype) \
	template<> struct param<T> { \
		static T get(HSQUIRRELVM vm, SQInteger index) { return (T)param<inttype>::get(vm, index); } \
		static SQInteger push(HSQUIRRELVM vm, T const& v) { return param<inttype>::push(vm, v); } \
		declare_types("i", sqtype); \
	};


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
	declare_enum_param(waytype_t, sint16, "way_types");
	declare_enum_param(systemtype_t, uint8, "way_system_types");
	declare_enum_param(obj_t::typ, uint8, "map_objects");
	declare_enum_param(climate, uint8, "climates");
	declare_specialized_param(my_ribi_t, "i", "dir");
	declare_specialized_param(my_slope_t, "i", "slope");

	declare_specialized_param(double, "i|f", "float");

	declare_specialized_param(const char*, ".", "string");
	// no string typemask, as we call to_string
	declare_specialized_param(plainstring, ".", "string");

	declare_specialized_param(koord, "t|x|y", "coord");
	declare_specialized_param(koord3d, "t|x|y", "coord3d");

	declare_specialized_param(convoi_t*, "t|x|y", "convoy_x");
	declare_specialized_param(fabrik_t*, "t|x|y", "factory_x");
	declare_specialized_param(const fabrik_t*, "t|x|y", "factory_x");
	declare_specialized_param(grund_t*, "t|x|y", "tile_x");
	declare_specialized_param(const haltestelle_t*, "t|x|y", "halt_x");
	declare_param_mask(haltestelle_t*, "t|x|y", "halt_x");
	declare_specialized_param(karte_t*, ".", "world");
	declare_specialized_param(planquadrat_t*, "t|x|y", "square_x");
	declare_specialized_param(settings_t*, "t|x|y", "settings");
	declare_specialized_param(schedule_t*, "t|x|y", "schedule_x");
	declare_specialized_param(const schedule_t*, "t|x|y", "schedule_x");
	declare_specialized_param(schedule_entry_t, "t|x|y", "schedule_entry_x");
	declare_specialized_param(mytime_t, "i|t|x|y", "time_x");
	declare_specialized_param(mytime_ticks_t, "i|t|x|y", "time_ticks_x");
	declare_specialized_param(scenario_t*, "t|x|y", "");
	declare_specialized_param(simline_t*, "t|x|y", "line_x");
	declare_specialized_param(player_t*, "t|x|y", "player_x");
	declare_specialized_param(stadt_t*, "t|x|y", "city_x");
	declare_specialized_param(const ware_production_t*, "t|x|y", "factory_production_x");
	declare_specialized_param(const factory_supplier_desc_t*, "t|x|y", "factory_production_x");
	declare_specialized_param(const factory_product_desc_t*, "t|x|y", "factory_production_x");
	declare_param_mask(ware_production_t*, "t|x|y", "factory_production_x");
	declare_specialized_param(tool_t*, "x", "command_x");
	declare_specialized_param(way_builder_t*, "t|x|y", "way_planner_x");

	// export of obj_t derived classes in api/map_objects.cc
	declare_specialized_param(obj_t*, "t|x|y", "map_object_x");
	declare_specialized_param(baum_t*, "t|x|y", "tree_x");
	declare_specialized_param(gebaeude_t*, "t|x|y", "building_x");
	declare_specialized_param(label_t*, "t|x|y", "label_x");
	declare_specialized_param(leitung_t*, "t|x|y", "powerline_x");
	declare_specialized_param(weg_t*, "t|x|y", "way_x");
	declare_specialized_param(field_t*, "t|x|y", "field_x");
	declare_specialized_param(wayobj_t*, "t|x|y", "wayobj_x");

	/**
	 * Returns the player associated to the script
	 * (or NULL for scenarios)
	 */
	player_t* get_my_player(HSQUIRRELVM vm);

	// trivial specialization for zero arguments
	inline SQInteger push_param(HSQUIRRELVM) { return 0; }

	/**
	 * Pushes (recursively) all arguments on the stack.
	 * First argument is pushed first.
	 * @returns number of arguments (or SQ_ERROR)
	 */
	template<class A1, class... As>
	SQInteger push_param(HSQUIRRELVM vm, const A1 & a1, const As &...  as)
	{
		// push first parameter
		if (SQ_SUCCEEDED(script_api::param<A1>::push(vm, a1))) {
			// push the other parameters
			int res = push_param(vm, as...);
			if (SQ_SUCCEEDED(res)) {
				return res+1;
			}
			else {
				// error: pop our parameter
				sq_poptop(vm);
			}
		}
		return -1;
	}

	/**
	 * Templated interface to declare free variables for
	 * c++ function calls
	 */
	template<class A1, class... As> struct freevariable : freevariable<As...>
	{
		using base = freevariable<As...>;
		A1 arg1;

		freevariable(const A1 & a1, const As &... as) : base(as...), arg1(a1) {}
		/**
		 * Pushes the free variables. Lifo.
		 * @returns number of pushed parameters
		 */
		SQInteger push(HSQUIRRELVM vm) const
		{
			int res = base::push(vm);
			if (SQ_SUCCEEDED(res)) {
				// first parameter is pushed last
				if (SQ_SUCCEEDED(script_api::param<A1>::push(vm, arg1))) {
					return res+1;
				}
				else {
					sq_pop(vm, res); // pop other parameters in case of failure
				}
			}
			return -1;
		}
	};

	template<class A1> struct freevariable<A1>
	{
		A1 arg1;

		freevariable(const A1 & a1) : arg1(a1) {}
		/**
		 * Pushes the free variables
		 * @returns number of pushed parameters
		 */
		SQInteger push(HSQUIRRELVM vm) const
		{
			return push_param(vm, arg1);
		}
	};


	/**
	 * Static class to handle the translation of world coordinates (which are sensible to rotation)
	 * to script coordinates (that are independent of rotation).
	 */
	class coordinate_transform_t {
	private:
		/// Stores how many times initial map was rotated.
		/// Scripts do not take care of rotated maps.
		/// Coordinates will be translated between in-game coordinates and script coordinates.
		/// First v.m. to be started sets this value
		static uint8 rotation;
	public:
		/// called if a new world is initialized
		static void new_world() { rotation=4; /*invalid*/ }

		/// inits rotation from karte_t::settings
		static void initialize();

		/// keep track of rotation
		static void rotate90()  { if (rotation<4) rotation = (rotation+1)&3; }

		/// read/save rotation to stay consistent after saving & loading
		static void rdwr(loadsave_t*);

		/**
		 * rotate actual world coordinates back,
		 * coordinates after transform are like in the
		 * scenario's original savegame
		 */
		static void koord_w2sq(koord &);

		/**
		 * rotate original coordinates to actual world coordinates
		 */
		static void koord_sq2w(koord &);

		/**
		 * rotate actual world coordinates direction to original direction
		 */
		static void ribi_w2sq(ribi_t::ribi &r);

		/**
		 * rotate original direction to actual world coordinates direction
		 */
		static void ribi_sq2w(ribi_t::ribi &r);

		/**
		 * rotate actual slope to original slope
		 */
		static void slope_w2sq(slope_t::type &s);

		/**
		 * rotate original slope to actual slope
		 */
		static void slope_sq2w(slope_t::type &s);

		static uint8 get_rotation() { return rotation; }
	};

	/// called by karte_t directly
	void rotate90();
	/// called by karte_t directly
	void new_world();

}; // end of namespace
#endif
