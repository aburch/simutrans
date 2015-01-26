/*
 * field, which can extend factories
 *
 * Hj. Malthaner
 */

#include <string.h>

#include "../simworld.h"
#include "../simobj.h"
#include "../simfab.h"
#include "../display/simimg.h"

#include "../boden/grund.h"

#include "../dataobj/loadsave.h"

#include "../player/simplay.h"

#include "field.h"


field_t::field_t(koord3d p, player_t *player, const field_class_besch_t *besch, fabrik_t *fab) : 
#ifdef INLINE_OBJ_TYPE
	obj_t(obj_t::field)
#else
	obj_t()
#endif
{
	this->besch = besch;
	this->fab = fab;
	set_owner( player );
	p.z = welt->max_hgt(p.get_2d());
	set_pos( p );
}



field_t::~field_t()
{
	fab->remove_field_at( get_pos().get_2d() );
}



const char *field_t:: is_deletable(const player_t *)
{
	// Allow removal provided that the number of fields do not fall below half the minimum
	return (fab->get_field_count() > fab->get_besch()->get_field_group()->get_min_fields() / 2) ? NULL : "Not enough fields would remain.";
}



// remove costs
void field_t::cleanup(player_t *player)
{
	player_t::book_construction_costs(player, welt->get_settings().cst_multiply_remove_field, get_pos().get_2d(), ignore_wt);
	mark_image_dirty( get_bild(), 0 );
}



// return the  right month graphic for factories
image_id field_t::get_bild() const
{
	const skin_besch_t *s=besch->get_bilder();
	uint16 anzahl=s->get_bild_anzahl() - besch->has_snow_image();
	if(  besch->has_snow_image()  &&  (get_pos().z >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate)  ) {
		// last images will be shown above snowline
		return s->get_bild_nr(anzahl);
	}
	else {
		// welt->get_yearsteps(): resolution 1/8th month (0..95)
		int anzahl_yearsteps = anzahl * (welt->get_yearsteps() + 1) - 1;
		const image_id image = s->get_bild_nr( anzahl_yearsteps / 96 );
		if(anzahl_yearsteps % 96 < anzahl) {
			mark_image_dirty( image, 0 );
		}
		return image;
	}
}



/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void field_t::show_info()
{
	// show the info of the corresponding factory
	fab->show_info();
}
