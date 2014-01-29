/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

/*
 * Basic class of all visible things
 *
 * Hj. Maltahner
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "simdebug.h"
#include "display/simimg.h"
#include "simcolor.h"
#include "display/simgraph.h"
#include "display/viewport.h"
#include "gui/simwin.h"
#include "player/simplay.h"
#include "simobj.h"
#include "simworld.h"
#include "obj/baum.h"
#include "vehicle/simvehikel.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "boden/grund.h"
#include "gui/obj_info.h"
#include "utils/cbuffer_t.h"
#include "utils/simstring.h"


/**
 * Pointer to the world of this thing. Static to conserve space.
 * Change to instance variable once more than one world is available.
 * @author Hj. Malthaner
 */
karte_ptr_t obj_t::welt;

bool obj_t::show_owner = false;


void obj_t::init()
{
	pos = koord3d::invalid;

	xoff = 0;
	yoff = 0;

	besitzer_n = PLAYER_UNOWNED;

	flags = keine_flags;
	set_flag(dirty);
}


obj_t::obj_t()
{
	init();
}


obj_t::obj_t(loadsave_t *file)
{
	init();
	rdwr(file);
}


obj_t::obj_t(koord3d pos)
{
	init();
	this->pos = pos;
}


// removes an object and tries to delete it also from the corresponding objlist
obj_t::~obj_t()
{
	destroy_win((ptrdiff_t)this);

	if(flags&not_on_map  ||  !welt->is_within_limits(pos.get_2d())) {
		return;
	}

	// find object on the map and remove it
	grund_t *gr = welt->lookup(pos);
	if(!gr  ||  !gr->obj_remove(this)) {
		// not found? => try harder at all map locations
		dbg->warning("obj_t::~obj_t()","couldn't remove %p from %d,%d,%d",this, pos.x , pos.y, pos.z);

		// first: try different height ...
		gr = welt->access(pos.get_2d())->get_boden_von_obj(this);
		if(gr  &&  gr->obj_remove(this)) {
			dbg->warning("obj_t::~obj_t()",
				"removed %p from %d,%d,%d, but it should have been on %d,%d,%d",
				this,
				gr->get_pos().x, gr->get_pos().y, gr->get_pos().z,
				pos.x, pos.y, pos.z);
			return;
		}

		// then search entire map
		koord k;
		for(k.y=0; k.y<welt->get_size().y; k.y++) {
			for(k.x=0; k.x<welt->get_size().x; k.x++) {
				grund_t *gr = welt->access(k)->get_boden_von_obj(this);
				if (gr && gr->obj_remove(this)) {
					dbg->warning("obj_t::~obj_t()",
						"removed %p from %d,%d,%d, but it should have been on %d,%d,%d",
						this,
						gr->get_pos().x, gr->get_pos().y, gr->get_pos().z,
						pos.x, pos.y, pos.z);
					return;
				}
			}
		}
	}
}


/**
 * sets owner of object
 */
void obj_t::set_besitzer(spieler_t *sp)
{
	int i = welt->sp2num(sp);
	assert(i>=0);
	besitzer_n = (uint8)i;
}


spieler_t *obj_t::get_besitzer() const
{
	return welt->get_spieler(besitzer_n);
}


/* the only general info we can give is the name
 * we want to format it nicely,
 * with two linebreaks at the end => thus the little extra effort
 */
void obj_t::info(cbuffer_t & buf) const
{
	char              translation[256];
	char const* const owner =
		besitzer_n == 1              ? translator::translate("Eigenbesitz\n")   :
		besitzer_n == PLAYER_UNOWNED ? translator::translate("Kein Besitzer\n") :
		get_besitzer()->get_name();
	tstrncpy(translation, owner, lengthof(translation));
	// remove trailing linebreaks etc.
	rtrim(translation);
	buf.append( translation );
	// only append linebreaks if not empty
	if(  buf.len()>0  ) {
		buf.append( "\n\n" );
	}
}


void obj_t::zeige_info()
{
	create_win( new obj_infowin_t(this), w_info, (ptrdiff_t)this);
}


// returns NULL, if removal is allowed
const char *obj_t::ist_entfernbar(const spieler_t *sp)
{
	if(besitzer_n==PLAYER_UNOWNED  ||  welt->get_spieler(besitzer_n) == sp  ||  welt->get_spieler(1) == sp) {
		return NULL;
	}
	else {
		return "Der Besitzer erlaubt das Entfernen nicht";
	}
}


