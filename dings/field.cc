/*
 * field, which can extend factories
 *
 * Hj. Malthaner
 */

#include <string.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simfab.h"
#include "../simmem.h"
#include "../simimg.h"

#include "../boden/grund.h"

#include "../gui/thing_info.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"

#include "field.h"

// ***************** static ***********************

stringhashtable_tpl<const field_besch_t *> field_t::besch_table;

void field_t::register_besch(field_besch_t *besch, const char*name)
{
    besch_table.put(name, besch);
}

const field_besch_t *field_t::gib_besch(const char *name)
{
    return besch_table.get(name);
}



// ***************** normal ***********************

field_t::field_t(karte_t *welt, koord3d p, spieler_t *sp, const field_besch_t *besch, fabrik_t *fab) : ding_t(welt)
{
	this->besch = besch;
	this->fab = fab;
	setze_besitzer( sp );
	p.z = welt->max_hgt(p.gib_2d());
	setze_pos( p );
}



field_t::~field_t()
{
	fab->remove_field_at( gib_pos().gib_2d() );
}



const char *
field_t::ist_entfernbar(const spieler_t *)
{
	// we allow removal, if there is less than
	return (fab->gib_field_count() > besch->gib_min_fields()) ? NULL : "Not enough fields would remain.";
}



// remove costs
void
field_t::entferne(spieler_t *sp)
{
	if(sp) {
		sp->buche(umgebung_t::cst_multiply_remove_field, gib_pos().gib_2d(), COST_CONSTRUCTION);
	}
}



// return the  right month graphic for factories
image_id
field_t::gib_bild() const
{
	const skin_besch_t *s=besch->gib_bilder();
	uint16 anzahl=s->gib_bild_anzahl() - besch->has_snow_bild();
	if(besch->has_snow_bild()  &&  gib_pos().z>=welt->get_snowline()) {
		// last images will be shown above snowline
		return s->gib_bild_nr(anzahl);
	}
	else {
		// resolution 1/8th month (0..95)
		const uint32 yearsteps = (welt->get_current_month()%12)*8 + ((welt->gib_zeit_ms()>>(welt->ticks_bits_per_tag-3))&7) + 1;
		const image_id bild = s->gib_bild_nr( (anzahl*yearsteps-1)/96 );
		if((anzahl*yearsteps-1)%96<anzahl) {
			mark_image_dirty( bild, 0 );
		}
		return bild;
	}
}



/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void field_t::zeige_info()
{
	// show the info of the corresponding factory
	grund_t *gr = welt->lookup(fab->gib_pos());
	gebaeude_t* gb = gr->find<gebaeude_t>();
	gb->zeige_info();
}




