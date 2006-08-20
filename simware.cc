/*
 * simware.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>
#include <string.h>

#include "simio.h"
#include "simmem.h"
#include "simdebug.h"
#include "simtypes.h"
#include "simware.h"

#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "dataobj/koord.h"

#include "utils/cstring_t.h"

#include "besch/ware_besch.h"
#include "bauer/warenbauer.h"


#ifdef _MSC_VER
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif

/**
 * gibt den nicht-uebersetzten warennamen zurück
 * @author Hj. Malthaner
 */
const char *ware_t::gib_name() const
{
    return type->gib_name();
}


int ware_t::gib_preis()  const
{
    return type->gib_preis();
}


int ware_t::gib_catg()  const
{
    return type->gib_catg();
}


const char *ware_t::gib_mass() const
{
    return type->gib_mass();
}

ware_t::ware_t() : ziel(-1, -1), zwischenziel(-1, -1), zielpos(-1, -1)
{
    menge = 0;
    max = 0;
    type = 0;
}


ware_t::ware_t(const ware_besch_t *wtyp) : ziel(-1, -1), zwischenziel(-1, -1), zielpos(-1, -1)
{
    menge = 0;
    max = 0;
    type = wtyp;
}

ware_t::ware_t(loadsave_t *file)
{
    rdwr(file);
}


ware_t::~ware_t()
{
    menge = 0;
    max = 0;

    type = 0;
}


void
ware_t::rdwr(loadsave_t *file)
{
    file->rdwr_int(menge, " ");
    file->rdwr_int(max, " ");

    const char *typ = NULL;

    if(file->is_saving()) {
	typ = type->gib_name();
    }
    file->rdwr_str(typ, " ");
    if(file->is_loading()) {
	type = warenbauer_t::gib_info(typ);
	guarded_free(const_cast<char *>(typ));
    }
    ziel.rdwr(file);
    zwischenziel.rdwr(file);
    zielpos.rdwr(file);
}
