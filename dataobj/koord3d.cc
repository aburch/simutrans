/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "koord3d.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/umgebung.h"


const koord3d koord3d::invalid(-1, -1, -1);


void koord3d::rotate90( sint16 y_diff )
{
	sint16 new_x = y_diff-y;
	y = x;
	x = new_x;
}


void koord3d::rdwr(loadsave_t *file)
{
	xml_tag_t k( file, "koord3d" );
	sint16 v16;

	v16 = x;
	file->rdwr_short(v16);
	x = v16;

	v16 = y;
	file->rdwr_short(v16);
	y = v16;

	if(file->get_version()<99005) {
		file->rdwr_short(v16);
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
		file->rdwr_byte(v8);
		z = v8;
	}

	if(  file->is_loading()  &&  file->get_version() < 112007  &&  x != -1  &&  y != -1  ) {
		// convert heights from old single height saved game
		z *= umgebung_t::pak_height_conversion_factor;
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


// für debugmeldungen ...
const char *koord3d::get_fullstr() const
{
	static char pos_str[32];
	if(x==-1  &&  y==-1  &&  z==-1) {
		return "koord3d invalid";
	}
	sprintf( pos_str, "(%i,%i,%i)", x, y, z );
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

ribi_t::ribi koord3d_vector_t::get_short_ribi( uint32 index ) const
{
	ribi_t::ribi ribi = ribi_t::keine;
	const koord pos = operator[](index).get_2d();
	if( index > 0 ) {
		const koord pos2 = operator[](index-1).get_2d();
		if (koord_distance(pos,pos2)<=1) {
			ribi |= ribi_typ( pos2-pos );
		}
	}
	if( index+1 < get_count() ) {
		const koord pos2 = operator[](index+1).get_2d();
		if (koord_distance(pos,pos2)<=1) {
			ribi |= ribi_typ( pos2-pos );
		}
	}
	return ribi;
}

void koord3d_vector_t::rotate90( sint16 y_size )
{
	FOR(koord3d_vector_t, & i, *this) {
		i.rotate90(y_size);
	}
}
