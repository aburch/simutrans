/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "way_desc.h"
#include "../network/checksum.h"


waytype_t way_desc_t::get_finance_waytype() const
{
	return get_styp() == type_tram ? tram_wt : get_wtyp();
}

void way_desc_t::calc_checksum(checksum_t *chk) const
{
	PAKSET_INFO("","obj=way");
	obj_desc_transport_infrastructure_t::calc_checksum(chk);
#if MSG_LEVEL>0
	if(max_weight) PAKSET_INFO("payload=","%d",max_weight);
	if(styp) PAKSET_INFO("system_type=","%d",styp);
	if(draw_as_obj) PAKSET_INFO("draw_as_ding=1","");
	PAKSET_INFO("--","");
#endif
	chk->input(max_weight);
	chk->input(styp);
	chk->input(has_double_slopes());
}
