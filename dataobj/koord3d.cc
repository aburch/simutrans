/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
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
	xml_tag_t k( file, "koord3d" );
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
const char *koord3d::get_str() const
{
	static char pos_str[32];
	if(x==-1  &&  y==-1  &&  z==-1) {
		return "koord3d invalid";
	}
	sprintf( pos_str, "%i,%i,%i", x, y, z );
	return pos_str;
}


ribi_t::ribi koord3d_vector_t::get_ribi( uint32 index ) const
{
	ribi_t::ribi ribi = ribi_t::keine;
	koord3d pos = operator[](index);
	if( index > 0 ) {
		ribi |= ribi_typ( operator[](index-1).get_2d()-pos.get_2d() );
	}
	if( index+1 < get_count() ) {
		ribi |= ribi_typ( operator[](index+1).get_2d()-pos.get_2d() );
	}
	return ribi;
}

void koord3d_vector_t::rotate90( sint16 y_size )
{
	for( uint32 i = 0; i < get_count(); i++ ) {
		operator[](i).rotate90( y_size );
	}
}
