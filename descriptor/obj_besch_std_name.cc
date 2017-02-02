#include "obj_besch_std_name.h"
#include "../network/checksum.h"


void obj_desc_timelined_t::calc_checksum(checksum_t *chk) const
{
	chk->input(intro_date);
	chk->input(obsolete_date);
}


void obj_desc_transport_related_t::calc_checksum(checksum_t *chk) const
{
	obj_desc_timelined_t::calc_checksum(chk);
	chk->input(maintenance);
	chk->input(cost);
	chk->input(wt);
	chk->input(topspeed);
	chk->input(axle_load);
}
