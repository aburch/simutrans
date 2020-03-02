/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "obj_base_desc.h"
#include "../network/checksum.h"


void obj_desc_timelined_t::calc_checksum(checksum_t *chk) const
{
	chk->input(intro_date);
	chk->input(retire_date);
}


void obj_desc_transport_related_t::calc_checksum(checksum_t *chk) const
{
	obj_desc_timelined_t::calc_checksum(chk);
	chk->input(maintenance);
	chk->input(price);
	chk->input(wtyp);
	chk->input(topspeed);
	chk->input(axle_load);
}