void obj_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "obj_t" );
	if(  file->get_version()<101000) {
		pos.rdwr( file );
	}

	sint8 byte = (sint8)(((sint16)16*(sint16)xoff)/OBJECT_OFFSET_STEPS);
	file->rdwr_byte(byte);
	xoff = (sint8)(((sint16)byte*OBJECT_OFFSET_STEPS)/16);
	byte = (sint8)(((sint16)16*(sint16)yoff)/OBJECT_OFFSET_STEPS);
	file->rdwr_byte(byte);
	yoff = (sint8)(((sint16)byte*OBJECT_OFFSET_STEPS)/16);
	byte = besitzer_n;
	file->rdwr_byte(byte);
	besitzer_n = byte;
}


/**
 * draw the object
 * the dirty-flag is reset from objlist_t::display_obj_fg, or objlist_t::display_overlay when multithreaded
 */
#ifdef MULTI_THREAD
void obj_t::display(int xpos, int ypos, const sint8 clip_num) const
#else
void obj_t::display(int xpos, int ypos) const
#endif
{
	image_id bild = get_bild();
	image_id const outline_bild = get_outline_bild();
	if(  bild!=IMG_LEER  ||  outline_bild!=IMG_LEER  ) {
		const int raster_width = get_current_tile_raster_width();
		const bool is_dirty = get_flag(obj_t::dirty);

		if (vehikel_basis_t const* const v = obj_cast<vehikel_basis_t>(this)) {
			// vehicles need finer steps to appear smoother
			v->get_screen_offset( xpos, ypos, raster_width );
		}
		xpos += tile_raster_scale_x(get_xoff(), raster_width);
		ypos += tile_raster_scale_y(get_yoff(), raster_width);

		const int start_ypos = ypos;
		for(  int j=0;  bild!=IMG_LEER;  ) {

			if(  besitzer_n != PLAYER_UNOWNED  ) {
				if(  obj_t::show_owner  ) {
#ifdef MULTI_THREAD
					display_blend( bild, xpos, ypos, besitzer_n, (welt->get_spieler(besitzer_n)->get_player_color1()+2) | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty, clip_num );
#else
					display_blend( bild, xpos, ypos, besitzer_n, (welt->get_spieler(besitzer_n)->get_player_color1()+2) | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty );
#endif
				}
				else {
#ifdef MULTI_THREAD
					display_color( bild, xpos, ypos, besitzer_n, true, is_dirty, clip_num );
#else
					display_color( bild, xpos, ypos, besitzer_n, true, is_dirty );
#endif
				}
			}
			else {
#ifdef MULTI_THREAD
				display_normal( bild, xpos, ypos, 0, true, is_dirty, clip_num );
#else
				display_normal( bild, xpos, ypos, 0, true, is_dirty );
#endif
			}
			// this obj has another image on top (e.g. skyscraper)
			ypos -= raster_width;
			bild = get_bild(++j);
		}

		if(  outline_bild != IMG_LEER  ) {
			// transparency?
			const PLAYER_COLOR_VAL transparent = get_outline_colour();
			if(  TRANSPARENT_FLAGS&transparent  ) {
				// only transparent outline
#ifdef MULTI_THREAD
				display_blend( get_outline_bild(), xpos, start_ypos, besitzer_n, transparent, 0, is_dirty, clip_num );
#else
				display_blend( get_outline_bild(), xpos, start_ypos, besitzer_n, transparent, 0, is_dirty );
#endif
			}
			else if(  obj_t::get_flag( highlight )  ) {
				// highlight this tile
#ifdef MULTI_THREAD
				display_blend( get_bild(), xpos, start_ypos, besitzer_n, COL_RED | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty, clip_num );
#else
				display_blend( get_bild(), xpos, start_ypos, besitzer_n, COL_RED | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty );
#endif
			}
		}
		else if(  obj_t::get_flag( highlight )  ) {
			// highlight this tile
#ifdef MULTI_THREAD
			display_blend( get_bild(), xpos, start_ypos, besitzer_n, COL_RED | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty, clip_num );
#else
			display_blend( get_bild(), xpos, start_ypos, besitzer_n, COL_RED | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty );
#endif
		}
	}
}


