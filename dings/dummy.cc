/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simimg.h"

#include "../besch/grund_besch.h"

#include "../dataobj/freelist.h"

#include "dummy.h"

/**
 * prissi: a dummy typ for old things, which are now ignored
 */

dummy_ding_t::dummy_ding_t(karte_t *welt, loadsave_t *file) :
    ding_t(welt)
{
	rdwr(file);
	// do not remove from this position, since there will be nothing
	ding_t::set_flag(ding_t::not_on_map);
}

dummy_ding_t::dummy_ding_t(karte_t *welt, koord3d pos, spieler_t *) :
    ding_t(welt, pos)
{
}


void * dummy_ding_t::operator new(size_t /*s*/)
{
	return freelist_t::gimme_node(sizeof(dummy_ding_t));
}


void dummy_ding_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(dummy_ding_t),p);
}
