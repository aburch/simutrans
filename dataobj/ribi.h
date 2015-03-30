/*
 * Copyright (c) 1997 - 2002 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef dataobj_ribi_t_h
#define dataobj_ribi_t_h

#include "../simtypes.h"
#include "../simconst.h"

class koord;
class koord3d;

/**
 * slopes
 */
class hang_t {
	static const int flags[81];

	// wegbar: flat enough for ways, infach=only one ew/ns direction, doppel=two height difference, all_up=unused slope (equals a slope starting on z-level up)
	enum { doppel = 1, wegbar_ns = 2, wegbar_ow = 4, einfach = 8, all_up = 16 };

public:
	/*
	 * Bitfield
	 * Bit 0 is set if southwest corner is raised
	 * Bit 1 is set if southeast corner is raised
	 * Bit 2 is set if northeast corner is raised
	 * Bit 3 is set if northwest corner is raised
	 *
	 * Don't get confused - the southern/southward slope has its northern corners raised
	 */
	typedef sint8 typ;
	/*
	 * Macros to access the height of the 4 corners
	 */
#define scorner1(i) (i%2)    	// sw corner
#define scorner2(i) ((i/2)%2)	// se corner
#define scorner3(i) ((i/4)%2)	// ne corner
#define scorner4(i) (i/8)    	// nw corner

#define corner1(i) (i%3)    	// sw corner
#define corner2(i) ((i/3)%3)	// se corner
#define corner3(i) ((i/9)%3)	// ne corner
#define corner4(i) (i/27)   	// nw corner

	enum _corners {
		corner_SW = 1,
		corner_SE = 2,
		corner_NE = 4,
		corner_NW = 8
	};

	enum _typ {
		flach=0,
		nord = 3+1, 	    // North slope
		west = 9+3, 	    // West slope
		ost = 27+1,	    // East slope
		sued = 27+9,	    // South slope
		nordwest = 27,
		nordost = 9,
		suedost = 3,
		suedwest = 1,
		erhoben = 80	    // Special for bridge entrances (prissi: unused, I think)
	};

	// A little tricky implementation:
	//static bool ist_gegenueber(typ x, typ y) { return ist_einfach(x) && ist_einfach(y) && x + y == 40; }	// unused at present need to extend to cope with double heights
	static typ gegenueber(typ x) { return ist_einfach(x) ? (ist_doppel(x) ? (80 - x) : (40 - x)) : flach; }
	static typ rotate90(typ x) { return ( ( (x % 3) * 27 ) + ( ( x - (x % 3) ) / 3 ) ); }

	static bool is_all_up(typ x) { return (flags[x] & all_up)>0; }

	static uint8 max_diff(typ x) { return ((sint8)x!=0)+(flags[x]&doppel); }
	static sint8 min_diff(typ high, typ low) { return min( min( corner1(high) - corner1(low), corner2(high)-corner2(low) ), min( corner3(high) - corner3(low), corner4(high) - corner4(low) ) ); }

	static const hang_t::typ hang_from_ribi[16];

	//
	// Ranges werden nicht geprüft!
	//
	static bool ist_doppel(typ x)  { return (flags[x] & doppel) != 0; }
	static bool ist_einfach(typ x) { return (flags[x] & einfach) != 0; }
	static bool ist_wegbar(typ x)  { return (flags[x] & (wegbar_ns | wegbar_ow)) != 0; }
	static bool ist_wegbar_ns(typ x)  { return (flags[x] & wegbar_ns) != 0; }
	static bool ist_wegbar_ow(typ x)  { return (flags[x] & wegbar_ow) != 0; }
	static sint16 get_sloping_upwards(const typ slope, const sint16 relative_pos_x, const sint16 relative_pos_y)
	{
		if(  relative_pos_y < 0  ) {
			if(  slope == nord  ) {
				return 1;
			}
			else if(  slope == 2 * nord  ) {
				return 2;
			}
			return 0;
		}
		if(  relative_pos_y > 0  ) {
			if(  slope == sued  ) {
				return 1;
			}
			else if(  slope == 2 * sued  ) {
				return 2;
			}
			return 0;
		}
		if(  relative_pos_x < 0  ) {
			if(  slope == west  ) {
				return 1;
			}
			else if(  slope == 2 * west  ) {
				return 2;
			}
			return 0;
		}
		if(  relative_pos_x > 0  ) {
			if(  slope == ost  ) {
				return 1;
			}
			else if(  slope == 2 * ost  ) {
				return 2;
			}
		}
		return 0;
	}
};



/**
 * Directions in simutrans
 * ribi_t = Richtungs-Bit = Directions-Bitfield
 * @author Hj. Malthaner
 */
class ribi_t {
	static const int flags[16];