// called during map rotation
void obj_t::rotate90()
{
	// most basic: rotate coordinate
	pos.rotate90( welt->get_size().y-1 );
	if(xoff!=0) {
		sint8 new_dx = -2*yoff;
		yoff = xoff/2;
		xoff = new_dx;
	}
}


#ifdef MULTI_THREAD
void obj_t::display_after(int xpos, int ypos, const sint8 clip_num) const
#else
void obj_t::display_after(int xpos, int ypos, bool) const
#endif
{
	image_id bild = get_after_bild();
	if(  bild != IMG_LEER  ) {
		const int raster_width = get_current_tile_raster_width();
		const bool is_dirty = get_flag( obj_t::dirty );

		xpos += tile_raster_scale_x( get_xoff(), raster_width );
		ypos += tile_raster_scale_y( get_yoff(), raster_width );

		if(  besitzer_n != PLAYER_UNOWNED  ) {
			if(  obj_t::show_owner  ) {
#ifdef MULTI_THREAD
				display_blend( bild, xpos, ypos, besitzer_n, (welt->get_spieler(besitzer_n)->get_player_color1()+2) | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty, clip_num );
#else
				display_blend( bild, xpos, ypos, besitzer_n, (welt->get_spieler(besitzer_n)->get_player_color1()+2) | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty );
#endif
			}
			else if(  obj_t::get_flag( highlight )  ) {
				// highlight this tile
#ifdef MULTI_THREAD
				display_blend( bild, xpos, ypos, besitzer_n, COL_RED | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty, clip_num );
#else
				display_blend( bild, xpos, ypos, besitzer_n, COL_RED | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty );
#endif
			}
			else {
#ifdef MULTI_THREAD
				display_color( bild, xpos, ypos, besitzer_n, true, is_dirty, clip_num );
#else
				display_color( bild, xpos, ypos, besitzer_n, true, is_dirty );
#endif
			}
		}
		else if(  obj_t::get_flag( highlight )  ) {
			// highlight this tile
#ifdef MULTI_THREAD
			display_blend( bild, xpos, ypos, besitzer_n, COL_RED | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty, clip_num );
#else
			display_blend( bild, xpos, ypos, besitzer_n, COL_RED | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty );
#endif
		}
		else {
#ifdef MULTI_THREAD
			display_normal( bild, xpos, ypos, 0, true, is_dirty, clip_num );
#else
			display_normal( bild, xpos, ypos, 0, true, is_dirty );
#endif
		}
	}
}


/*
 * when a vehicle moves or a cloud moves, it needs to mark the old spot as dirty (to copy to screen)
 * sometimes they have an extra offset, this is the yoff parameter
* @author prissi
 */
void obj_t::mark_image_dirty(image_id bild, sint16 yoff) const
{
	if(  bild != IMG_LEER  ) {
		const sint16 rasterweite = get_tile_raster_width();
		int xpos=0, ypos=0;
		if(  is_moving()  ) {
			vehikel_basis_t const* const v = obj_cast<vehikel_basis_t>(this);
			// vehicles need finer steps to appear smoother
			v->get_screen_offset( xpos, ypos, get_tile_raster_width() );
		}

		viewport_t *vp = welt->get_viewport();
		scr_coord scr_pos = vp->get_screen_coord(get_pos(), koord(xpos + get_xoff(), ypos + get_yoff() + yoff));

		// mark the region after the image as dirty
		display_mark_img_dirty( bild, scr_pos.x, scr_pos.y );

		// too close to border => set dirty to be sure (smoke, skyscrapes, birds, or the like)
		KOORD_VAL xbild, ybild, wbild, hbild;
		display_get_image_offset( bild, &xbild, &ybild, &wbild, &hbild );
		const sint16 distance_to_border = 3 - (yoff+get_yoff()+ybild)/(rasterweite/4);
		if(  pos.x <= distance_to_border  ||  pos.y <= distance_to_border  ) {
			// but only if the image is actually visible ...
			if(   scr_pos.x+xbild+wbild >= 0  &&  xpos <= display_get_width()  &&   scr_pos.y+ybild+hbild >= 0  &&  ypos+ybild < display_get_height()  ) {
				welt->set_background_dirty();
			}
		}
	}
}
