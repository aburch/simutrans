/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Basisklasse aller Dinge
 *
 * Hj. Maltahner
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "simdebug.h"
#include "simimg.h"
#include "simcolor.h"
#include "simgraph.h"
#include "simwin.h"
#include "player/simplay.h"
#include "simdings.h"
#include "simworld.h"
#include "dings/baum.h"
#include "dings/gebaeude.h"
#include "vehicle/simvehikel.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "boden/grund.h"
#include "gui/thing_info.h"
#include "utils/cbuffer_t.h"
#include "utils/simstring.h"



/**
 * Pointer to the world of this thing. Static to conserve space.
 * Change to instance variable once more than one world is available.
 * @author Hj. Malthaner
 */
karte_t * ding_t::welt = NULL;


void ding_t::init(karte_t *wl)
{
	welt = wl;
	pos = koord3d::invalid;    // nicht in der karte enthalten!

	xoff = 0;
	yoff = 0;

	besitzer_n = PLAYER_UNOWNED;

	flags = keine_flags;
	set_flag(dirty);
}


ding_t::ding_t(karte_t *wl)
{
	init(wl);
}


ding_t::ding_t(karte_t *wl, loadsave_t *file)
{
	init(wl);
	rdwr(file);
}


ding_t::ding_t(karte_t *wl, koord3d pos)
{
	init(wl);
	this->pos = pos;
}


// removes an object and tries to delete it also form the corresponding dinglist
ding_t::~ding_t()
{
	destroy_win((long)this);

	if(flags&not_on_map  ||  !welt->ist_in_kartengrenzen(pos.get_2d())) {
//		DBG_MESSAGE("ding_t::~ding_t()","deleted %p not on the map",this);
		return;
	}

	// pruefe ob objekt auf karte und ggf. entfernen
	grund_t *gr = welt->lookup(pos);
	if(!gr  ||  !gr->obj_remove(this)) {
		// not found? => try harder at all map locations
		dbg->warning("ding_t::~ding_t()","couldn't remove %p from %d,%d,%d",this, pos.x , pos.y, pos.z);

		// first: try different height ...
		gr = welt->access(pos.get_2d())->get_boden_von_obj(this);
		if(gr  &&  gr->obj_remove(this)) {
			dbg->warning("ding_t::~ding_t()",
				"removed %p from %d,%d,%d, but it should have been on %d,%d,%d",
				this,
				gr->get_pos().x, gr->get_pos().y, gr->get_pos().z,
				pos.x, pos.y, pos.z);
			return;
		}

		// then search entire map
		koord k;
		for(k.y=0; k.y<welt->get_groesse_y(); k.y++) {
			for(k.x=0; k.x<welt->get_groesse_x(); k.x++) {
				grund_t *gr = welt->access(k)->get_boden_von_obj(this);
				if (gr && gr->obj_remove(this)) {
					dbg->warning("ding_t::~ding_t()",
						"removed %p from %d,%d,%d, but it should have been on %d,%d,%d",
						this,
						gr->get_pos().x, gr->get_pos().y, gr->get_pos().z,
						pos.x, pos.y, pos.z);
					return;
				}
			}
		}
	}
//DBG_MESSAGE("ding_t::~ding_t()","finished");
}




/**
 * setzt den Besitzer des dings
 * (public wegen Rathausumbau - V.Meyer)
 * "sets the owner of the thing" (Babelfish)
 * @author Hj. Malthaner
 */
void ding_t::set_besitzer(spieler_t *sp)
{
	int i = welt->sp2num(sp);
	assert(i>=0);
	besitzer_n = (uint8)i;
}



/**
 * Ein Objekt kann einen Besitzer haben.
 * @return Einen Zeiger auf den Besitzer des Objekts oder NULL,
 * wenn das Objekt niemand gehört.
 * @author Hj. Malthaner
 */
spieler_t *ding_t::get_besitzer() const
{
	return welt->get_spieler(besitzer_n);
}



/* the only general info we can give is the name
 * we want to format it nicely,
 * with two linebreaks at the end => thus the little extra effort
 */
void ding_t::info(cbuffer_t & buf) const
{
	char              translation[256];
	char const* const owner =
		besitzer_n == 1              ? translator::translate("Eigenbesitz\n")   :
		besitzer_n == PLAYER_UNOWNED ? translator::translate("Kein Besitzer\n") :
		get_besitzer()->get_name();
	tstrncpy(translation, owner, lengthof(translation));
	// remove trailing linebreaks etc.
	for(  int i=strlen(translation);  i>0  &&  ' '>(uint8)translation[i-1];  i--  ) {
		translation[i-1] = 0;
	}
	buf.append( translation );
	// only append linebreaks if not empty
	if(  buf.len()>0  ) {
		buf.append( "\n\n" );
	}
}


void ding_t::zeige_info()
{
	create_win( new ding_infowin_t(this), w_info, (long)this);
}



