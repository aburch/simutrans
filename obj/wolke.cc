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
#include "../simtools.h"
#include "wolke.h"

#include "../dataobj/loadsave.h"

#include "../tpl/vector_tpl.h"


vector_tpl<const skin_besch_t *>wolke_t::all_clouds(0);

bool wolke_t::register_besch(const skin_besch_t* besch)
{
	// avoid duplicates with same name
	FOR(vector_tpl<skin_besch_t const*>, & i, all_clouds) {
		if (strcmp(i->get_name(), besch->get_name()) == 0) {
			i = besch;
			return true;
		}
	}
	return all_clouds.append_unique( besch );
}



wolke_t::wolke_t(koord3d pos, sint8 x_off, sint8 y_off, const skin_besch_t* besch ) :
	obj_no_info_t(pos)
{
	cloud_nr = all_clouds.index_of(besch);
	base_y_off = clamp( (sint16)y_off - 8, -128, 127 );
	set_xoff( x_off );
	set_yoff( base_y_off );
	insta_zeit = 0;
}



wolke_t::~wolke_t()
{
	mark_image_dirty( get_bild(), 0 );
	if(  insta_zeit != 2499  ) {
		if(  !welt->sync_way_eyecandy_remove( this )  ) {
			dbg->error( "wolke_t::~wolke_t()", "wolke not in the correct sync list" );
		}
	}
}



wolke_t::wolke_t(loadsave_t* const file) : obj_no_info_t()
{
	rdwr(file);
}


image_id wolke_t::get_bild() const
{
	const skin_besch_t *besch = all_clouds[cloud_nr];
	return besch->get_bild_nr( (insta_zeit*besch->get_bild_anzahl())/2500 );
}



void wolke_t::rdwr(loadsave_t *file)
{
	// not saving cloads! (and loading only for compatibility)
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



bool wolke_t::sync_step(uint32 delta_t)
{
	insta_zeit += delta_t;
	if(insta_zeit>=2499) {
		// delete wolke ...
		insta_zeit = 2499;
		return false;
	}
	// move cloud up
	sint8 ymove = ((insta_zeit*OBJECT_OFFSET_STEPS) >> 12);
	if(  base_y_off-ymove!=get_yoff()  ) {
		// move/change cloud ... (happens much more often than image change => image change will be always done when drawing)
		if(!get_flag(obj_t::dirty)) {
			mark_image_dirty(get_bild(),0);
		}
		set_yoff(  base_y_off - ymove  );
		set_flag(obj_t::dirty);
	}
	return true;
}


// called during map rotation
void wolke_t::rotate90()
{
	// restore pure yoff
	set_yoff( base_y_off + 8 );
	obj_t::rotate90();
	// .. and recalc smoke offsets
	base_y_off = clamp( (sint16)get_yoff()-8, -128, 127 );
	set_yoff( base_y_off - ((insta_zeit*OBJECT_OFFSET_STEPS) >> 12) );
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
