/*
 *
 *  tunnel_besch.cpp
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */

#include <stdio.h>

#include "../dataobj/ribi.h"
#include "tunnel_besch.h"

/*
 *  static data
 */

int tunnel_besch_t::hang_indices[16] = {
    -1,			// 0:flach
    -1,			// 1:spitze SW
    -1,			// 2:spitze SO
    1,			// 3:nordhang
    -1,			// 4:spitze NO
    -1,			// 5:spitzen SW+NO
    2,			// 6:westhang
    -1,			// 7:tal NW
    -1,			// 8:spitze NW
    3,			// 9:osthang
    -1,			// 10:spitzen NW+SO
    -1,			// 11:tal NO
    0,			// 12:suedhang
    -1,			// 13:tal SO
    -1,			// 14:tal SW
    -1			// 15:alles oben
};
