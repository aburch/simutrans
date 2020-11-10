/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
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
	chk->input(base_maintenance);
	chk->input(base_cost);
	chk->input(wtyp);
	chk->input(topspeed);
	chk->input(topspeed-topspeed_gradient_1);
	chk->input(topspeed-topspeed_gradient_2);
	chk->input(axle_load);
	chk->input(way_only_cost);
}
