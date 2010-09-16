#include "vehikel_besch.h"
#include "xref_besch.h"
#include "../utils/checksum.h"

void vehikel_besch_t::calc_checksum(checksum_t *chk) const
{
	chk->input(preis);
	chk->input(zuladung);
	chk->input(geschw);
	chk->input(gewicht);
	chk->input(leistung);
	chk->input(betriebskosten);
	chk->input(intro_date);
	chk->input(obsolete_date);
	chk->input(gear);
	chk->input(typ);
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
