/*
 * raucher.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simtools.h"
#include "../simmem.h"
#include "../simskin.h"
#include "../simintr.h"
#include "../besch/text_besch.h"
#include "../besch/fabrik_besch.h"
#include "raucher.h"
#include "wolke.h"

#include "../dataobj/loadsave.h"
#include "../boden/grund.h"

stringhashtable_tpl<const rauch_besch_t *> raucher_t::besch_table;

void raucher_t::register_besch(const rauch_besch_t *besch, const char*name)
{
    besch_table.put(name, besch);
}

const rauch_besch_t *raucher_t::gib_besch(const char *name)
{
    return besch_table.get(name);
}

raucher_t::raucher_t(karte_t *welt, loadsave_t *file) : ding_t (welt)
{
    rdwr(file);
}


raucher_t::raucher_t(karte_t *welt, koord3d pos, const rauch_besch_t *besch) :
    ding_t(welt, pos)
{
    this->besch = besch;
    setze_yoff( besch->gib_xy_off().y);
    setze_xoff( besch->gib_xy_off().x);
}

bool
raucher_t::step(long /*delta_t*/)
{
	if(simrand(besch->gib_zeitmaske()) < 16) {
		welt->lookup(gib_pos())->obj_add(new async_wolke_t(welt, gib_pos(), gib_xoff()+simrand(7)-3, gib_yoff(), besch->gib_bilder()->gib_bild_nr((0))));
		INT_CHECK("raucher 57");
	}
	return true;
}

void
raucher_t::zeige_info()
{
    // raucher sollten keine Infofenster erzeugen
}

void
raucher_t::rdwr(loadsave_t *file)
{
    ding_t::rdwr( file );
    const char *s = NULL;

    if(file->is_saving()) {
	s = besch->gib_name();
    }
    file->rdwr_str(s, "N");
    if(file->is_loading()) {
	besch = gib_besch(s);
	guarded_free(const_cast<char *>(s));
    }
}
