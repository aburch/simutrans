/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#include <string.h>

#include "../simworld.h"
#include "../simobj.h"
#include "../utils/simrandom.h"
#include "wolke.h"

#include "../dataobj/loadsave.h"

#include "../tpl/vector_tpl.h"


vector_tpl<const skin_desc_t *>wolke_t::all_clouds(0);

bool wolke_t::register_desc(const skin_desc_t* desc)
{
	// avoid duplicates with same name
	FOR(vector_tpl<skin_desc_t const*>, & i, all_clouds) {
		if (strcmp(i->get_name(), desc->get_name()) == 0) {
			i = desc;
			return true;
		}
	}
	return all_clouds.append_unique( desc );
}



wolke_t::wolke_t(koord3d pos, sint8 x_off, sint8 y_off, const skin_desc_t* desc ) :
#ifdef INLINE_OBJ_TYPE
    obj_no_info_t(obj_t::sync_wolke, pos)
#else
    obj_no_info_t(pos)
#endif
{
	cloud_nr = all_clouds.index_of(desc);
	base_y_off = clamp( (sint16)y_off - 8, -128, 127 );
	set_xoff( x_off );
	set_yoff( base_y_off );
	purchase_time = 0;
}



wolke_t::~wolke_t()
{
	mark_image_dirty( get_image(), 0 );
	if(  purchase_time != 2499  ) {
		welt->sync_way_eyecandy.remove( this );
	}
}


wolke_t::wolke_t(loadsave_t* const file) :
#ifdef INLINE_OBJ_TYPE
	obj_no_info_t(obj_t::sync_wolke)
#else
	obj_no_info_t()
#endif
{
	rdwr(file);
}


image_id wolke_t::get_image() const
{
	const skin_desc_t *desc = all_clouds[cloud_nr];
	return desc->get_image_id( (purchase_time*desc->get_count())/2500 );
}



void wolke_t::rdwr(loadsave_t *file)
{
	// not saving clouds! (and loading only for compatibility)
	assert(file->is_loading());

	obj_t::rdwr( file );

	cloud_nr = 0;
	purchase_time = 0;

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
	const image_id old_img = get_image();

	purchase_time += delta_t;
	if (purchase_time >= 2499) {
		// delete wolke ...
		purchase_time = 2499;
		return SYNC_DELETE;
	}
	const image_id new_img = get_image();

	// move cloud up
	const sint8 new_yoff = base_y_off - ((purchase_time * OBJECT_OFFSET_STEPS) >> 12);
	if (new_yoff != get_yoff() || new_img != old_img) {
		// move/change cloud ... (happens much more often than image change => image change will be always done when drawing)
		if (!get_flag(obj_t::dirty)) {
			set_flag(obj_t::dirty);
			mark_image_dirty(old_img, 0);
			if (new_img != old_img) {
				mark_image_dirty(new_img, 0);
			}
		}
		set_yoff(new_yoff);
	}
	return SYNC_OK;
}


// called during map rotation
void wolke_t::rotate90()
{
	// restore pure yoff
	set_yoff( base_y_off + 8 );
	obj_t::rotate90();
	// .. and recalc smoke offsets
	base_y_off = clamp( (sint16)get_yoff()-8, -128, 127 );
	set_yoff( base_y_off - ((purchase_time*OBJECT_OFFSET_STEPS) >> 12) );
}

/***************************** just for compatibility, the old raucher and smoke clouds *********************************/

raucher_t::raucher_t(loadsave_t *file) : 
#ifdef INLINE_OBJ_TYPE
	obj_t(obj_t::raucher)
#else
	obj_t()
#endif
{
	assert(file->is_loading());
	obj_t::rdwr( file );
	const char *s = NULL;
	file->rdwr_str(s);
	free(const_cast<char *>(s));

	// do not remove from this position, since there will be nothing
	obj_t::set_flag(obj_t::not_on_map);
}


async_wolke_t::async_wolke_t(loadsave_t *file) : 
#ifdef INLINE_OBJ_TYPE
	obj_t(obj_t::async_wolke)
#else
	obj_t()
#endif
{
	// not saving clouds!
	assert(file->is_loading());

	obj_t::rdwr( file );

	uint32 dummy;
	file->rdwr_long(dummy);

	// do not remove from this position, since there will be nothing
	obj_t::set_flag(obj_t::not_on_map);
}
