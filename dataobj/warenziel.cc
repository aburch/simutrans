/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "warenziel.h"
#include "koord.h"
#include "loadsave.h"
#include "../macros.h"

void warenziel_t::rdwr(loadsave_t *file)
{
	// dummy ...
	koord ziel;
	ziel.rdwr(file);

	char tn[256];
	file->rdwr_str(tn, lengthof(tn));
}
