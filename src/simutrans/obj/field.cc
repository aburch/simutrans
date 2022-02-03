/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>

#include "../world/simworld.h"
#include "simobj.h"
#include "../simfab.h"
#include "../display/simimg.h"

#include "../ground/grund.h"

#include "../dataobj/loadsave.h"

#include "../player/simplay.h"

#include "field.h"


field_t::field_t(koord3d p, player_t *player, const field_class_desc_t *desc, fabrik_t *fab) : obj_t()
{
	this->desc = desc;
	this->fab = fab;
	set_owner( player );
	p.z = welt->max_hgt(p.get_2d());
	set_pos( p );
}



field_t::~field_t()
{
	// mark field image area as dirty for removal
	mark_image_dirty( get_image(), 0 );

	fab->remove_field_at( get_pos().get_2d() );
}



const char *field_t::is_deletable(const player_t *)
{
	// we allow removal, if there is less than
	return (fab->get_field_count() > fab->get_desc()->get_field_group()->get_min_fields()) ? NULL : "Not enough fields would remain.";
}



// remove costs
void field_t::cleanup(player_t *player)
{
	player_t::book_construction_costs(player, welt->get_settings().cst_multiply_remove_field, get_pos().get_2d(), ignore_wt);
	mark_image_dirty( get_image(), 0 );
}



// return the  right month graphic for factories
image_id field_t::get_image() const
{
	const skin_desc_t *s=desc->get_images();
	uint16 count=s->get_count() - desc->has_snow_image();
	if(  desc->has_snow_image()  &&  (get_pos().z >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate)  ) {
		// last images will be shown above snowline
		return s->get_image_id(count);
	}
	else {
		// resolution 1/8th month (0..95)
		const uint32 yearsteps = (welt->get_current_month()%12)*8 + ((welt->get_ticks()>>(welt->ticks_per_world_month_shift-3))&7) + 1;
		const image_id image = s->get_image_id( (count*yearsteps-1)/96 );
		if((count*yearsteps-1)%96<count) {
			mark_image_dirty( image, 0 );
		}
		return image;
	}
}


void field_t::show_info()
{
	// show the info of the corresponding factory
	fab->open_info_window();
}
