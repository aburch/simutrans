/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#include "../simdebug.h"
#include "../simconst.h"
#include "ribi.h"
#include "koord.h"
#include "koord3d.h"

const ribi_t::ribi ribi_t::nsow[4] = {
	nord,
	sued,
	ost,
	west
};

// same like the layouts of buildings
const ribi_t::ribi ribi_t::layout_to_ribi[4] = {
	sued,
	ost,
	nord,
	west
};

const int ribi_t::flags[16] = {
	0,						// none
	einfach | gerade_ns,	// north
	einfach | gerade_ow,	// east
	kurve | twoway,			// north-east
	einfach | gerade_ns,	// south
	gerade_ns | twoway,		// north-south
	kurve | twoway,			// south-east
	threeway,				// north-south-east
	einfach | gerade_ow,	// west
	kurve | twoway,			// north-west
	gerade_ow | twoway,		// east-west
	threeway,				// north-east-west
	kurve | twoway,			// south-west
	threeway,				// north-south-west
	threeway,				// south-east-west
	threeway,				// all
};

const ribi_t::ribi ribi_t::rwr[16] = {
	alle,		// none
	sued,		// north
	west,		// east
	suedwest,	// north-east
	nord,		// south
	nordsued,	// north-south
	nordwest,	// south-east
	west,		// north-south-east
	ost,		// west
	suedost,	// north-west
	ostwest,	// east-west
	sued,		// north-east-west
	nordost,	// south-west
	ost,		// north-south-west
	nord,		// south-east-west
	keine		// all
};

const ribi_t::ribi ribi_t::doppelr[16] = {
	keine,		// none
	nordsued,	// north
	ostwest,	// east
	keine,		// north-east
	nordsued,	// south
	nordsued,	// north-south
	keine,		// south-east
	keine,		// north-south-east
	ostwest,	// west
	keine,		// north-west
	ostwest,	// east-west
	keine,		// north-east-west
	keine,		// south-west
	keine,		// north-south-west
	keine,		// south-east-west
	keine		// all
};

const ribi_t::ribi ribi_t::fwrd[16] = {
	alle,			// none
	nordostwest,	// north
	nordsuedost,	// east
	nordost,		// north-east
	suedostwest,	// south
	keine,			// north-south
	suedost,		// south-east
	keine,			// north-south-east
	nordsuedwest,	// west
	nordwest,		// north-west
	keine,			// east-west
	keine,			// north-east-west
	suedwest,		// south-west
	keine,			// north-south-west
	keine,			// south-east-west
	keine			// all
};


static const ribi_t::ribi from_hang[81] = {
	ribi_t::keine, // ribi_t::none:flat
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::sued,  // 4:north single height slope
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::sued,  // 8:north double height slope
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::ost,   // 12:west single height slope
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::ost,   // 24:west double height slope
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::west,  // 28:east single height slope
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::nord,  // 36:south single height slope
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::west,  // 56:east double height slope
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::nord,  // 72:south double height slope
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine  // 80:all of the above
};


