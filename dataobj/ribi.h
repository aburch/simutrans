/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_RIBI_H
#define DATAOBJ_RIBI_H


#include "../simtypes.h"
#include "../simconst.h"
#include "../simdebug.h"

class koord;
class koord3d;

/**
 * Slopes of tiles.
 */
class slope_t {

	/// Static lookup table
	static const int flags[81];

	/// Named constants for the flags table
	enum {
		doubles = 1,   ///< two-height difference slopes
		way_ns  = 2,   ///< way possible in north-south direction
		way_ew  = 4,   ///< way possible in east-west direction
		single  = 8,   ///< way possible
		all_up  = 16,  ///< all corners raised
	};

public:

	typedef sint8 type;

	/*
	 * Macros to access the height of the 4 corners:
	 * Each corner has height 0,1,2.
	 * Calculation has to be done modulo 3 (% 3).
	 */
#define corner_sw(i) (i%3)      // sw corner
#define corner_se(i) ((i/3)%3)  // se corner
#define corner_ne(i) ((i/9)%3)  // ne corner
#define corner_nw(i) (i/27)     // nw corner

	/**
	 * Named constants for special cases.
	 */
	enum _type {
		flat=0,
		north = 3+1,    ///< North slope
		west = 9+3,     ///< West slope
		east = 27+1,    ///< East slope
		south = 27+9,   ///< South slope
		northwest = 27, ///< NW corner
		northeast = 9,  ///< NE corner
		southeast = 3,  ///< SE corner
		southwest = 1,  ///< SW corner
		raised = 80,    ///< special meaning: used as slope of bridgeheads and in terraforming tools
	};


	/// Compute the slope opposite to @p x. Returns flat if @p x does not allow ways on it.
	static type opposite(type x) { return is_single(x) ? (x & 7 ? (40 - x) : (80 - x * 2)) : flat; }
	/// Rotate.
	static type rotate90(type x) { return ( ( (x % 3) * 27 ) + ( ( x - (x % 3) ) / 3 ) ); }
	/// Returns true if @p x has all corners raised.
	static bool is_all_up(type x) { return (flags[x] & all_up)>0; }
	/// Returns maximal height difference between the corners of this slope.
	static uint8 max_diff(type x) { return (x!=0)+(flags[x]&doubles); }
	/// Computes minimum height differnce between corners of  @p high and @p low.
	static sint8 min_diff(type high, type low) { return min( min( corner_sw(high) - corner_sw(low), corner_se(high)-corner_se(low) ), min( corner_ne(high) - corner_ne(low), corner_nw(high) - corner_nw(low) ) ); }

	/// Returns if slope prefers certain way directions (either n/s or e/w).
	static bool is_single(type x) { return (flags[x] & single) != 0; }
	/// Returns if way can be build on this slope.
	static bool is_way(type x)  { return (flags[x] & (way_ns | way_ew)) != 0; }
	/// Returns if way in n/s direction can be build on this slope.
	static bool is_way_ns(type x)  { return (flags[x] & way_ns) != 0; }
	/// Returns if way in e/w direction can be build on this slope.
	static bool is_way_ew(type x)  { return (flags[x] & way_ew) != 0; }
};


/**
 * Old implementation of slopes: one bit per corner.
 * Used as bitfield to refer to specific corners of a tile
 * as well as for compatibility.
 */
struct slope4_t {
	/* bit-field:
	 * Bit 0 is set if southwest corner is raised
	 * Bit 1 is set if southeast corner is raised
	 * Bit 2 is set if northeast corner is raised
	 * Bit 3 is set if northwest corner is raised
	 *
	 * Don't get confused - the southern/southward slope has its northern corners raised
	 *
	 * Macros to access the height of the 4 corners for single slope:
	 * One bit per corner
	 */
	typedef sint8 type;

	#define scorner_sw(i) (i%2)     // sw corner
	#define scorner_se(i) ((i/2)%2) // se corner
	#define scorner_ne(i) ((i/4)%2) // ne corner
	#define scorner_nw(i) (i/8)     // nw corner
	enum _corners {
		corner_SW = 1,
		corner_SE = 2,
		corner_NE = 4,
		corner_NW = 8
	};
};


/**
 * Directions in simutrans.
 * ribi_t = Richtungs-Bit = Directions-Bitfield
 */
class ribi_t {
	/// Static lookup table
	static const int flags[16];

