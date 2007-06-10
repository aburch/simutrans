/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdlib.h>
#include <stdio.h>

#include "../simtypes.h"
#include "koord3d.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/freelist.h"


const koord3d koord3d::invalid(-1, -1, -1);


void * koord3d::operator new(size_t /*s*/)
{
	return freelist_t::gimme_node(sizeof(koord3d));
}


void koord3d::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(koord3d),p);
}



void
koord3d::rdwr(loadsave_t *file)
{
	sint16 v16;

	v16 = x;
	file->rdwr_short(v16, " ");
	x = v16;

	v16 = y;
	file->rdwr_short(v16, " ");
	y = v16;

	if(file->get_version()<99005) {
		file->rdwr_short(v16, "\n");
		if(v16!=-1) {
			z = (v16/16);
		}
		else {
			// keep invalid position
			z = -1;
		}
	}
	else {
		sint8 v8=z;
		file->rdwr_byte(v8, "\n");
		z = v8;
	}
}

koord3d::koord3d(loadsave_t *file)
{
	rdwr(file);
}
