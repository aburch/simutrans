/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "koord.h"
#include "loadsave.h"
#include "../display/scr_coord.h"
#include "../utils/simrandom.h"
#include "../simconst.h"


// default: close and far away does not matter
uint32 koord::locality_factor = 10000;


const scr_coord scr_coord::invalid(-1, -1);

const scr_size scr_size::invalid(-1, -1);
const scr_size scr_size::inf(0x7fffffff, 0x7fffffff);

const koord koord::invalid(-1, -1);
const koord koord::north(    0, -1);
const koord koord::east(     1,  0);
const koord koord::south(    0,  1);
const koord koord::west(   -1,  0);
const koord koord::nesw[] = {
	koord( 0, -1),
	koord( 1,  0),
	koord( 0,  1),
	koord(-1,  0)
};

const koord koord::neighbours[] = {
	koord( -1, -1),
	koord( -1, 0 ),
	koord( -1, 1 ),
	koord( 0,  1 ),
	koord( 1,  1 ),
	koord( 1,  0 ),
	koord( 1, -1 ),
	koord( 0, -1 )
};

const koord koord::from_ribi[] = {
	koord( 0,  0), // none
	koord( 0, -1), // north             (1)
	koord( 1,  0), // east              (2)
	koord( 1, -1), // north-east        (3)
	koord( 0,  1), // south             (4)
	koord( 0,  0), // north-south       (5)
	koord( 1,  1), // south-east        (6)
	koord( 1,  0), // north-south-east  (7)
	koord(-1,  0), // west              (8)
	koord(-1, -1), // north-west        (9)
	koord( 0,  0), // east-west         (10)
	koord( 0, -1), // north-east-west   (11)
	koord(-1,  1), // south-west        (12)
	koord(-1,  0), // north-south-west  (13)
	koord( 0,  1), // south-east-west   (14)
	koord( 0,  0)  // all
};

const koord koord::from_hang[] = {
	koord( 0,  0), // 0:flat
	koord( 0,  0), // 1:
	koord( 0,  0), // 2:
	koord( 0,  0), // 3:
	koord( 0,  1), // 4:north single height slope
	koord( 0,  0), // 5:
	koord( 0,  0), // 6:
	koord( 0,  0), // 7:
	koord( 0,  1), // 8:north double height slope
	koord( 0,  0), // 9:
	koord( 0,  0), // 10:
	koord( 0,  0), // 11:
	koord( 1,  0), // 12:west single height slope
	koord( 0,  0), // 13:
	koord( 0,  0), // 14:
	koord( 0,  0), // 15:
	koord( 0,  0), // 16:
	koord( 0,  0), // 17:
	koord( 0,  0), // 18:
	koord( 0,  0), // 19:
	koord( 0,  0), // 20:
	koord( 0,  0), // 21:
	koord( 0,  0), // 22:
	koord( 0,  0), // 23:
	koord( 1,  0), // 24:west double height slope
	koord( 0,  0), // 25:
	koord( 0,  0), // 26:
	koord( 0,  0), // 27:
	koord(-1,  0), // 28:east single height slope
	koord( 0,  0), // 29:
	koord( 0,  0), // 30:
	koord( 0,  0), // 31:
	koord( 0,  0), // 32:
	koord( 0,  0), // 33:
	koord( 0,  0), // 34:
	koord( 0,  0), // 35:
	koord( 0, -1), // 36:south single height slope
	koord( 0,  0), // 37:
	koord( 0,  0), // 38:
	koord( 0,  0), // 39:
	koord( 0,  0), // 40:
	koord( 0,  0), // 41:
	koord( 0,  0), // 42:
	koord( 0,  0), // 43:
	koord( 0,  0), // 44:
	koord( 0,  0), // 45:
	koord( 0,  0), // 46:
	koord( 0,  0), // 47:
	koord( 0,  0), // 48:
	koord( 0,  0), // 49:
	koord( 0,  0), // 50:
	koord( 0,  0), // 51:
	koord( 0,  0), // 52:
	koord( 0,  0), // 53:
	koord( 0,  0), // 54:
	koord( 0,  0), // 55:
	koord(-1,  0), // 56:east double height slope
	koord( 0,  0), // 57:
	koord( 0,  0), // 58:
	koord( 0,  0), // 59:
	koord( 0,  0), // 60:
	koord( 0,  0), // 61:
	koord( 0,  0), // 62:
	koord( 0,  0), // 63:
	koord( 0,  0), // 64:
	koord( 0,  0), // 65:
	koord( 0,  0), // 66:
	koord( 0,  0), // 67:
	koord( 0,  0), // 68:
	koord( 0,  0), // 69:
	koord( 0,  0), // 70:
	koord( 0,  0), // 71:
	koord( 0, -1), // 72:south double height slope
	koord( 0,  0), // 73:
	koord( 0,  0), // 74:
	koord( 0,  0), // 75:
	koord( 0,  0), // 76:
	koord( 0,  0), // 77:
	koord( 0,  0), // 78:
	koord( 0,  0), // 79:
	koord( 0,  0)  // 80:
};


void koord::rdwr(loadsave_t *file)
{
	xml_tag_t k( file, "koord" );
	file->rdwr_short(x);
	file->rdwr_short(y);
}


// for debug messages...
const char *koord::get_str() const
{
	static char pos_str[32];
	if(x==-1  &&  y==-1) {
		return "koord invalid";
	}
	sprintf( pos_str, "%i,%i", x, y );
	return pos_str;
}


const char *koord::get_fullstr() const
{
	static char pos_str[32];
	if(x==-1  &&  y==-1) {
		return "koord invalid";
	}
	sprintf( pos_str, "(%i,%i)", x, y );
	return pos_str;
}

// obey order of simrand among different compilers
koord koord::koord_random( uint16 xrange, uint16 yrange )
{
	koord ret;
	ret.x = simrand(xrange);
	ret.y = simrand(yrange);
	return ret;
}
