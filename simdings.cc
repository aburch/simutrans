/*
 * simdings.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * simdings.cc
 *
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
#include "simplay.h"
#include "simdings.h"
#include "simworld.h"
#include "dings/baum.h"
#include "dings/gebaeude.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "boden/grund.h"
#include "gui/thing_info.h"
#include "utils/cbuffer_t.h"
#include "tpl/ptrhashtable_tpl.h"



/**
 * Pointer to the world of this thing. Static to conserve space.
 * Change to instance variable once more than one world is available.
 * @author Hj. Malthaner
 */
karte_t * ding_t::welt = NULL;


static ptrhashtable_tpl<ding_t*, ding_infowin_t*> ding_infos;


void ding_t::entferne_ding_info() {
	ding_infos.remove(this);
}


void ding_t::init(karte_t *wl)
{
	welt = wl;
	pos = koord3d::invalid;    // nicht in der karte enthalten!

	xoff = 0;
	yoff = 0;

	besitzer_n = -1;

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
	destroy_win(ding_infos.get(this));

	if(flags&not_on_map  ||  !welt->ist_in_kartengrenzen(pos.gib_2d())) {
//		DBG_MESSAGE("ding_t::~ding_t()","deleted %p not on the map",this);
		return;
	}

	// pruefe ob objekt auf karte und ggf. entfernen
	grund_t *gr = welt->lookup(pos);
	if(!gr  ||  !gr->obj_remove(this)) {
		// not found? => try harder at all map locations
		dbg->warning("ding_t::~ding_t()","couldn't remove %p from %d,%d,%d",this, pos.x , pos.y, pos.z);

		// first: try different height ...
		gr = welt->access(pos.gib_2d())->gib_boden_von_obj(this);
		if(gr  &&  gr->obj_remove(this)) {
			dbg->warning("ding_t::~ding_t()",
				"removed %p from %d,%d,%d, but it should have been on %d,%d,%d",
				this,
				gr->gib_pos().x, gr->gib_pos().y, gr->gib_pos().z,
				pos.x, pos.y, pos.z);
			return;
		}

		// then search entire map
		koord k;
		for(k.y=0; k.y<welt->gib_groesse_y(); k.y++) {
			for(k.x=0; k.x<welt->gib_groesse_x(); k.x++) {
				grund_t *gr = welt->access(k)->gib_boden_von_obj(this);
				if (gr && gr->obj_remove(this)) {
					dbg->warning("ding_t::~ding_t()",
						"removed %p from %d,%d,%d, but it should have been on %d,%d,%d",
						this,
						gr->gib_pos().x, gr->gib_pos().y, gr->gib_pos().z,
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
 * @author Hj. Malthaner
 */
void ding_t::setze_besitzer(spieler_t *sp)
{
	besitzer_n = welt->sp2num(sp);
}



/**
 * Ein Objekt kann einen Besitzer haben.
 * @return Einen Zeiger auf den Besitzer des Objekts oder NULL,
 * wenn das Objekt niemand gehört.
 * @author Hj. Malthaner
 */
spieler_t * ding_t::gib_besitzer() const {
	return besitzer_n == -1 ? 0 : welt->gib_spieler(besitzer_n);
}



void
ding_t::info(cbuffer_t & buf) const
{
	if(besitzer_n==1) {
		buf.append(translator::translate("Eigenbesitz\n"));
	} else if(besitzer_n==0 || besitzer_n > 1) {
		buf.append(translator::translate("Spieler"));
		buf.append(" ");
		buf.append(besitzer_n);
		buf.append("\n");
	}
//	else {
//		buf.append(translator::translate("Kein Besitzer\n"));
//	}
}



ding_infowin_t *ding_t::new_info()
{
	return new ding_infowin_t(welt, this);
}



void
ding_t::zeige_info()
{
	ding_infowin_t* info = ding_infos.get(this);
	if (info == NULL) {
		info = new_info();
		ding_infos.put(this, info);
	}
	create_win(-1, -1, info, w_autodelete);
}



const char *
ding_t::ist_entfernbar(const spieler_t *sp)
{
	if(besitzer_n<0 || gib_besitzer() == sp) {
		return NULL;
	} else {
		return "Der Besitzer erlaubt das Entfernen nicht";
	}
}



void
ding_t::rdwr(loadsave_t *file)
{
	file->wr_obj_id(gib_typ());
	pos.rdwr( file );

	sint8 byte=xoff;
	file->rdwr_byte(byte, " ");
	xoff = byte;
	byte=yoff;
	file->rdwr_byte(byte, "\n");
	yoff = byte;
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
	const int raster_width = get_tile_raster_width();

	image_id bild = gib_bild();

	xpos += tile_raster_scale_x(gib_xoff(), raster_width);
	ypos += tile_raster_scale_y(gib_yoff(), raster_width);
	const int start_ypos = ypos;

	bool dirty = get_flag(ding_t::dirty);
	int j = 0;
	while(bild!=IMG_LEER) {

		if(besitzer_n!=-1) {
			display_color_img(bild, xpos, ypos, besitzer_n, true, dirty);
		}	else {
			display_img(bild, xpos, ypos, dirty);
		}
		// this ding has another image on top (e.g. skyscraper)
		ypos -= raster_width;
		bild = gib_bild(++j);
	}

	// transparentcy?
	const PLAYER_COLOR_VAL transparent = gib_outline_colour();
	if(TRANSPARENT_FLAGS&transparent) {
		// only transparent outline
		display_img_blend(gib_outline_bild(), xpos, start_ypos, transparent, 0, dirty);
	}
}



void
ding_t::display_after(int xpos, int ypos, bool is_global) const
{
	image_id bild = gib_after_bild();
	if(bild != IMG_LEER) {
		const int raster_width = get_tile_raster_width();

		xpos += tile_raster_scale_x(gib_xoff(), raster_width);
		ypos += tile_raster_scale_y(gib_yoff(), raster_width);

		// unfourtunately the dirty flag is already cleared, when we reach here ...
		if(besitzer_n!=-1) {
			display_color_img(bild, xpos, ypos, besitzer_n, true, false );
		}
		else {
			display_img(bild, xpos, ypos, false );
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
		// better not try to twist your brain to follow the retransformation ...
		const sint16 rasterweite=get_tile_raster_width();
		const koord diff = gib_pos().gib_2d()-welt->gib_ij_off()-welt->gib_ansicht_ij_offset();
		const sint16 x = (diff.x-diff.y)*(rasterweite/2) + tile_raster_scale_x(gib_xoff(), rasterweite);
		const sint16 y = (diff.x+diff.y)*(rasterweite/4) + tile_raster_scale_y( yoff+gib_yoff()-gib_pos().z*TILE_HEIGHT_STEP/Z_TILE_STEP, rasterweite) + ((display_get_width()/rasterweite)&1)*(rasterweite/4);
		// mark the region after the image as dirty
		display_mark_img_dirty( bild, x+welt->gib_x_off(), y+welt->gib_y_off() );
	}
}