const int hang_t::flags[81] = {
	wegbar_ns | wegbar_ow,	// slope 0 # flat	straight ns|ew
	0,	// slope 1 # sw1
	doppel,	// slope 2 # sw2
	0,	// slope 3 # se1
	wegbar_ns | einfach,	// slope 4 # se1,sw1	straight ns
	doppel,	// slope 5 # se1,sw2
	doppel,	// slope 6 # se2
	doppel,	// slope 7 # se2,sw1
	wegbar_ns | einfach | doppel,	// slope 8 # se2,sw2	straight ns2
	0,	// slope 9 # ne1
	0,	// slope 10 # ne1,sw1
	doppel,	// slope 11 # ne1,sw2
	wegbar_ow | einfach,	// slope 12 # ne1,se1	straight ew
	0,	// slope 13 # ne1,se1,sw1
	doppel,	// slope 14 # ne1,se1,sw2
	doppel,	// slope 15 # ne1,se2
	doppel,	// slope 16 # ne1,se2,sw1
	doppel,	// slope 17 # ne1,se2,sw2
	doppel,	// slope 18 # ne2
	doppel,	// slope 19 # ne2,sw1
	doppel,	// slope 20 # ne2,sw2
	doppel,	// slope 21 # ne2,se1
	doppel,	// slope 22 # ne2,se1,sw1
	doppel,	// slope 23 # ne2,se1,sw2
	wegbar_ow | einfach | doppel,	// slope 24 # ne2,se2	straight ew2
	doppel,	// slope 25 # ne2,se2,sw1
	doppel,	// slope 26 # ne2,se2,sw2
	0,	// slope 27 # nw1
	wegbar_ow | einfach,	// slope 28 # nw1,sw1	straight ew
	doppel,	// slope 29 # nw1,sw2
	0,	// slope 30 # nw1,se1
	0,	// slope 31 # nw1,se1,sw1
	doppel,	// slope 32 # nw1,se1,sw2
	0,	// slope 33 # nw1,se2
	0,	// slope 34 # nw1,se2,sw1
	doppel,	// slope 35 # nw1,se2,sw2
	wegbar_ns | einfach,	// slope 36 # nw1,ne1	straight ns
	0,	// slope 37 # nw1,ne1,sw1
	doppel,	// slope 38 # nw1,ne1,sw2
	0,	// slope 39 # nw1,ne1,se1
	wegbar_ns | wegbar_ow | all_up,	// slope 40 # nw1,ne1,se1,sw1	TODO	0 up 1
	doppel | all_up,	// slope 41 # nw1,ne1,se1,sw2	TODO	1 up 1
	doppel,	// slope 42 # nw1,ne1,se2
	doppel | all_up,	// slope 43 # nw1,ne1,se2,sw1	TODO	3 up 1
	wegbar_ns | einfach | doppel | all_up,	// slope 44 # nw1,ne1,se2,sw2	TODOns	4 up 1
	doppel,	// slope 45 # nw1,ne2
	doppel,	// slope 46 # nw1,ne2,sw1
	doppel,	// slope 47 # nw1,ne2,sw2
	doppel,	// slope 48 # nw1,ne2,se1
	doppel | all_up,	// slope 49 # nw1,ne2,se1,sw1	TODO	9 up 1
	doppel | all_up,	// slope 50 # nw1,ne2,se1,sw2	TODO	10 up 1
	doppel,	// slope 51 # nw1,ne2,se2
	wegbar_ow | einfach | doppel | all_up,	// slope 52 # nw1,ne2,se2,sw1	TODOew	12 up 1
	doppel | all_up,	// slope 53 # nw1,ne2,se2,sw2	TODO	13 up 1
	doppel,	// slope 54 # nw2
	doppel,	// slope 55 # nw2,sw1
	wegbar_ow | einfach | doppel,	// slope 56 # nw2,sw2	straight ew2
	doppel,	// slope 57 # nw2,se1
	doppel,	// slope 58 # nw2,se1,sw1
	doppel,	// slope 59 # nw2,se1,sw2
	doppel,	// slope 60 # nw2,se2
	doppel,	// slope 61 # nw2,se2,sw1
	doppel,	// slope 62 # nw2,se2,sw2
	doppel,	// slope 63 # nw2,ne1
	doppel,	// slope 64 # nw2,ne1,sw1
	doppel,	// slope 65 # nw2,ne1,sw2
	doppel,	// slope 66 # nw2,ne1,se1
	doppel | all_up,	// slope 67 # nw2,ne1,se1,sw1	TODO	27 up 1
	wegbar_ow | einfach | doppel | all_up,	// slope 68 # nw2,ne1,se1,sw2	TODOew	28 up 1
	doppel,	// slope 69 # nw2,ne1,se2
	doppel | all_up,	// slope 70 # nw2,ne1,se2,sw1	TODO	30 up 1
	doppel | all_up,	// slope 71 # nw2,ne1,se2,sw2	TODO	31 up 1
	wegbar_ns | einfach | doppel,	// slope 72 # nw2,ne2	straight ns2
	doppel,	// slope 73 # nw2,ne2,sw1
	doppel,	// slope 74 # nw2,ne2,sw2
	doppel,	// slope 75 # nw2,ne2,se1
	wegbar_ns | einfach | doppel | all_up,	// slope 76 # nw2,ne2,se1,sw1	TODOns	36 up 1
	doppel | all_up,	// slope 77 # nw2,ne2,se1,sw2	TODO	37 up 1
	doppel,	// slope 78 # nw2,ne2,se2
	doppel | all_up,	// slope 79 # nw2,ne2,se2,sw1	TODO	39 up 1
	wegbar_ns | wegbar_ow | all_up	// slope 80 # nw2,ne2,se2,sw2	TODO	0 up 2
};