	/// Named constants for properties of directions
	enum {
		single      = 1,  ///< only one bit set, way ends here
		straight_ns = 2,  ///< contains straight n/s connection
		straight_ew = 4,  ///< contains straight e/w connection
		bend        = 8,   ///< is a bend
		twoway      = 16, ///< two bits set
		threeway    = 32, ///< three bits set
	};

public:
	/**
	 * Named constants for all possible directions.
	 * 1=North, 2=East, 4=South, 8=West
	 */
	enum _ribi {
		none =0,
		north = 1,
		east = 2,
		northeast = 3,
		south = 4,
		northsouth = 5,
		southeast = 6,
		northsoutheast = 7,
		west = 8,
		northwest = 9,
		eastwest = 10,
		northeastwest = 11,
		southwest = 12,
		northsouthwest = 13,
		southeastwest = 14,
		all = 15
	};
	typedef uint8 ribi;

	/**
	 * Named constants to translate direction to image number for vehicles, signs.
	 */
	enum _dir {
		dir_invalid = 0,
		dir_south = 0,
		dir_west = 1,
		dir_southwest = 2,
		dir_southeast = 3,
		dir_north = 4,
		dir_east = 5,
		dir_northeast = 6,
		dir_northwest = 7
	};
	typedef uint8 dir;

private:
	/// Lookup table to compute backward direction
	static const ribi backwards[16];
	/// Lookup table ...
	static const ribi doppelr[16];
	/// Lookup table to convert ribi to dir.
	static const dir  dirs[16];
public:
	/// Table containing the four compass directions
	static const ribi nsew[4];
	/// Convert building layout to ribi (four rotations), use doppelt in case of two rotations
	static const ribi layout_to_ribi[4];	// building layout to ribi (for four rotations, for two use doppelt()!

	static bool is_twoway(ribi x) { return (flags[x]&twoway)!=0; }
	static bool is_threeway(ribi x) { return (flags[x]&threeway)!=0; }
	static bool is_perpendicular(ribi x, ribi y);
	static bool is_single(ribi x) { return (flags[x] & single) != 0; }
	static bool is_bend(ribi x) { return (flags[x] & bend) != 0; }
	static bool is_straight(ribi x) { return (flags[x] & (straight_ns | straight_ew)) != 0; }
	static bool is_straight_ns(ribi x) { return (flags[x] & straight_ns) != 0; }
	static bool is_straight_ew(ribi x) { return (flags[x] & straight_ew) != 0; }

	/// Convert single/straight direction into their doubled form (n, ns -> ns), map all others to zero
	static ribi doubles(ribi x) { return doppelr[x]; }
	/// Backward direction for single ribi's, bitwise-NOT for all others
	static ribi backward(ribi x) { return backwards[x]; }

	/**
	 * Same as backward, but for single directions only.
	 * Effectively does bit rotation. Avoids lookup table backwards.
	 * @returns backward(x) for single ribis, 0 for x==0.
	 */
	static inline ribi reverse_single(ribi x) {
		return ((x  |  x<<4) >> 2) & 0xf;
	}

	/// Rotate 90 degrees to the right. Does bit rotation.
	static ribi rotate90(ribi x) { return ((x  |  x<<4) >> 3) & 0xf; }
	/// Rotate 90 degrees to the left. Does bit rotation.
	static ribi rotate90l(ribi x) { return ((x  |  x<<4) >> 1) & 0xf; }
	static ribi rotate45(ribi x) { return (is_single(x) ? x|rotate90(x) : x&rotate90(x)); } // 45 to the right
	static ribi rotate45l(ribi x) { return (is_single(x) ? x|rotate90l(x) : x&rotate90l(x)); } // 45 to the left

	/// Convert ribi to dir
	static dir get_dir(ribi x) { return dirs[x]; }
};

/**
 * Calculate slope from directions.
 * Go upward on the slope: going north translates to slope_t::south.
 */
slope_t::type slope_type(koord dir);

/**
 * Calculate slope from directions.
 * Go upward on the slope: going north translates to slope_t::south.
 */
slope_t::type slope_type(ribi_t::ribi);

/**
 * Check if the slope is upwards, relative to the direction @p from.
 * @returns 1 for single upwards and 2 for double upwards
 */
sint16 get_sloping_upwards(const slope_t::type slope, const ribi_t::ribi from);

/**
 * Calculate direction bit from coordinate differences.
 */
ribi_t::ribi ribi_typ_intern(sint16 dx, sint16 dy);

/**
 * Calculate direction bit from direction.
 */
ribi_t::ribi ribi_type(const koord& dir);
ribi_t::ribi ribi_type(const koord3d& dir);

/**
 * Calculate direction bit from slope.
 * Note: slope_t::north (slope north) will be translated to ribi_t::south (direction south).
 */
ribi_t::ribi ribi_type(slope_t::type slope);

/**
 * Calculate direction bit for travel from @p from to @p to.
 */
template<class K1, class K2>
ribi_t::ribi ribi_type(const K1&from, const K2& to)
{
	return ribi_typ_intern(to.x - from.x, to.y - from.y);
}

#endif
