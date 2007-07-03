#include "koord.h"
#include "loadsave.h"


const koord koord::invalid(-1, -1);
const koord koord::nord(    0, -1);
const koord koord::ost(     1,  0);
const koord koord::sued(    0,  1);
const koord koord::west(   -1,  0);
const koord koord::nsow[] = {
	koord( 0, -1),
	koord( 0,  1),
	koord( 1,  0),
	koord(-1,  0)
};

const koord koord::from_ribi[] = {
	koord( 0,  0), // keine
	koord( 0, -1), // nord (1)
	koord( 1,  0), // ost	(2)
	koord( 1, -1), // nordost  (3)
	koord( 0,  1), // sued (4)
	koord( 0,  0), // nordsued (5)
	koord( 1,  1), // suedost (6)
	koord( 1,  0), // nordsuedost (7)
	koord(-1,  0), // west (8)
	koord(-1, -1), // nordwest (9)
	koord( 0,  0), // ostwest  (10)
	koord( 0, -1), // nordostwest (11)
	koord(-1,  1), // suedwest (12)
	koord(-1,  0), // nordsuedwest (13)
	koord( 0,  1), // suedostwest (14)
	koord( 0,  0)  // alle
};

const koord koord::from_hang[] = {
	koord( 0,  0), // 0:flach
	koord( 0,  0), // 1:spitze SW
	koord( 0,  0), // 2:spitze SO
	koord( 0,  1), // 3:nordhang
	koord( 0,  0), // 4:spitze NO
	koord( 0,  0), // 5:spitzen SW+NO
	koord( 1,  0), // 6:westhang
	koord( 0,  0), // 7:tal NW
	koord( 0,  0), // 8:spitze NW
	koord(-1,  0), // 9:osthang
	koord( 0,  0), // 10:spitzen NW+SO
	koord( 0,  0), // 11:tal NO
	koord( 0, -1), // 12:suedhang
	koord( 0,  0), // 13:tal SO
	koord( 0,  0), // 14:tal SW
	koord( 0,  0)  // 15:alles oben
};


void koord::rdwr(loadsave_t *file)
{
	file->rdwr_short(x, " ");
	file->rdwr_short(y, "\n");
}


koord::koord(loadsave_t *file)
{
	rdwr(file);
}
