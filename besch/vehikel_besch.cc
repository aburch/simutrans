#include "vehikel_besch.h"
#include "xref_besch.h"
#include "../network/checksum.h"

void vehikel_besch_t::calc_checksum(checksum_t *chk) const
{
	obj_besch_transport_related_t::calc_checksum(chk);
	chk->input(zuladung);
	chk->input(loading_time);
	chk->input(gewicht);
	chk->input(leistung);
	chk->input(running_cost);
	chk->input(fixed_cost);
	chk->input(gear);
	chk->input(len);
	chk->input(vorgaenger);
	chk->input(nachfolger);
	chk->input(engine_type);
	// freight
	const xref_besch_t *xref = get_child<xref_besch_t>(2);
	chk->input(xref ? xref->get_name() : "NULL");
	// vehicle constraints
	for(uint8 i=0; i<vorgaenger+nachfolger; i++) {
		const xref_besch_t *xref = get_child<xref_besch_t>(6+i);
		chk->input(xref ? xref->get_name() : "NULL");
	}
}