// returns NULL, if removal is allowed
const char *ding_t::ist_entfernbar(const spieler_t *sp)
{
	if(besitzer_n==PLAYER_UNOWNED  ||  welt->get_spieler(besitzer_n) == sp  ||  welt->get_spieler(1) == sp) {
		return NULL;
	}
	else {
		return "Der Besitzer erlaubt das Entfernen nicht";
	}
}



void
ding_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "ding_t" );
	if(  file->get_version()<101000) {
		pos.rdwr( file );
	}

	sint8 byte = (sint8)(((sint16)16*(sint16)xoff)/TILE_STEPS);
	file->rdwr_byte(byte, " ");
	xoff = (sint8)(((sint16)byte*TILE_STEPS)/16);
	byte = (sint8)(((sint16)16*(sint16)yoff)/TILE_STEPS);
	file->rdwr_byte(byte, "\n");
	yoff = (sint8)(((sint16)byte*TILE_STEPS)/16);
	byte = besitzer_n;
	file->rdwr_byte(byte, "\n");
	besitzer_n = byte;
}



/**
 * Ding zeichnen
 * (reset dirty will be done from dingliste! It is true only for drawing the main window.)
 * @author Hj. Malthaner
 */
void
ding_t::display(int xpos, int ypos, bool /*reset_dirty*/) const
{
	const int raster_width = get_current_tile_raster_width();

	if(is_moving()) {
		// vehicles needs finer steps to appear smoother
		const vehikel_basis_t* const v = (const vehikel_basis_t*)this;
		v->get_screen_offset( xpos, ypos, raster_width );
	}
	xpos += tile_raster_scale_x(get_xoff(), raster_width);
	ypos += tile_raster_scale_y(get_yoff(), raster_width);

	const int start_ypos = ypos;

	bool dirty = get_flag(ding_t::dirty);
	int j = 0;
	image_id bild = get_bild();

	while(bild!=IMG_LEER) {

		if(besitzer_n!=PLAYER_UNOWNED) {
			display_color(bild, xpos, ypos, besitzer_n, true, dirty);
		}
		else {
			display_normal(bild, xpos, ypos, 0, true, dirty);
		}
		// this ding has another image on top (e.g. skyscraper)
		ypos -= raster_width;
		bild = get_bild(++j);
	}

	// transparentcy?
	const PLAYER_COLOR_VAL transparent = get_outline_colour();
	if(TRANSPARENT_FLAGS&transparent) {
		// only transparent outline
		display_blend(get_outline_bild(), xpos, start_ypos, besitzer_n, transparent, 0, dirty);
	}
}



// called during map rotation
void ding_t::rotate90()
{
	// most basic: rotate coordinate
	pos.rotate90( welt->get_groesse_y()-1 );
	if(xoff!=0) {
		sint8 new_dx = -2*yoff;
		yoff = xoff/2;
		xoff = new_dx;
	}
}



void
ding_t::display_after(int xpos, int ypos, bool /*is_global*/ ) const
{
	image_id bild = get_after_bild();
	if(bild != IMG_LEER) {
		const int raster_width = get_current_tile_raster_width();
		const bool dirty = get_flag(ding_t::dirty);

		xpos += tile_raster_scale_x(get_xoff(), raster_width);
		ypos += tile_raster_scale_y(get_yoff(), raster_width);

		if(besitzer_n!=PLAYER_UNOWNED) {
			display_color(bild, xpos, ypos, besitzer_n, true, dirty );
		}
		else {
			display_normal(bild, xpos, ypos, 0, true, dirty );
		}
	}
}



/*
 * when a vehicle moves or a cloud moves, it needs to mark the old spot as dirty (to copy to screen)
 * sometimes they have an extra offset, this the yoff parameter
* @author prissi
 */
void
ding_t::mark_image_dirty(image_id bild,sint8 yoff) const
{
	if(bild!=IMG_LEER) {
		int xpos=0, ypos=0;
		if(is_moving()) {
			// vehicles needs finer steps to appear smoother
			const vehikel_basis_t* const v = (const vehikel_basis_t*)this;
			v->get_screen_offset( xpos, ypos, get_tile_raster_width() );
		}
		// better not try to twist your brain to follow the retransformation ...
		const sint16 rasterweite=get_tile_raster_width();
		const koord diff = get_pos().get_2d()-welt->get_world_position()-welt->get_ansicht_ij_offset();
		const sint16 x = (diff.x-diff.y)*(rasterweite/2) + tile_raster_scale_x(get_xoff(), rasterweite) + xpos;
		const sint16 y = (diff.x+diff.y)*(rasterweite/4) + tile_raster_scale_y( yoff+get_yoff()-get_pos().z*TILE_HEIGHT_STEP/Z_TILE_STEP, rasterweite) + ((display_get_width()/rasterweite)&1)*(rasterweite/4) + ypos;
		// mark the region after the image as dirty
		display_mark_img_dirty( bild, x+welt->get_x_off(), y+welt->get_y_off() );
	}
}
