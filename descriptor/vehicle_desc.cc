/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "vehicle_desc.h"
#include "xref_desc.h"
#include "../network/checksum.h"


vehicle_desc_t *vehicle_desc_t::any_vehicle = NULL;


void vehicle_desc_t::calc_checksum(checksum_t *chk) const
{
	obj_desc_transport_related_t::calc_checksum(chk);
	chk->input(capacity);
	chk->input(loading_time);
	chk->input(weight);
	chk->input(power);
	chk->input(running_cost);
	chk->input(maintenance);
	chk->input(gear);
	chk->input(len);
	chk->input(leader_count);
	chk->input(trailer_count);
	chk->input(engine_type);
	// freight
	const xref_desc_t *xref = get_child<xref_desc_t>(2);
	chk->input(xref ? xref->get_name() : "NULL");
	// vehicle constraints
	for(uint16 i=0; i<leader_count+trailer_count; i++) {
		const xref_desc_t *xref = get_child<xref_desc_t>(6+i);
		chk->input(xref ? xref->get_name() : "NULL");
	}
}

