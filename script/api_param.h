#ifndef _API_PARAM_H_
#define _API_PARAM_H_

/** @file api_param.h templates for transfer of function call parameters */

#include "../squirrel/squirrel.h"
#include "../simobj.h"
#include "../simtypes.h"
#include "../tpl/quickstone_tpl.h"
#include "../utils/cbuffer_t.h"

class baum_t;
class convoi_t;
class fabrik_t;
class factory_supplier_desc_t;
class factory_product_desc_t;
class gebaeude_t;
class grund_t;
class haltestelle_t;
class world_t;
class karte_ptr_t;
class koord;
class koord3d;
class label_t;
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
class spieler_t;
class stadt_t;
class tool_t;
class ware_production_t;
class weg_t;
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
	};

	/**
	 * Create slots in table/class on the stack:
	 * it has the same effect as 'table.name <- value'.
	 * @tparam type of the new value
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
	 * @tparam type of the new value
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
	 * @tparam type of the new value
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
	/// macro to declare fake types, inherited from void_t,
	/// for documentation purposes
#define declare_fake_param(T, sqtype) \
	class T { public: T(void_t) {};  operator void_t() const { return void_t();} };  \
	template<> struct param<T> { \
		static T get(HSQUIRRELVM vm, SQInteger index) { return param<void_t>::get(vm, index); } \
		static SQInteger push(HSQUIRRELVM vm, T const& v) { return param<void_t>::push(vm, v); } \
		declare_types(".", sqtype); \
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
	declare_specialized_param(systemtype_t, "i", "way_system_types");
	declare_specialized_param(obj_t::typ, "i", "map_objects");
	declare_specialized_param(climate, "i", "climates");
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
	declare_specialized_param(grund_t*, "t|x|y", "tile_x");
	declare_specialized_param(const haltestelle_t*, "t|x|y", "halt_x");
	declare_param_mask(haltestelle_t*, "t|x|y", "halt_x");
	declare_specialized_param(world_t*, ".", "world");
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
	declare_specialized_param(weg_t*, "t|x|y", "way_x");

	/**
	 * Returns the player associated to the script
	 * (or NULL for scenarios)
	 */
	player_t* get_my_player(HSQUIRRELVM vm);

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

		/// inits rotation from world_t::settings
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

	/// called by world_t directly
	void rotate90();
	/// called by world_t directly
	void new_world();

}; // end of namespace
#endif
