/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include "../simdebug.h"
#include "../simconst.h"
#include "ribi.h"
#include "koord.h"
#include "koord3d.h"

// since we have now a dummy function instead an array
const ribi_t::_nesw ribi_t::nesw;;

// same like the layouts of buildings
const ribi_t::ribi ribi_t::layout_to_ribi[4] = {
	south,
	east,
	north,
	west
};

const int ribi_t::flags[16] = {
	0,                    // none
	single | straight_ns, // north
	single | straight_ew, // east
	bend | twoway,        // north-east
	single | straight_ns, // south
	straight_ns | twoway, // north-south
	bend | twoway,        // south-east
	threeway,             // north-south-east
	single | straight_ew, // west
	bend | twoway,        // north-west
	straight_ew | twoway, // east-west
	threeway,             // north-east-west
	bend | twoway,        // south-west
	threeway,             // north-south-west
	threeway,             // south-east-west
	threeway,             // all
};

const ribi_t::ribi ribi_t::backwards[16] = {
	all,        // none
	south,      // north
	west,       // east
	southwest,  // north-east
	north,      // south
	northsouth, // north-south
	northwest,  // south-east
	west,       // north-south-east
	east,       // west
	southeast,  // north-west
	eastwest,   // east-west
	south,      // north-east-west
	northeast,  // south-west
	east,       // north-south-west
	north,      // south-east-west
	none        // all
};

const ribi_t::ribi ribi_t::doppelr[16] = {
	none,       // none
	northsouth, // north
	eastwest,   // east
	none,       // north-east
	northsouth, // south
	northsouth, // north-south
	none,       // south-east
	none,       // north-south-east
	eastwest,   // west
	none,       // north-west
	eastwest,   // east-west
	none,       // north-east-west
	none,       // south-west
	none,       // north-south-west
	none,       // south-east-west
	none        // all
};


static const ribi_t::ribi from_hang[81] = {
	ribi_t::none, // ribi_t::none:flat
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::south,  // 4:north single height slope
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::south,  // 8:north doubles height slope
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::east,   // 12:west single height slope
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::east,   // 24:west doubles height slope
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::west,  // 28:east single height slope
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::north,  // 36:south single height slope
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::west,  // 56:east doubles height slope
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::north,  // 72:south doubles height slope
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none,
	ribi_t::none  // 80:all of the above
};


const int slope_t::flags[81] = {
	way_ns | way_ew,                    // slope 0  # flat straight ns|ew
	0,                                  // slope 1  #             sw1
	doubles,                            // slope 2  #             sw2
	0,                                  // slope 3  #         se1
	way_ns | single,                    // slope 4  #         se1,sw1   straight ns
	doubles,                            // slope 5  #         se1,sw2
	doubles,                            // slope 6  #         se2
	doubles,                            // slope 7  #         se2,sw1
	way_ns | single | doubles,          // slope 8  #         se2,sw2   straight ns2
	0,                                  // slope 9  #     ne1
	0,                                  // slope 10 #     ne1,    sw1
	doubles,                            // slope 11 #     ne1,    sw2
	way_ew | single,                    // slope 12 #     ne1,se1       straight ew
	0,                                  // slope 13 #     ne1,se1,sw1
	doubles,                            // slope 14 #     ne1,se1,sw2
	doubles,                            // slope 15 #     ne1,se2
	doubles,                            // slope 16 #     ne1,se2,sw1
	doubles,                            // slope 17 #     ne1,se2,sw2
	doubles,                            // slope 18 #     ne2
	doubles,                            // slope 19 #     ne2,    sw1
	doubles,                            // slope 20 #     ne2,    sw2
	doubles,                            // slope 21 #     ne2,se1
	doubles,                            // slope 22 #     ne2,se1,sw1
	doubles,                            // slope 23 #     ne2,se1,sw2
	way_ew | single | doubles,          // slope 24 #     ne2,se2       straight ew2
	doubles,                            // slope 25 #     ne2,se2,sw1
	doubles,                            // slope 26 #     ne2,se2,sw2
	0,                                  // slope 27 # nw1
	way_ew | single,                    // slope 28 # nw1,        sw1   straight ew
	doubles,                            // slope 29 # nw1,        sw2
	0,                                  // slope 30 # nw1,    se1
	0,                                  // slope 31 # nw1,    se1,sw1
	doubles,                            // slope 32 # nw1,    se1,sw2
	doubles,                            // slope 33 # nw1,    se2
	doubles,                            // slope 34 # nw1,    se2,sw1
	doubles,                            // slope 35 # nw1,    se2,sw2
	way_ns | single,                    // slope 36 # nw1,ne1           straight ns
	0,                                  // slope 37 # nw1,ne1,    sw1
	doubles,                            // slope 38 # nw1,ne1,    sw2
	0,                                  // slope 39 # nw1,ne1,se1
	way_ns | way_ew | all_up,           // slope 40 # nw1,ne1,se1,sw1   TODO    0 up 1
	doubles | all_up,                   // slope 41 # nw1,ne1,se1,sw2   TODO    1 up 1
	doubles,                            // slope 42 # nw1,ne1,se2
	doubles | all_up,                   // slope 43 # nw1,ne1,se2,sw1   TODO    3 up 1
	way_ns | single | doubles | all_up, // slope 44 # nw1,ne1,se2,sw2   TODO ns 4 up 1
	doubles,                            // slope 45 # nw1,ne2
	doubles,                            // slope 46 # nw1,ne2,    sw1
	doubles,                            // slope 47 # nw1,ne2,    sw2
	doubles,                            // slope 48 # nw1,ne2,se1
	doubles | all_up,                   // slope 49 # nw1,ne2,se1,sw1   TODO    9 up 1
	doubles | all_up,                   // slope 50 # nw1,ne2,se1,sw2   TODO    10 up 1
	doubles,                            // slope 51 # nw1,ne2,se2
	way_ew | single | doubles | all_up, // slope 52 # nw1,ne2,se2,sw1   TODO ew 12 up 1
	doubles | all_up,                   // slope 53 # nw1,ne2,se2,sw2   TODO    13 up 1
	doubles,                            // slope 54 # nw2
	doubles,                            // slope 55 # nw2,        sw1
	way_ew | single | doubles,          // slope 56 # nw2,        sw2   straight ew2
	doubles,                            // slope 57 # nw2,    se1
	doubles,                            // slope 58 # nw2,    se1,sw1
	doubles,                            // slope 59 # nw2,    se1,sw2
	doubles,                            // slope 60 # nw2,    se2
	doubles,                            // slope 61 # nw2,    se2,sw1
	doubles,                            // slope 62 # nw2,    se2,sw2
	doubles,                            // slope 63 # nw2,ne1
	doubles,                            // slope 64 # nw2,ne1,    sw1
	doubles,                            // slope 65 # nw2,ne1,    sw2
	doubles,                            // slope 66 # nw2,ne1,se1
	doubles | all_up,                   // slope 67 # nw2,ne1,se1,sw1   TODO    27 up 1
	way_ew | single | doubles | all_up, // slope 68 # nw2,ne1,se1,sw2   TODO ew 28 up 1
	doubles,                            // slope 69 # nw2,ne1,se2
	doubles | all_up,                   // slope 70 # nw2,ne1,se2,sw1   TODO    30 up 1
	doubles | all_up,                   // slope 71 # nw2,ne1,se2,sw2   TODO    31 up 1
	way_ns | single | doubles,          // slope 72 # nw2,ne2           straight ns2
	doubles,                            // slope 73 # nw2,ne2,    sw1
	doubles,                            // slope 74 # nw2,ne2,    sw2
	doubles,                            // slope 75 # nw2,ne2,se1
	way_ns | single | doubles | all_up, // slope 76 # nw2,ne2,se1,sw1   TODO ns 36 up 1
	doubles | all_up,                   // slope 77 # nw2,ne2,se1,sw2   TODO    37 up 1
	doubles,                            // slope 78 # nw2,ne2,se2
	doubles | all_up,                   // slope 79 # nw2,ne2,se2,sw1   TODO    39 up 1
	doubles | way_ns | way_ew | all_up            // slope 80 # nw2,ne2,se2,sw2   TODO    0  up 2
};


