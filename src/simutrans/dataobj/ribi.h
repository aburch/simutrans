
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
		doubles = 1 << 0, ///< two-height difference slopes
		way_ns  = 1 << 1, ///< way possible in north-south direction
		way_ew  = 1 << 2, ///< way possible in east-west direction
		single  = 1 << 3, ///< way possible
		all_up  = 1 << 4  ///< all corners raised
	};

public:

	typedef sint8 type;


	/**
	 * Named constants for special cases.
	 */
	enum _type {
		flat = 0,

		northwest = 27, ///< NW corner
		northeast = 9,  ///< NE corner
		southeast = 3,  ///< SE corner
		southwest = 1,  ///< SW corner

		north = slope_t::southeast+slope_t::southwest,	///< North slope
		west  = slope_t::northeast+slope_t::southeast,  ///< West slope
		east  = slope_t::northwest+slope_t::southwest,  ///< East slope
		south = slope_t::northwest+slope_t::northeast,  ///< South slope

		all_up_one = slope_t::southwest+slope_t::southeast+slope_t::northeast+slope_t::northwest, ///all corners 1 high
		all_up_two = slope_t::all_up_one * 2,                                                     ///all corners 2 high

		raised = all_up_two,    ///< special meaning: used as slope of bridgeheads and in terraforming tools (keep for compatibility)

		max_number = all_up_two
	};

	/*
	 * Macros to access the height of the 4 corners:
	 */
#define corner_sw(i)  ((i)%slope_t::southeast)                      // sw corner
#define corner_se(i) (((i)/slope_t::southeast)%slope_t::southeast)  // se corner
#define corner_ne(i) (((i)/slope_t::northeast)%slope_t::southeast)  // ne corner
#define corner_nw(i)  ((i)/slope_t::northwest)                      // nw corner

#define encode_corners(sw, se, ne, nw) ( (sw) * slope_t::southwest + (se) * slope_t::southeast + (ne) * slope_t::northeast + (nw) * slope_t::northwest )

#define is_one_high(i)   (i & 7)  // quick method to know whether a slope is one high - relies on two high slopes being divisible by 8 -> i&7=0 (only works for slopes with flag single)

	/// Compute the slope opposite to @p x. Returns flat if @p x does not allow ways on it.
	static type opposite(type x) { return is_single(x) ? (is_one_high(x) ? (slope_t::all_up_one - x) : (slope_t::all_up_two - x)) : flat; }
	/// Rotate.
	static type rotate90(type x) { return ( ( (x % slope_t::southeast) * slope_t::northwest ) + ( ( x - (x % slope_t::southeast) ) / slope_t::southeast ) ); }
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
		corner_SW = 1 << 0,
		corner_SE = 1 << 1,
		corner_NE = 1 << 2,
		corner_NW = 1 << 3
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
		single      = 1 << 0, ///< only one bit set, way ends here
		straight_ns = 1 << 1, ///< contains straight n/s connection
		straight_ew = 1 << 2, ///< contains straight e/w connection
		bend        = 1 << 3, ///< is a bend
		twoway      = 1 << 4, ///< two bits set
		threeway    = 1 << 5  ///< three bits set
	};

public:
	/**
	 * Named constants for all possible directions.
	 * 1=North, 2=East, 4=South, 8=West
	 */
	enum _ribi {
		none           = 0,
		north          = 1,
		east           = 2,
		northeast      = 3,
		south          = 4,
		northsouth     = 5,
		southeast      = 6,
		northsoutheast = 7,
		west           = 8,
		northwest      = 9,
		eastwest       = 10,
		northeastwest  = 11,
		southwest      = 12,
		northsouthwest = 13,
		southeastwest  = 14,
		all            = 15
	};
	typedef uint8 ribi;

	/**
	 * Named constants to translate direction to image number for vehicles, signs.
	 */
	enum _dir {
		dir_invalid   = 0,
		dir_south     = 0,
		dir_west      = 1,
		dir_southwest = 2,
		dir_southeast = 3,
		dir_north     = 4,
		dir_east      = 5,
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

	/// Table containing the four compass directions (now as function)
	struct _nesw {
		ribi operator [] ( const uint8 i ) const { return 1<<i; }
	};
	static const _nesw nesw;

	/// Convert building layout to ribi (four rotations), use doppelt in case of two rotations
	static const ribi layout_to_ribi[4]; // building layout to ribi (for four rotations, for two use doppelt()!

	static bool is_perpendicular(ribi x, ribi y);

#ifdef RIBI_LOOKUP
	static bool is_twoway(ribi x) { return (flags[x]&twoway)!=0; }
	static bool is_threeway(ribi x) { return (flags[x]&threeway)!=0; }
	static bool is_single(ribi x) { return (flags[x] & single) != 0; }
	static bool is_bend(ribi x) { return (flags[x] & bend) != 0; }
	static bool is_straight(ribi x) { return (flags[x] & (straight_ns | straight_ew)) != 0; }
	static bool is_straight_ns(ribi x) { return (flags[x] & straight_ns) != 0; }
	static bool is_straight_ew(ribi x) { return (flags[x] & straight_ew) != 0; }

	/// Convert single/straight direction into their doubled form (n, ns -> ns), map all others to zero
	static ribi doubles(ribi x) { return doppelr[x]; }

	/// Backward direction for single ribi's, bitwise-NOT for all others
	static ribi backward(ribi x) { return backwards[x]; }

	/// Convert ribi to dir
	static dir get_dir(ribi x) { return dirs[x]; }
#else
#ifdef USE_GCC_POPCOUNT
	static uint8 get_numways(ribi x) { return (__builtin_popcount(x)); }
	static bool is_twoway(ribi x) { return get_numways(x) == 2; }
	static bool is_threeway(ribi x) { return get_numways(x) > 2; }
	static bool is_single(ribi x) { return get_numways(x) == 1; }
#else
	static bool is_twoway(ribi x) { return (0x1668 >> x) & 1; }
	static bool is_threeway(ribi x) { return (0xE880 >> x) & 1; }
	static bool is_single(ribi x) { return (0x0116 >> x) & 1; }
#endif
	static bool is_bend(ribi x) { return (0x1248 >> x) & 1; }
	static bool is_straight(ribi x) { return (0x0536 >> x) & 1; }
	static bool is_straight_ns(ribi x) { return (0x0032 >> x) & 1; }
	static bool is_straight_ew(ribi x) { return (0x0504 >> x) & 1; }

	static ribi doubles(ribi x) { return (INT64_C(0x00000A0A00550A50) >> (x * 4)) & 0x0F; }
	static ribi backward(ribi x) { return (INT64_C(0x01234A628951C84F) >> (x * 4)) & 0x0F; }

	/// Convert ribi to dir
	static dir get_dir(ribi x) { return (INT64_C(0x0002007103006540) >> (x * 4)) & 0x7; }
#endif
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
