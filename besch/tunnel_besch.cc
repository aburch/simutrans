/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#include <stdio.h>

#include "../dataobj/ribi.h"
#include "tunnel_besch.h"
#include "../utils/checksum.h"


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

void tunnel_besch_t::calc_checksum(checksum_t *chk) const
{
	chk->input(topspeed);
	chk->input(preis);
	chk->input(maintenance);
	chk->input(wegtyp);
	chk->input(intro_date);
	chk->input(obsolete_date);
	
	//Experimental settings
	chk->input(max_weight);
	chk->input(way_constraints.get_permissive());
	chk->input(way_constraints.get_prohibitive());
}
