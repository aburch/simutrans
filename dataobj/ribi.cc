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
	wegbar_ns | wegbar_ow, // 0:flat
	0,
	doppel,
	0,
	wegbar_ns | einfach,   // 4:north single height slope
	doppel,
	doppel,
	doppel,
	wegbar_ns | einfach | doppel,   // 8:north double height slope
	0,
	0,
	doppel,
	wegbar_ow | einfach,   // 12:west single height slope
	0,
	doppel,
	doppel,
	doppel,
	doppel,
	doppel,
	doppel,
	doppel,
	doppel,
	doppel,
	doppel,
	wegbar_ow | einfach | doppel,   // 24:west double height slope
	doppel,
	doppel,
	0,
	wegbar_ow | einfach,   // 28:east single height slope
	doppel,
	0,
	0,
	doppel,
	0,
	0,
	doppel,
	wegbar_ns | einfach,   // 36:south single height slope
	0,
	doppel,
	0,
	wegbar_ns | wegbar_ow | all_up, // 40:all 1 tile high
	doppel | all_up,
	doppel | all_up,
	doppel,
	wegbar_ns | einfach | doppel | all_up,   // 44 north slope 2
	doppel | all_up,
	doppel,
	doppel,
	doppel,
	doppel,
	doppel | all_up,
	doppel,
	wegbar_ow | einfach | doppel | all_up,   // 52 west slope 2
	doppel | all_up,
	doppel,
	doppel,
	wegbar_ow | einfach | doppel,   // 56:east double height slope
	doppel,
	doppel,
	doppel,
	doppel,
	doppel,
	doppel,
	doppel,
	doppel,
	doppel,
	doppel,
	doppel | all_up,
	wegbar_ow | einfach | doppel | all_up,   // 68:east slope 2
	doppel,
	doppel | all_up,
	doppel | all_up,
	wegbar_ns | einfach | doppel,   // 72:south double height slope
	doppel,
	doppel,
	doppel,
	wegbar_ns | einfach | doppel | all_up,   // 76:south slope 2
	doppel | all_up,
	doppel,
	doppel | all_up,
	wegbar_ns | wegbar_ow | all_up  // 80:all 2 tile high
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
