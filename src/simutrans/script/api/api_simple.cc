/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_simple.cc exports simple data types. */

#include "api_simple.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../dataobj/ribi.h"
#include "../../world/simworld.h"
#include "../../dataobj/scenario.h"

using namespace script_api;


#ifdef DOXYGEN
/**
 * Struct to hold information about time as month-year.
 * If used as argument to functions then only the value of @ref raw matters.
 * If filled by an api-call the year and month will be set, too.
 * Relation: raw = 12*month + year.
 * @see world::get_time obj_desc_time_x
 */
class time_x { // begin_class("time_x")
public:
#ifdef SQAPI_DOC // document members
	integer raw;   ///< raw integer value of date
	integer year;  ///< year
	integer month; ///< month in 0..11
#endif
}; // end_class

/**
 * Struct to get precise information about time.
 * @see world::get_time
 */
class time_ticks_x : public time_x { // begin_class("time_ticks_x", "time_x")
public:
#ifdef SQAPI_DOC // document members
	integer ticks;            ///< current time in ticks
	integer ticks_per_month;  ///< length of one in-game month in ticks
	integer next_month_ticks; ///< new month will start at this time
#endif
}; // end_class
#endif

// pushes table = { raw = , year = , month = }
SQInteger param<mytime_t>::push(HSQUIRRELVM vm, mytime_t const& v)
{
	sq_newtableex(vm, 6);
	create_slot<uint32>(vm, "raw",   v.raw);
	create_slot<uint32>(vm, "year",  v.raw/12);
	create_slot<uint32>(vm, "month", v.raw%12);
	return 1;
}

SQInteger param<mytime_ticks_t>::push(HSQUIRRELVM vm, mytime_ticks_t const& v)
{
	param<mytime_t>::push(vm, v);
	create_slot<uint32>(vm, "ticks",            v.ticks);
	create_slot<uint32>(vm, "ticks_per_month",  v.ticks_per_month);
	create_slot<uint32>(vm, "next_month_ticks", v.next_month_ticks);
	return 1;
}


mytime_t param<mytime_t>::get(HSQUIRRELVM vm, SQInteger index)
{
	// 0 has special meaning of 'no-timeline'
	SQInteger i=1;
	if (!SQ_SUCCEEDED(sq_getinteger(vm, index, &i))) {
		get_slot(vm, "raw", i, index);
	}
	return (uint16) (i >= 0 ? i : 1);
}


#define map_ribi_any(f,type) \
	type export_## f(my_ribi_t r) \
	{ \
		return ribi_t::f(r); \
	}

// export the ribi functions
map_ribi_any(is_single, bool);
map_ribi_any(is_twoway, bool);
map_ribi_any(is_threeway, bool);
map_ribi_any(is_bend, bool);
map_ribi_any(is_straight, bool);
map_ribi_any(doubles, my_ribi_t);
map_ribi_any(backward, my_ribi_t);

my_slope_t ribi_to_slope(my_ribi_t ribi)
{
	return slope_type((ribi_t::ribi)ribi);
}

my_ribi_t slope_to_ribi(my_slope_t slope)
{
	return ribi_type((slope_t::type)slope);
}


template<int idx> SQInteger coord_to_ribi(HSQUIRRELVM vm)
{
	// get coordinate vector without transformation
	sint16 x=-1, y=-1;
	get_slot(vm, "x", x, idx);
	get_slot(vm, "y", y, idx);
	koord k(x,y);

	// and do not transform ribi either
	uint8 ribi = ribi_type(k);
	return param<uint8>::push(vm, ribi);
}

SQInteger ribi_to_coord(HSQUIRRELVM vm)
{
	const uint8 ribi = param<uint8>::get(vm, 2);
	if ((ribi & ~(uint8)ribi_t::all) != 0) {
		return sq_raise_error(vm, "Invalid dir %hhu (valid values are 0..15)", ribi);
	}

	koord k( (ribi_t::ribi)ribi );
	return push_instance(vm, "coord", k.x, k.y);
}


