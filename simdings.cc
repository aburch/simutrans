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
ptrhashtable_tpl<ding_t *, ding_info_t *> * ding_t::ding_infos = new ptrhashtable_tpl<ding_t *, ding_info_t *> ();


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


void ding_t::entferne_ding_info() {
    ding_infos->remove(this);
}


void ding_t::init(karte_t *wl)
{
    welt = wl;
    pos = koord3d::invalid;    // nicht in der karte enthalten!

    xoff = 0;
    yoff = 0;

    besitzer_n = -1;

    bild = (unsigned short)IMG_LEER;

    step_frequency = 1;

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
	destroy_win(ding_infos->get(this));

	// pruefe ob objekt auf karte und ggf. entfernen
	grund_t *gr = welt->lookup(pos);
	if(gr) {

		if(gr->obj_ist_da(this) ) {
			// normal case
			gr->obj_remove(this, gib_besitzer());
			welt->markiere_dirty(pos);
		}
		else {
			// not found? => try harder at all map locations
			dbg->warning("ding_t::~ding_t()","couldn't remove %p from %d,%d,%d",this , pos.x , pos.y, pos.z);
			DBG_MESSAGE("ding_t::~ding_t()","removing %p failed, checking all plan squares",this);

			koord k;
			for(k.y=0; k.y<welt->gib_groesse_y(); k.y++) {
				for(k.x=0; k.x<welt->gib_groesse_x(); k.x++) {
					grund_t *gr = welt->access( k )->gib_boden_von_obj(this);

					if(gr && gr->obj_remove(this, gib_besitzer())) {
						dbg->warning("ding_t::~ding_t()",
							"removed %p from %d,%d,%d, but it should have been on %d,%d,%d",
							this,
							gr->gib_pos().x, gr->gib_pos().y, gr->gib_pos().z,
							pos.x, pos.y, pos.z);
					}
				}
			}
		}
	}
//DBG_MESSAGE("ding_t::~ding_t()","finished");
}


void ding_t::info(cbuffer_t & buf) const
{
  if(besitzer_n == 1) {
    if(fabrik() != NULL) {
      buf.append(translator::translate("Privatbesitz\n"));
    } else {
      buf.append(translator::translate("Eigenbesitz\n"));
    }
  } else if(besitzer_n == 0 || besitzer_n > 1) {
    buf.append(translator::translate("Spieler"));
    buf.append(" ");
    buf.append(besitzer_n);
    buf.append("\n");
  }
}


void
ding_t::setze_bild(int n, int bild)
{
    assert(n==0);

    if(this->bild != bild) {
	this->bild = bild;
	set_flag(dirty);
    }
}


void
ding_t::setze_xoff(int xoff)
{
    if(this->xoff != xoff) {
	this->xoff = xoff;
	set_flag(dirty);
    }
}


void
ding_t::setze_yoff(int yoff)
{
    if(this->yoff != yoff) {
	this->yoff = yoff;
	set_flag(dirty);
    }
}


void
ding_t::setze_pos(koord3d k)
{
    if(k != pos) {
	if(welt->lookup(pos.gib_2d()) &&
           welt->lookup(pos.gib_2d())->gib_kartenboden()) {
	    // dirty spots are checked while painting grounds.
	    // thus we need to mark the ground pos, rather than our pos.

	    // welt->markiere_dirty(welt->lookup(pos.gib_2d())->gib_boden()->gib_pos());
	    // welt->markiere_dirty(k);
	}

	pos = k;

	set_flag(dirty);
    }
}

ding_info_t *ding_t::new_info()
{
    return new ding_infowin_t(welt, this);
}

void
ding_t::zeige_info()
{
    if(!ding_infos->get(this)) {
	ding_infos->put(this, new_info());
    }
    create_win(-1, -1, ding_infos->get(this), w_autodelete);
}
/*
void
ding_t::zeige_info(ding_t *dt)
{
    if(!ding_infos->get(dt)) {
	ding_infos->put(dt, new_info());
    }
    create_win(-1, -1, ding_infos->get(dt), w_autodelete);
}
*/
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

    file->rdwr_byte(xoff, " ");
    file->rdwr_byte(yoff, "\n");
    uint8 owner = besitzer_n;
    file->rdwr_byte(owner, "\n");
    besitzer_n = owner;

    if(file->is_loading()) {
	bild = static_cast<uint16>(IMG_LEER);
    }
}


/**
 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void ding_t::laden_abschliessen()
{
  // Nothing to do
}


/**
 * Ding zeichnen
 * @author Hj. Malthaner
 */
void
ding_t::display(int xpos, int ypos, bool dirty) const
{
	const int raster_width = get_tile_raster_width();

	int bild = gib_bild();

	ypos += tile_raster_scale_x(gib_yoff(), raster_width);
	xpos += tile_raster_scale_x(gib_xoff(), raster_width);

	dirty |= get_flag(ding_t::dirty);

	if(dirty && bild == 0xFFFF) {
		mark_rect_dirty_wc(xpos-8, ypos-32, xpos+80, ypos+76);
	}

	int j = 1;
	while(bild!=-1) {

		if(gib_besitzer()) {
			display_color_img(bild, xpos, ypos, gib_besitzer()->kennfarbe, true, dirty);
		}
		else {
			display_img(bild, xpos, ypos, dirty);
		}
		// this ding has another image on top (e.g. skyscraper)
		ypos -= raster_width;
		bild = gib_bild(j++);
	}
}


void
ding_t::display_after(int xpos, int ypos, bool dirty) const
{
	int bild = gib_after_bild();

	if(bild != -1) {
		const int raster_width = get_tile_raster_width();

		ypos += tile_raster_scale_x(gib_yoff(), raster_width);
		xpos += tile_raster_scale_x(gib_xoff(), raster_width);

		dirty |= get_flag(ding_t::dirty);

		if(gib_besitzer()) {
			display_color_img(bild, xpos, ypos, gib_besitzer()->kennfarbe, true, dirty);
		}
		else {
			display_img(bild, xpos, ypos, dirty);
		}
	}
}
