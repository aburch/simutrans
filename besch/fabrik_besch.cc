#include "fabrik_besch.h"
#include "xref_besch.h"
#include "../network/checksum.h"


void field_class_besch_t::calc_checksum(checksum_t *chk) const
{
	chk->input(production_per_field);
	chk->input(storage_capacity);
	chk->input(spawn_weight);
}


void field_group_besch_t::calc_checksum(checksum_t *chk) const
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
	chk->input(get_ware()->get_name());
}


void fabrik_produkt_besch_t::calc_checksum(checksum_t *chk) const
{
    chk->input(kapazitaet);
	chk->input(faktor);
	chk->input(get_ware()->get_name());
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
	chk->input(expand_probability);
	chk->input(expand_minimum);
	chk->input(expand_range);
	chk->input(expand_times);
	chk->input(electric_boost);
	chk->input(pax_boost);
	chk->input(mail_boost);
	chk->input(electric_amount);
	chk->input(pax_demand);
	chk->input(mail_demand);

	for (uint8 i=0; i<lieferanten; i++) {
		const fabrik_lieferant_besch_t *supp = get_lieferant(i);
		supp->calc_checksum(chk);
	}

	for (uint8 i=0; i<produkte; i++) {
		const fabrik_produkt_besch_t *prod = get_produkt(i);
		prod->calc_checksum(chk);
	}

	const field_group_besch_t *field_group = get_field_group();
	if (field_group) {
		field_group->calc_checksum(chk);
	}

	get_haus()->calc_checksum(chk);
}
