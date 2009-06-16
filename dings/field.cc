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

#include "../gui/thing_info.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/umgebung.h"

#include "field.h"

// ***************** static ***********************

stringhashtable_tpl<const field_besch_t *> field_t::besch_table;

void field_t::register_besch(field_besch_t *besch, const char*name)
{
	// remove duplicates
	if(  besch_table.remove( name )  ) {
		dbg->warning( "field_t::register_besch()", "Object %s was overlaid by addon!", name );
	}
	besch_table.put(name, besch);
}

const field_besch_t *field_t::get_besch(const char *name)
{
	return besch_table.get(name);
}



// ***************** normal ***********************

field_t::field_t(karte_t *welt, koord3d p, spieler_t *sp, const field_besch_t *besch, fabrik_t *fab) : ding_t(welt)
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



const char *
field_t::ist_entfernbar(const spieler_t *)
{
	// we allow removal, if there is less than
	return (fab->get_field_count() > besch->get_min_fields()) ? NULL : "Not enough fields would remain.";
}



// remove costs
void
field_t::entferne(spieler_t *sp)
{
	spieler_t::accounting( sp, welt->get_einstellungen()->cst_multiply_remove_field, get_pos().get_2d(), COST_CONSTRUCTION);
	mark_image_dirty( get_bild(), 0 );
}



// return the  right month graphic for factories
image_id
field_t::get_bild() const
{
	const skin_besch_t *s=besch->get_bilder();
	int anzahl=s->get_bild_anzahl() - besch->has_snow_bild();
	if(besch->has_snow_bild()  &&  get_pos().z>=welt->get_snowline()) {
		// last images will be shown above snowline
		return s->get_bild_nr(anzahl);
	}
	else {
		// welt->get_yearsteps(): resolution 1/8th month (0..95)
		int anzahl_yearsteps = anzahl * (welt->get_yearsteps() + 1) - 1;
		const image_id bild = s->get_bild_nr( anzahl_yearsteps / 96 );
		if(anzahl_yearsteps % 96 < anzahl) {
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
	grund_t *gr = welt->lookup(fab->get_pos());
	gebaeude_t* gb = gr->find<gebaeude_t>();
	gb->zeige_info();
}