void export_simple(HSQUIRRELVM vm)
{

	/**
	 * Class that holds 2d coordinates.
	 * All functions that use this as input parameters will accept every data structure that contains members x and y.
	 *
	 * Coordinates always refer to the original rotation in @ref map.file.
	 * They will be rotated if transferred between the game engine and squirrel.
	 */
	begin_class(vm, "coord");
#ifdef SQAPI_DOC // document members
	/// x-coordinate
	integer x;
	/// y-coordinate
	integer y;
	// operators are defined in script_base.nut
	coord(int x, int y);
	coord operator + (coord other);
	coord operator - (coord other);
	coord operator - ();
	coord operator * (integer fac);
	coord operator / (integer fac);
	/**
	 * Converts coordinate to string containing the coordinates in the current rotation of the map.
	 *
	 * Cannot be used in links in scenario texts. Use @ref href instead.
	 */
	string _tostring();
	/**
	 * Generates text to generate links to coordinates in scenario texts.
	 * @param text text to be shown in the link
	 * @returns a-tag with link in href
	 * @see get_rule_text
	 */
	string href(string text);
#endif
	/**
	 * Helper function to convert direction vector to dir type.
	 * @typemask dir()
	 */
	register_function(vm, coord_to_ribi<1>, "to_dir", 1, "t|x|y");

	end_class(vm);

	/**
	 * Class that holds 3d coordinates.
	 * All functions that use this as input parameters will accept every data structure that contains members x, y, and z.
	 *
	 * Coordinates always refer to the original rotation in @ref map.file.
	 * They will be rotated if transferred between the game engine and squirrel.
	 */
	begin_class(vm, "coord3d", "coord");
#ifdef SQAPI_DOC // document members
	/// x-coordinate
	integer x;
	/// y-coordinate
	integer y;
	/// z-coordinate - height
	integer z;
	// operators are defined in script_base.nut
	coord(int x, int y, int z);
	coord3d operator + (coord3d other);
	coord3d operator - (coord other);
	coord3d operator + (coord3d other);
	coord3d operator - (coord other);
	coord3d operator - ();
	coord3d operator * (integer fac);
	coord3d operator / (integer fac);
	/**
	 * Converts coordinate to string containing the coordinates in the current rotation of the map.
	 *
	 * Cannot be used in links in scenario texts. Use @ref href instead.
	 */
	string _tostring();
	/**
	 * Generates text to generate links to coordinates in scenario texts.
	 * @param text text to be shown in the link
	 * @returns a-tag with link in href
	 * @see get_rule_text
	 */
	string href(string text);
#endif
	end_class(vm);

	/**
	 * Helper function to convert direction vector to dir type.
	 * @param dir
	 * @typemask dir(coord)
	 */
	register_function(vm, coord_to_ribi<2>, "coord_to_dir", 2, "x t|x|y");

	/**
	 * Class holding static methods to work with directions.
	 * Directions are just bit-encoded integers.
	 */
	begin_class(vm, "dir");

#ifdef SQAPI_DOC // document members
	/** @name Named directions. */
	//@{
	static const dir none;
	static const dir north;
	static const dir east;
	static const dir northeast;
	static const dir south;
	static const dir northsouth;
	static const dir southeast;
	static const dir northsoutheast;
	static const dir west;
	static const dir northwest;
	static const dir eastwest;
	static const dir northeastwest;
	static const dir southwest;
	static const dir northsouthwest;
	static const dir southeastwest;
	static const dir all;
	//@}
#endif
	/**
	 * @param d direction to test
	 * @return whether direction is single direction, i.e. just one of n/s/e/w
	 */
	STATIC register_method(vm, &export_is_single,  "is_single", false, true);
	/**
	 * @param d direction to test
	 * @return whether direction is double direction, e.g. n+e, n+s.
	 */
	STATIC register_method(vm, &export_is_twoway,  "is_twoway", false, true);
	/**
	 * @param d direction to test
	 * @return whether direction is triple direction, e.g. n+s+e or n+s+e+w.
	 */
	STATIC register_method(vm, &export_is_threeway,  "is_threeway", false, true);
	/**
	 * @param d direction to test
	 * @return whether direction is curve, e.g. n+e, s+w.
	 */
	STATIC register_method(vm, &export_is_bend,  "is_curve", false, true);
	/**
	 * @param d direction to test
	 * @return whether direction is straight and has no curves in it, e.g. n+s, w.
	 */
	STATIC register_method(vm, &export_is_straight,  "is_straight", false, true);
	/**
	 * @param d direction
	 * @return complements direction to complete straight, i.e. w -> w+e, but n+w -> 0.
	 */
	STATIC register_method(vm, &export_doubles,  "double", false, true);
	/**
	 * @param d direction to test
	 * @return backward direction, e.g. w -> e, n+w -> s+e, n+w+s -> e.
	 */
	STATIC register_method(vm, &export_backward,  "backward", false, true);

	/**
	 * Converts direction to slope: direction goes upward on slope.
	 * @param d direction
	 */
	STATIC register_method(vm, &ribi_to_slope, "to_slope", false, true);
	/**
	 * Helper function to convert direction vector to dir type.
	 * @typemask coord(dir)
	 */
	STATIC register_function(vm, ribi_to_coord, "to_coord", 2, ".i", true);

	end_class(vm);

	/**
	 * Class holding static methods to work with slopes.
	 * Slopes are just integers.
	 */
	begin_class(vm, "slope");

#ifdef SQAPI_DOC // document members
	/** @name Named slopes. */
	//@{
	static const slope flat;
	static const slope north;      ///< North slope
	static const slope west;       ///< West slope
	static const slope east;       ///< East slope
	static const slope south;      ///< South slope
	static const slope northwest;  ///< NW corner
	static const slope northeast;  ///< NE corner
	static const slope southeast;  ///< SE corner
	static const slope southwest;  ///< SW corner
	static const slope raised;     ///< special meaning: used as slope of bridgeheads
	static const slope all_up_slope   = 82; ///< used for terraforming tools
	static const slope all_down_slope = 83; ///< used for terraforming tools
	//@}
#endif

	/**
	 * Converts slope to dir: direction goes upward on slope.
	 * If slope cannot be walked on, it returns @ref dir::none.
	 * @param s slope
	 */
	STATIC register_method(vm, &slope_to_ribi, "to_dir", false, true);
	end_class(vm);

#ifdef SQAPI_DOC
	/**
	 * Classes to access in-game objects.
	 * Creating an instance of such classes will not create an object in the game.
	 * Instead, these classes contain data to access the in-game object.
	 * If the in-game object disappears, then any access to it via these classes will fail.
	 * Use the method @r ingame_object::is_valid to check whether the in-game object is still alive.
	 */
	class ingame_object
	{
		/**
		 * @returns true if in-game object can still be accessed.
		 */
		bool is_valid();
	}
#endif
}