const slope_t::type slope_from_ribi[16] = {
	0,
	slope_t::south,
	slope_t::west,
	0,
	slope_t::north,
	0,
	0,
	0,
	slope_t::east,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};


const ribi_t::dir ribi_t::dirs[16] = {
	dir_invalid,   // none
	dir_north,     // north
	dir_east,      // east
	dir_northeast, // north-east
	dir_south,     // south
	dir_invalid,   // north-south
	dir_southeast, // south-east
	dir_invalid,   // north-south-east
	dir_west,      // west
	dir_northwest, // north-west
	dir_invalid,   // east-west
	dir_invalid,   // north-east-west
	dir_southwest, // south-west
	dir_invalid,   // north-south-west
	dir_invalid,   // south-east-west
	dir_invalid    // all
};


ribi_t::ribi ribi_type(slope_t::type hang)   // north slope -> south, ... !
{
	return from_hang[hang];
}


ribi_t::ribi ribi_typ_intern(sint16 dx, sint16 dy)
{
	ribi_t::ribi ribi = ribi_t::none;

	if(dx<0) {
		ribi |= ribi_t::west;
	}
	else if(dx>0) {
		ribi |= ribi_t::east;
	}

	if(dy<0) {
		ribi |= ribi_t::north;
	}
	else if(dy>0) {
		ribi |= ribi_t::south;
	}
	return ribi;
}


ribi_t::ribi ribi_type(const koord& dir)
{
	return ribi_typ_intern(dir.x, dir.y);
}


ribi_t::ribi ribi_type(const koord3d& dir)
{
	return ribi_typ_intern(dir.x, dir.y);
}


/**
 * check, if two directions are orthogonal
 * works with diagonals too
 */
bool ribi_t::is_perpendicular(ribi x, ribi y)
{
	// for straight direction x use doppelr lookup table
	if(is_straight(x)) {
		return (doppelr[x] | doppelr[y]) == all;
	}
	// now diagonals (more tricky)
	if(x!=y) {
		return ((x-y)%3)==0;
	}
	// ok, then they are not orthogonal
	return false;
}


sint16 get_sloping_upwards(const slope_t::type slope, const ribi_t::ribi from)
{
	// slope upwards relative to direction 'from'
	const slope_t::type from_slope = slope_type(from);

	if (from_slope == slope) {
		return 1;
	}
	else if (2*from_slope == slope) {
		return 2;
	}
	return 0;
}


slope_t::type slope_type(koord dir)
{
	if(dir.x == 0) {
		if(dir.y < 0) {            // north direction -> south slope
			return slope_t::south;
		}
		if(dir.y > 0) {
			return slope_t::north; // south direction -> north slope
		}
	}
	if(dir.y == 0) {
		if(dir.x < 0) {
			return slope_t::east;  // west direction -> east slope
		}
		if(dir.x > 0) {
			return slope_t::west;  // east direction -> west slope
		}
	}
	return slope_t::flat;          // ???
}


slope_t::type slope_type(ribi_t::ribi r)
{
	return slope_from_ribi[r];
}
