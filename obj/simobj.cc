/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simobj.h"

#include "baum.h"

#include "../boden/grund.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../display/simgraph.h"
#include "../display/simimg.h"
#include "../display/viewport.h"
#include "../player/simplay.h"
#include "../gui/obj_info.h"
#include "../gui/simwin.h"
#include "../vehicle/vehicle_base.h"
#include "../simcolor.h"
#include "../simdebug.h"
#include "../simworld.h"
#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>


/**
 * Pointer to the world of this thing. Static to conserve space.
 * Change to instance variable once more than one world is available.
 */
karte_ptr_t obj_t::welt;

bool obj_t::show_owner = false;


void obj_t::init()
{
	pos = koord3d::invalid;

	xoff = 0;
	yoff = 0;

	owner_n = PLAYER_UNOWNED;

	flags = no_flags;
	set_flag(dirty);
}


obj_t::obj_t()
{
	init();
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
		dbg->warning("obj_t::~obj_t()", "Could not remove %s %p from (%s)", get_name(), (void *)this, pos.get_str());

		// first: try different height ...
		gr = welt->access(pos.get_2d())->get_boden_von_obj(this);
		if(gr  &&  gr->obj_remove(this)) {
			dbg->warning("obj_t::~obj_t()",
				"Removed %s %p from (%hi,%hi,%hhi), but it should have been on (%hi,%hi,%hhi)",
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
void obj_t::set_owner(player_t *player)
{
	int i = welt->sp2num(player);
	assert(i>=0);
	owner_n = (uint8)i;
}


player_t *obj_t::get_owner() const
{
	return welt->get_player(owner_n);
}


/* the only general info we can give is the name
 * we want to format it nicely,
 * with two linebreaks at the end => thus the little extra effort
 */
void obj_t::info(cbuffer_t & buf) const
{
	char              translation[256];
	char const* const owner =
		owner_n == 1              ? translator::translate("Eigenbesitz\n")   :
		owner_n == PLAYER_UNOWNED ? "" : // was translator::translate("Kein Besitzer\n") :
		get_owner()->get_name();
	tstrncpy(translation, owner, lengthof(translation));
	// remove trailing linebreaks etc.
	rtrim(translation);
	buf.append( translation );
	// only append linebreaks if not empty
	if(  buf.len()>0  ) {
		buf.append( "\n\n" );
	}
}


void obj_t::show_info()
{
	create_win( new obj_infowin_t(this), w_info, (ptrdiff_t)this);
}

bool obj_t::has_managed_lifecycle() const {
	return false;
}

// returns NULL, if removal is allowed
const char *obj_t::is_deletable(const player_t *player)
{
	if(owner_n==PLAYER_UNOWNED  ||  welt->get_player(owner_n) == player  ||  welt->get_public_player() == player) {
		return NULL;
	}
	else {
		return "Der Besitzer erlaubt das Entfernen nicht";
	}
}


void obj_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "obj_t" );
	if(  file->is_version_less(101, 0)  ) {
		pos.rdwr( file );
	}

	sint8 byte = (sint8)(((sint16)16*(sint16)xoff)/OBJECT_OFFSET_STEPS);
	file->rdwr_byte(byte);
	xoff = (sint8)(((sint16)byte*OBJECT_OFFSET_STEPS)/16);
	byte = (sint8)(((sint16)16*(sint16)yoff)/OBJECT_OFFSET_STEPS);
	file->rdwr_byte(byte);
	yoff = (sint8)(((sint16)byte*OBJECT_OFFSET_STEPS)/16);
	byte = owner_n;
	file->rdwr_byte(byte);
	owner_n = byte;
}


/**
 * draw the object
 * the dirty-flag is reset from objlist_t::display_obj_fg, or objlist_t::display_overlay when multithreaded
 */
void obj_t::display(int xpos, int ypos  CLIP_NUM_DEF) const
{
	image_id image = get_image();
	image_id const outline_image = get_outline_image();
	if(  image!=IMG_EMPTY  ||  outline_image!=IMG_EMPTY  ) {
		const int raster_width = get_current_tile_raster_width();
		const bool is_dirty = get_flag(obj_t::dirty);

		if (vehicle_base_t const* const v = obj_cast<vehicle_base_t>(this)) {
			// vehicles need finer steps to appear smoother
			v->get_screen_offset( xpos, ypos, raster_width );
		}
		xpos += tile_raster_scale_x(get_xoff(), raster_width);
		ypos += tile_raster_scale_y(get_yoff(), raster_width);

		const int start_ypos = ypos;
		for(  int j=0;  image!=IMG_EMPTY;  ) {

			if(  owner_n != PLAYER_UNOWNED  ) {
				if(  obj_t::show_owner  ) {
					display_blend( image, xpos, ypos, owner_n, color_idx_to_rgb(welt->get_player(owner_n)->get_player_color1()+2) | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty  CLIP_NUM_PAR);
				}
				else {
					display_color( image, xpos, ypos, owner_n, true, is_dirty  CLIP_NUM_PAR);
				}
			}
			else {
				display_normal( image, xpos, ypos, 0, true, is_dirty  CLIP_NUM_PAR);
			}
			// this obj has another image on top (e.g. skyscraper)
			ypos -= raster_width;
			image = get_image(++j);
		}

		if(  outline_image != IMG_EMPTY  ) {
			// transparency?
			const FLAGGED_PIXVAL transparent = get_outline_colour();
			if(  TRANSPARENT_FLAGS&transparent  ) {
				// only transparent outline
				display_blend( get_outline_image(), xpos, start_ypos, owner_n, transparent, 0, is_dirty  CLIP_NUM_PAR);
			}
			else if(  obj_t::get_flag( highlight )  ) {
				// highlight this tile
				display_blend( get_image(), xpos, start_ypos, owner_n, color_idx_to_rgb(COL_RED) | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty  CLIP_NUM_PAR);
			}
		}
		else if(  obj_t::get_flag( highlight )  ) {
			// highlight this tile
			display_blend( get_image(), xpos, start_ypos, owner_n, color_idx_to_rgb(COL_RED) | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty  CLIP_NUM_PAR);
		}
	}
}


// called during map rotation
void obj_t::rotate90()
{
	// most basic: rotate coordinate
	pos.rotate90( welt->get_size().y-1 );
	sint8 new_dx = -2*yoff;
	yoff = xoff/2;
	xoff = new_dx;
}


#ifdef MULTI_THREAD
void obj_t::display_after(int xpos, int ypos, const sint8 clip_num) const
#else
void obj_t::display_after(int xpos, int ypos, bool) const
#endif
{
	image_id image = get_front_image();
	if(  image != IMG_EMPTY  ) {
		const int raster_width = get_current_tile_raster_width();
		const bool is_dirty = get_flag( obj_t::dirty );

		xpos += tile_raster_scale_x( get_xoff(), raster_width );
		ypos += tile_raster_scale_y( get_yoff(), raster_width );

		if(  owner_n != PLAYER_UNOWNED  ) {
			if(  obj_t::show_owner  ) {
				display_blend( image, xpos, ypos, owner_n, color_idx_to_rgb(welt->get_player(owner_n)->get_player_color1()+2) | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty  CLIP_NUM_PAR);
			}
			else if(  obj_t::get_flag( highlight )  ) {
				// highlight this tile
				display_blend( image, xpos, ypos, owner_n, color_idx_to_rgb(COL_RED) | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty  CLIP_NUM_PAR);
			}
			else {
				display_color( image, xpos, ypos, owner_n, true, is_dirty  CLIP_NUM_PAR);
			}
		}
		else if(  obj_t::get_flag( highlight )  ) {
			// highlight this tile
			display_blend( image, xpos, ypos, owner_n, color_idx_to_rgb(COL_RED) | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, is_dirty  CLIP_NUM_PAR);
		}
		else {
			display_normal( image, xpos, ypos, 0, true, is_dirty  CLIP_NUM_PAR);
		}
	}
}


/*
 * when a vehicle moves or a cloud moves, it needs to mark the old spot as dirty (to copy to screen)
 * sometimes they have an extra offset, this is the yoff parameter
 */
void obj_t::mark_image_dirty(image_id image, sint16 yoff) const
{
	if(  image != IMG_EMPTY  ) {
		const sint16 rasterweite = get_tile_raster_width();
		int xpos=0, ypos=0;
		if(  is_moving()  ) {
			vehicle_base_t const* const v = obj_cast<vehicle_base_t>(this);
			// vehicles need finer steps to appear smoother
			v->get_screen_offset( xpos, ypos, get_tile_raster_width() );
		}

		viewport_t *vp = welt->get_viewport();
		scr_coord scr_pos = vp->get_screen_coord(get_pos(), koord(get_xoff(), get_yoff()));
		// xpos, ypos, yoff are already in pixel units, no scaling needed

		// mark the region after the image as dirty
		display_mark_img_dirty( image, scr_pos.x + xpos, scr_pos.y + ypos + yoff);

		// too close to border => set dirty to be sure (smoke, skyscrapers, birds, or the like)
		scr_coord_val xbild = 0, ybild = 0, wbild = 0, hbild = 0;
		display_get_image_offset( image, &xbild, &ybild, &wbild, &hbild );
		const sint16 distance_to_border = 3 - (yoff+get_yoff()+ybild)/(rasterweite/4);
		if(  pos.x <= distance_to_border  ||  pos.y <= distance_to_border  ) {
			// but only if the image is actually visible ...
			if(   scr_pos.x+xbild+wbild >= 0  &&  xpos <= display_get_width()  &&   scr_pos.y+ybild+hbild >= 0  &&  ypos+ybild < display_get_height()  ) {
				welt->set_background_dirty();
			}
		}
	}
}