	enum { einfach = 1, gerade_ns = 2, gerade_ow = 4, kurve = 8, twoway=16, threeway=32 };

public:
	/* das enum richtung ist eine verallgemeinerung der richtungsbits auf
	 * benannte Konstanten; die Werte sind wie folgt zugeordnet
	 * 1=North, 2=East, 4=South, 8=West
	 *
	 * richtungsbits (ribi) koennen 16 Komb. darstellen.
	 *
	 * the enum direction is a generalization of the 
	 * direction bits to designated constants, the 
	 * values are as follows 1 = North, 2 = East, 4 = South, 8 = West
	 *
	 * direction bits (ribi) can 16 Komb. represent.
	 */
	enum _ribi {
		keine = 0, //None
		nord = 1, //North
		ost = 2, //East
		nordost = 3, //North-East
		sued = 4, // South
		nordsued = 5, 
		suedost = 6, //South-East
		nordsuedost = 7, 
		west = 8, //West
		nordwest = 9, //North-West
		ostwest = 10, 
		nordostwest = 11,
		suedwest = 12, //South-West
		nordsuedwest = 13, 
		suedostwest = 14, 
		alle = 15 //"Everything".
	};
	typedef uint8 ribi;

	/**
	* Named constants to translate direction to image number for vehicles, signs.
	*/

	enum _dir {
		dir_invalid = 0,
		dir_sued = 0,
		dir_west = 1,
		dir_suedwest = 2,
		dir_suedost = 3,
		dir_nord = 4,
		dir_ost = 5,
		dir_nordost = 6,
		dir_nordwest = 7
	};
	typedef uint8 dir;

private:
	static const ribi fwrd[16];
	static const ribi rwr[16];
	static const ribi doppelr[16];
	static const dir  dirs[16];

public:
	static const ribi nsow[4];
	static const ribi layout_to_ribi[4];	// building layout to ribi (for four rotations, for two use doppelt()!
	//
	// Alle Abfragen über statische Tabellen wg. Performance
	// Ranges werden nicht geprüft!
	//
	static bool is_twoway(ribi x) { return (flags[x]&twoway)!=0; }
	static bool is_threeway(ribi x) { return (flags[x]&threeway)!=0; }

	static bool ist_exakt_orthogonal(ribi x, ribi y);
	static bool ist_orthogonal(ribi x, ribi y) { return (doppelr[x] | doppelr[y]) == alle; }
	static bool ist_einfach(ribi x) { return (flags[x] & einfach) != 0; }
	static bool ist_kurve(ribi x) { return (flags[x] & kurve) != 0; }
	static bool ist_gerade(ribi x) { return (flags[x] & (gerade_ns | gerade_ow)) != 0; }
	static bool ist_gerade_ns(ribi x) { return (flags[x] & gerade_ns) != 0; }
	static bool ist_gerade_ow(ribi x) { return (flags[x] & gerade_ow) != 0; }

	static ribi doppelt(ribi x) { return doppelr[x]; }
	static ribi rueckwaerts(ribi x) { return rwr[x]; } // "Backwards" (Google)

	/**
	* Same as rueckwaerts, but for single directions only.
	* Effectively does bit rotation. Avoids lookup table rwr.
	* @returns rueckwaerts(x) for single ribis, 0 for x==0.
	*/
	static inline ribi reverse_single(ribi x) {
		return ((x | x<<4) >> 2) & 0xf;
	}

	static ribi get_forward(ribi x) { return fwrd[x]; }	// all ribis, that are in front of this thing
	/// Rotate 90 degrees to the right. Does bit rotation.
	static ribi rotate90(ribi x) { return ((x | x<<4) >> 3) & 0xf; }
	/// Rotate 90 degrees to the left. Does bit rotation.
	static ribi rotate90l(ribi x) { return ((x | x<<4) >> 1) & 0xf; }
	static ribi rotate45(ribi x) { return (ist_einfach(x) ? x|rotate90(x) : x&rotate90(x)); } // 45 to the right
	static ribi rotate45l(ribi x) { return (ist_einfach(x) ? x|rotate90l(x) : x&rotate90l(x)); } // 45 to the left

	static dir get_dir(ribi x) { return dirs[x]; }

};

//
// Umrechnungen zwische koord, ribi_t und hang_t:
//
//	ribi_t::nord entspricht koord::nord entspricht hang_t::sued !!!
//	-> ich denke aufwaerts, also geht es auf einem Suedhang nach Norden!
// I think upwards, so it goes on a southern slope to the north! (Google)
//
hang_t::typ  hang_typ(koord dir);   // dir:nord -> hang:sued, ...
hang_t::typ  hang_typ(ribi_t::ribi);

ribi_t::ribi ribi_typ(koord dir);
ribi_t::ribi ribi_typ(koord from, koord to);
ribi_t::ribi ribi_typ(koord3d dir);
ribi_t::ribi ribi_typ(koord3d from, koord3d to);
ribi_t::ribi ribi_typ(hang_t::typ hang);  // nordhang -> sued, ... !

#endif