const hang_t::typ hang_t::hang_from_ribi[16] = {
	0,
	hang_t::nord,
	hang_t::ost,
	0,
	hang_t::sued,
	0,
	0,
	0,
	hang_t::west,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};


const ribi_t::dir ribi_t::dirs[16] = {
	dir_invalid,	// none
	dir_nord,		// north
	dir_ost,		// east
	dir_nordost,	// north-east
	dir_sued,		// south
	dir_invalid,	// north-south
	dir_suedost,	// south-east
	dir_invalid,	// north-south-east
	dir_west,		// west
	dir_nordwest,	// north-west
	dir_invalid,	// east-west
	dir_invalid,	// north-east-west
	dir_suedwest,	// south-west
	dir_invalid,	// north-south-west
	dir_invalid,	// south-east-west
	dir_invalid		// all
};


ribi_t::ribi ribi_typ(koord from, koord to)
{
	return ribi_typ(to - from);
}


ribi_t::ribi ribi_typ(hang_t::typ hang)   // north slope -> south, ... !
{
	return from_hang[hang];
}


ribi_t::ribi ribi_typ(koord dir)
{
	ribi_t::ribi ribi = ribi_t::keine;

	if(dir.x<0) {
		ribi |= ribi_t::west;
	}
	else if(dir.x>0) {
		ribi |= ribi_t::ost;
	}

	if(dir.y<0) {
		ribi |= ribi_t::nord;
	}
	else if(dir.y>0) {
		ribi |= ribi_t::sued;
	}
	return ribi;
}


ribi_t::ribi ribi_typ(koord3d from, koord3d to)
{
	return ribi_typ(to-from);
}


ribi_t::ribi ribi_typ(koord3d dir)
{
	ribi_t::ribi ribi = ribi_t::keine;

	if(dir.x<0) {
		ribi |= ribi_t::west;
	}
	else if(dir.x>0) {
		ribi |= ribi_t::ost;
	}
	if(dir.y<0) {
		ribi |= ribi_t::nord;
	}
	else if(dir.y>0) {
		ribi |= ribi_t::sued;
	}
	return ribi;
}


/* check, if two directions are orthogonal
 * works with diagonals too
 * @author prissi
 */
bool ribi_t::ist_exakt_orthogonal(ribi x, ribi y)
{
	// for straight, we are finished here
	if(ist_gerade(x)) {
		return ist_orthogonal(x,y);
	}
	// now diagonals (more tricky)
	if(x!=y) {
		return ((x-y)%3)==0;
	}
	// ok, then they are not orthogonal
	return false;
}


hang_t::typ hang_typ(koord dir)
{
	if(dir.x == 0) {
		if(dir.y < 0) {		    // north direction -> south slope
			return hang_t::sued;
		}
		if(dir.y > 0) {
			return hang_t::nord;    // south direction -> north slope
		}
	}
	if(dir.y == 0) {
		if(dir.x < 0) {
			return hang_t::ost;	    // west direction -> east slope
		}
		if(dir.x > 0) {
			return hang_t::west;    // east direction -> west slope
		}
	}
	return hang_t::flach;	    // ???
}



hang_t::typ hang_typ(ribi_t::ribi r)
{
	return hang_t::hang_from_ribi[r];
}
