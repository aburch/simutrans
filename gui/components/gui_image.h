/*
 * just displays an image
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_image_h
#define gui_image_h

#include "../../simimg.h"
#include "../../simgraph.h"
#include "gui_komponente.h"


class gui_image_t : public gui_komponente_t
{
private:
	image_id id;
	uint16 player_nr;
	bool  remove_enabled;
	koord remove_offset;

public:
	gui_image_t( const image_id i=IMG_LEER, const uint8 p=0 ) : player_nr(p) {
		remove_enabled = false;
		remove_offset  = koord(0,0);
		set_image(i);
	}

	void set_image( const image_id i, bool remove_offsets = false ) {
		id = i;
		if(  id!=IMG_LEER  ) {
			KOORD_VAL x,y,w,h;
			remove_enabled = remove_offsets;
			display_get_base_image_offset( id, &x, &y, &w, &h );
			if (remove_enabled) {
				remove_offset = koord(-x,-y);
			}
			set_groesse( koord( x+w+remove_offset.x, y+h+remove_offset.y ) );
		}
		else {
			set_groesse( koord(0,0) );
			remove_offset = koord(0,0);
			remove_enabled = false;
		}
	}

	void enable_offset_removal(bool remove_offsets) { set_image(id,remove_offsets); }

	/**
	 * Zeichnet die Komponente
	 * @author Hj. Malthaner
	 */
	void zeichnen( koord offset ) { display_base_img( id, pos.x+offset.x+remove_offset.x, pos.y+offset.y+remove_offset.y, (sint8)player_nr, false, true ); }
};

#endif
