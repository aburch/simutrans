/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "koord3d.h"
#include "../dataobj/loadsave.h"


const koord3d koord3d::invalid(-1, -1, -1);



void
koord3d::rotate90( sint16 y_diff )
{
	sint16 new_x = y_diff-y;
	y = x;
	x = new_x;
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


// für debugmeldungen ...
const char *koord3d::gib_str() const
{
	static char pos_str[32];
	if(x==-1  &&  y==-1  &&  z==-1) {
		return "koord3d invalid";
	}
	sprintf( pos_str, "%i,%i,%i", x, y, z );
	return pos_str;
}
