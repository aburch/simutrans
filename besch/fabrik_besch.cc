#include "fabrik_besch.h"
#include "xref_besch.h"
#include "../utils/checksum.h"

void field_class_besch_t::calc_checksum(checksum_t *chk) const
{
	chk->input(production_per_field);
	chk->input(storage_capacity);
	chk->input(spawn_weight);
}

void field_besch_t::calc_checksum(checksum_t *chk) const
{
	chk->input(probability);
	chk->input(max_fields);
	chk->input(min_fields);
	chk->input(field_classes);
	for(uint16 i=0; i<field_classes; i++) {
		const field_class_besch_t *fc = get_field_class(i);
		fc->calc_checksum(chk);
	}
}


void fabrik_lieferant_besch_t::calc_checksum(checksum_t *chk) const
{
    chk->input(kapazitaet);
	chk->input(anzahl);
	chk->input(verbrauch);
	const xref_besch_t *xref = get_child<xref_besch_t>(0);
	chk->input(xref ? xref->get_name() : "NULL");
}


void fabrik_produkt_besch_t::calc_checksum(checksum_t *chk) const
{
    chk->input(kapazitaet);
	chk->input(faktor);
	const xref_besch_t *xref = get_child<xref_besch_t>(0);
	chk->input(xref ? xref->get_name() : "NULL");
}


void fabrik_besch_t::calc_checksum(checksum_t *chk) const
{
	chk->input((uint8)platzierung);
	chk->input(produktivitaet);
	chk->input(bereich);
	chk->input(gewichtung);
	chk->input(kennfarbe);
	chk->input(lieferanten);
	chk->input(produkte);
	chk->input(fields);
	chk->input(pax_level);
	chk->input(electricity_producer);

	for (uint8 i=0; i<lieferanten; i++) {
		const fabrik_lieferant_besch_t *supp = get_lieferant(i);
		supp->calc_checksum(chk);
	}

	for (uint8 i=0; i<produkte; i++) {
		const fabrik_produkt_besch_t *prod = get_produkt(i);
		prod->calc_checksum(chk);
	}

	const field_besch_t *field = get_field();
	if (field) {
		field->calc_checksum(chk);
	}

	get_haus()->calc_checksum(chk);
}
