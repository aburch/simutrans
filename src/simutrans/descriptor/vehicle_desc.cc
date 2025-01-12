/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "vehicle_desc.h"
#include "xref_desc.h"
#include "../network/checksum.h"
#include "../simdebug.h"


vehicle_desc_t *vehicle_desc_t::any_vehicle = NULL;


void vehicle_desc_t::calc_checksum(checksum_t *chk) const
{
	PAKSET_INFO("","obj=vehicle");
	obj_desc_transport_related_t::calc_checksum(chk);
#if MSG_LEVEL>0
	if(capacity) PAKSET_INFO("payload=","%d",capacity);
	if(loading_time!=1000) PAKSET_INFO("loading_time=","%d",loading_time);
	if(weight) PAKSET_INFO("weight=","%d",weight);
	if(power) PAKSET_INFO("power=","%d",power);
	if(running_cost) PAKSET_INFO("running_cost=","%d",running_cost);
	if(gear!=64) PAKSET_INFO("gear=","%d",(gear*100)/64);
	if(len!=8) PAKSET_INFO("length=","%d",len);
	if(const xref_desc_t *xref = get_child<xref_desc_t>(2)) PAKSET_INFO("freight=","%s",xref->get_name());
	switch((int)engine_type) {
		case electric:  PAKSET_INFO("waytype=","electric"); break;
		case steam:     PAKSET_INFO("waytype=","steam"); break;
		case bio:       PAKSET_INFO("waytype=","bio"); break;
		case sail:      PAKSET_INFO("waytype=","sail"); break;
		case fuel_cell: PAKSET_INFO("waytype=","fuel_cell"); break;
		case hydrogene: PAKSET_INFO("waytype=","hydrogene"); break;
		case battery:   PAKSET_INFO("waytype=","battery"); break;
		case unknown:   PAKSET_INFO("waytype=","unknown"); break;
		default:
		case diesel:
//		                PAKSET_INFO("waytype=","diesel"); 
			break;
	}
	// leader constraints
	for(uint16 i=0; i<leader_count; i++) {
		if(const xref_desc_t *xref = get_child<xref_desc_t>(6+i))
			PAKSET_INFO("constraint[prev]","[%d]=%s",i,xref->get_name());
	}
	// trailer constraints
	for(uint16 i=0; i<trailer_count; i++) {
		if(const xref_desc_t *xref = get_child<xref_desc_t>(6+i+leader_count))
			PAKSET_INFO("constraint[next]","[%d]=%s",i,xref->get_name());
	}
	PAKSET_INFO("--","");
#endif
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

