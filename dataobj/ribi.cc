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
	0,										// keine
	einfach | gerade_ns,	// nord
	einfach | gerade_ow,	// ost
	kurve | twoway,				// nordost
	einfach | gerade_ns,	// sued
	gerade_ns | twoway,		// nordsued
	kurve | twoway,				// suedost
	threeway,							// nordsuedost
	einfach | gerade_ow,	// west
	kurve | twoway,				// nordwest
	gerade_ow | twoway,		// ostwest
	threeway,							// nordostwest
	kurve | twoway,				// suedwest
	threeway,							// nordsuedwest
	threeway,							// suedostwest
	threeway,							// alle
};

const ribi_t::ribi ribi_t::rwr[16] = {
	alle,			// keine
	sued,			// nord
	west,			// ost
	suedwest,			// nordost
	nord,			// sued
	nordsued,			// nordsued
	nordwest,			// suedost
	west,			// nordsuedost
	ost,			// west
	suedost,			// nordwest
	ostwest,			// ostwest
	sued,			// nordostwest
	nordost,			// suedwest
	ost,			// nordsuedwest
	nord,			// suedostwest
	keine			// alle
};

const ribi_t::ribi ribi_t::doppelr[16] = {
	keine,			// keine
	nordsued,			// nord
	ostwest,			// ost
	keine,			// nordost
	nordsued,			// sued
	nordsued,			// nordsued
	keine,			// suedost
	keine,			// nordsuedost
	ostwest,			// west
	keine,			// nordwest
	ostwest,			// ostwest
	keine,			// nordostwest
	keine,			// suedwest
	keine,			// nordsuedwest
	keine,			// suedostwest
	keine			// alle
};

const ribi_t::ribi ribi_t::fwrd[16] = {
	alle,			// keine
	nordostwest,	// nord
	nordsuedost,	// ost
	nordost,			// nordost
	suedostwest,	// sued
	keine,				// nordsued
	suedost,			// suedost
	keine,				// nordsuedost
	nordsuedwest,	// west
	nordwest,			// nordwest
	keine,				// ostwest
	keine,				// nordostwest
	suedwest,			// suedwest
	keine,				// nordsuedwest
	keine,				// suedostwest
	keine			// alle
};


static const ribi_t::ribi from_hang[81] = {
	ribi_t::keine, // ribi_t::keine:flach
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::sued,  // 4:nordhang
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::sued,  // 8: double height nord
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::ost,   // 12:westhang
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
	ribi_t::ost,   // 24: double height west
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::west,  // 28:osthang
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::nord,  // 36:suedhang
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
	ribi_t::west,  // 56:double osthang
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
	ribi_t::nord,  // 72: double sued
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine,
	ribi_t::keine  // 80:alles oben
};


const int hang_t::flags[81] = {
	wegbar_ns | wegbar_ow, // 0:flach
	0,
	0,
	0,
	wegbar_ns | einfach,   // 4:nordhang
	0,
	0,
	0,
	wegbar_ns | einfach,   // 8: double height north
	0,
	0,
	0,
	wegbar_ow | einfach,   // 12:westhang
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	wegbar_ow | einfach,   // 24: double height west
	0,
	0,
	0,
	wegbar_ow | einfach,   // 28:osthang
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	wegbar_ns | einfach,   // 36:suedhang
	0,
	0,
	0,
	wegbar_ns | wegbar_ow, // 40:all 1 tile high
	0,
	0,
	0,
	wegbar_ns | einfach,   // 44 nordhang 2
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	wegbar_ow | einfach,   // 52 westhang 2
	0,
	0,
	0,
	wegbar_ow | einfach,   // 56:double height east
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	wegbar_ow | einfach,   // 68:osthang 2
	0,
	0,
	0,
	wegbar_ns | einfach,   // 72:double height south
	0,
	0,
	0,
	wegbar_ns | einfach,   // 76:suedhang 2
	0,
	0,
	0,
	wegbar_ns | wegbar_ow  // 80:all 2 tile high
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
	dir_invalid,		// keine
	dir_nord,			// nord
	dir_ost,			// ost
	dir_nordost,		// nordost
	dir_sued,			// sued
	dir_invalid,		// nordsued
	dir_suedost,		// suedost
	dir_invalid,		// nordsuedost
	dir_west,			// west
	dir_nordwest,		// nordwest
	dir_invalid,		// ostwest
	dir_invalid,		// nordostwest
	dir_suedwest,		// suedwest
	dir_invalid,		// nordsuedwest
	dir_invalid,		// suedostwest
	dir_invalid			// alle
};


ribi_t::ribi ribi_typ(koord from, koord to)
{
	return ribi_typ(to - from);
}


ribi_t::ribi ribi_typ(hang_t::typ hang)   // nordhang -> sued, ... !
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
		if(dir.y < 0) {		    // Richtung nord -> suedhang
			return hang_t::sued;
		}
		if(dir.y > 0) {
			return hang_t::nord;    // Richtung sued -> nordhang
		}
	}
	if(dir.y == 0) {
		if(dir.x < 0) {
			return hang_t::ost;	    // Richtung west -> osthang
		}
		if(dir.x > 0) {
			return hang_t::west;    // Richtung ost -> westhang
		}
	}
	return hang_t::flach;	    // ???
}



hang_t::typ hang_typ(ribi_t::ribi r)
{
	return hang_t::hang_from_ribi[r];
}
