#include "gui_image.h"
#include "../gui_frame.h"

gui_image_t::gui_image_t( const image_id i, const uint8 p, control_alignment_t alignment_par, bool remove_offset_enabled )
: player_nr(p)
{
	alignment = alignment_par;
	size_mode = size_mode_auto;
	remove_enabled = remove_offset_enabled;
	remove_offset  = koord(0,0);
	set_image(i,remove_offset_enabled);
}



void gui_image_t::set_groesse( koord size_par )
{
	if( id  !=  IMG_LEER ) {

		scr_coord_val x,y,w,h;
		display_get_base_image_offset( id, &x, &y, &w, &h );

		if( remove_enabled ) {
			remove_offset = koord(-x,-y);
		}

		if( size_mode  ==  size_mode_auto ) {
			size_par = koord( x+w+remove_offset.x, y+h+remove_offset.y );
		}
	}

	gui_komponente_t::set_groesse(size_par);
}



void gui_image_t::set_image( const image_id i, bool remove_offsets ) {

	id = i;
	remove_enabled = remove_offsets;

	if(  id ==IMG_LEER  ) {
		remove_offset = koord(0,0);
		remove_enabled = false;
	}

	set_groesse( groesse );
}



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_image_t::zeichnen( koord offset ) {

	if(  id!=IMG_LEER  ) {
		scr_coord_val x,y,w,h;
		display_get_base_image_offset( id, &x, &y, &w, &h );

		switch (alignment) {

			case ALIGN_RIGHT:
				offset.x += groesse.x - w + remove_offset.x;
				break;

			case ALIGN_BOTTOM:
				offset.y += groesse.y - h + remove_offset.y;
				break;

			case ALIGN_CENTER_H:
				offset.x += D_GET_CENTER_ALIGN_OFFSET(w,groesse.x);
				break;

			case ALIGN_CENTER_V:
				offset.y += D_GET_CENTER_ALIGN_OFFSET(h,groesse.y);
				break;

		}

		display_base_img( id, pos.x+offset.x+remove_offset.x, pos.y+offset.y+remove_offset.y, (sint8)player_nr, false, true );
	}
}
