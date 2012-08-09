/*
 * field, which can extend factories
 *
 * Hj. Malthaner
 */

#include <string.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simfab.h"
#include "../simimg.h"

#include "../boden/grund.h"

#include "../dataobj/loadsave.h"

#include "../player/simplay.h"

#include "field.h"


field_t::field_t(karte_t *welt, koord3d p, spieler_t *sp, const field_class_besch_t *besch, fabrik_t *fab) : ding_t(welt)
{
	this->besch = besch;
	this->fab = fab;
	set_besitzer( sp );
	p.z = welt->max_hgt(p.get_2d());
	set_pos( p );
}



field_t::~field_t()
{
	fab->remove_field_at( get_pos().get_2d() );
}



const char *field_t::ist_entfernbar(const spieler_t *)
{
	// we allow removal, if there is less than
	return (fab->get_field_count() > fab->get_besch()->get_field_group()->get_min_fields()) ? NULL : "Not enough fields would remain.";
}



// remove costs
void field_t::entferne(spieler_t *sp)
{
	spieler_t::accounting(sp, welt->get_settings().cst_multiply_remove_field, get_pos().get_2d(), COST_CONSTRUCTION);
	mark_image_dirty( get_bild(), 0 );
}



// return the  right month graphic for factories
image_id field_t::get_bild() const
{
	const skin_besch_t *s=besch->get_bilder();
	uint16 anzahl=s->get_bild_anzahl() - besch->has_snow_image();
	if(besch->has_snow_image()  &&  get_pos().z>=welt->get_snowline()) {
		// last images will be shown above snowline
		return s->get_bild_nr(anzahl);
	}
	else {
		// resolution 1/8th month (0..95)
		const uint32 yearsteps = (welt->get_current_month()%12)*8 + ((welt->get_zeit_ms()>>(welt->ticks_per_world_month_shift-3))&7) + 1;
		const image_id bild = s->get_bild_nr( (anzahl*yearsteps-1)/96 );
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
	fab->zeige_info();
}
