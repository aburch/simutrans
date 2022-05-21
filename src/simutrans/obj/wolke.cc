/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <string.h>

#include "../world/simworld.h"
#include "simobj.h"
#include "../utils/simrandom.h"
#include "wolke.h"

#include "../dataobj/loadsave.h"

#include "../descriptor/factory_desc.h"

#include "../tpl/vector_tpl.h"

vector_tpl<const skin_desc_t *>wolke_t::all_clouds(0);

freelist_iter_tpl<wolke_t> wolke_t::fl;

bool wolke_t::register_desc(const skin_desc_t* desc)
{
	// avoid duplicates with same name
	for(skin_desc_t const* & i : all_clouds) {
		if (strcmp(i->get_name(), desc->get_name()) == 0) {
			i = desc;
			return true;
		}
	}
	return all_clouds.append_unique( desc );
}


wolke_t::wolke_t(koord3d pos, sint8 b_x_off, sint8 b_y_off, sint16 b_h_off, uint16 lt, uint16 ul, const skin_desc_t* desc ) :
	obj_no_info_t(pos)
{
	cloud_nr = all_clouds.index_of(desc);
	base_y_off = b_h_off;
	set_xoff( b_x_off );
	set_yoff( b_y_off );
	insta_zeit = 0;
	lifetime = lt;
	uplift = ul;
}


wolke_t::~wolke_t()
{
	mark_image_dirty( get_image(), calc_yoff() );
}


wolke_t::wolke_t(loadsave_t* const file) : obj_no_info_t()
{
	rdwr(file);
}


image_id wolke_t::get_front_image() const
{
	const skin_desc_t *desc = all_clouds[cloud_nr];
	return desc->get_image_id( (insta_zeit*desc->get_count())/(lifetime+1) );
}


sint16 wolke_t::calc_yoff() const
{
	return tile_raster_scale_y( base_y_off - (((long)insta_zeit * uplift * OBJECT_OFFSET_STEPS) >> 16), get_current_tile_raster_width() );
}


void wolke_t::rdwr(loadsave_t *file)
{
	// not saving clouds! (and loading only for compatibility)
	assert(file->is_loading());

	obj_t::rdwr( file );

	cloud_nr = 0;
	insta_zeit = 0;

	uint32 ldummy = 0;
	file->rdwr_long(ldummy);

	uint16 dummy = 0;
	file->rdwr_short(dummy);
	file->rdwr_short(dummy);

	// do not remove from this position, since there will be nothing
	obj_t::set_flag(obj_t::not_on_map);
}


sync_result wolke_t::sync_step(uint32 delta_t)
{
	// we query the image twice, since it may have changed (there are sure more efficient ways for that ...
	const image_id old_img = get_front_image();
	const sint16 old_yoff = calc_yoff();
	insta_zeit += delta_t;
	if(  insta_zeit >= lifetime  ) {
		// delete wolke ...
		set_flag( obj_t::dirty );
		mark_image_dirty( old_img, old_yoff );
		insta_zeit = lifetime;
		return SYNC_DELETE;
	}
	const image_id new_img = get_front_image();

	// move cloud up
	const sint16 new_yoff = calc_yoff();
	if(  new_img != old_img  ) {
		// change cloud
		set_flag( obj_t::dirty );
		mark_image_dirty( old_img, old_yoff );
	}
	if( new_yoff != old_yoff ) {
		// move cloud
		set_flag( obj_t::dirty );
		mark_image_dirty( old_img, old_yoff );
		// wind effect
		set_xoff( get_xoff() - sim_async_rand(2));
	}
	return SYNC_OK;
}


#ifdef MULTI_THREAD
void wolke_t::display_after( int xpos, int ypos, const sint8 extra_param ) const
#else
void wolke_t::display_after( int xpos, int ypos, bool extra_param ) const
#endif
{
	obj_t::display_after( xpos, ypos + calc_yoff(), extra_param );
}



// called during map rotation
void wolke_t::rotate90()
{
	// most basic: rotate coordinate
	obj_t::rotate90();

	if( lifetime != DEFAULT_EXHAUSTSMOKE_TIME ) {
		// since factory smoke may come from the wrong tile, remove it on rotation
		insta_zeit = lifetime;
	}
}


/***************************** just for compatibility, the old raucher and smoke clouds *********************************/

raucher_t::raucher_t(loadsave_t *file) : obj_t()
{
	assert(file->is_loading());
	obj_t::rdwr( file );
	const char *s = NULL;
	file->rdwr_str(s);
	free(const_cast<char *>(s));

	// do not remove from this position, since there will be nothing
	obj_t::set_flag(obj_t::not_on_map);
}


async_wolke_t::async_wolke_t(loadsave_t *file) : obj_t()
{
	// not saving clouds!
	assert(file->is_loading());

	obj_t::rdwr( file );

	uint32 dummy;
	file->rdwr_long(dummy);

	// do not remove from this position, since there will be nothing
	obj_t::set_flag(obj_t::not_on_map);
}
