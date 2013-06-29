#include "gui_komponente.h"
#include "../../dataobj/koord.h"

void gui_komponente_t::align_to( gui_komponente_t* component_par, control_alignment_t alignment_par, koord offset_par ) {

	// Don't process NULL components and complete NONE alignment (both vert and horiz)
	if( component_par && alignment_par != ALIGN_NONE ) {

		koord new_pos     = get_pos();
		koord this_size   = get_groesse();
		koord target_pos  = component_par->get_pos();
		koord target_size = component_par->get_groesse();

		// Do vertical alignment
		switch(alignment_par & ( ALIGN_TOP | ALIGN_CENTER_V | ALIGN_BOTTOM | ALIGN_INTERIOR_V | ALIGN_EXTERIOR_V ) ) {

			// Interior alignment
			case ALIGN_TOP:
				new_pos.y = target_pos.y + offset_par.y;
				break;

			// Interior and Exterior center alignment are the same
			case ALIGN_CENTER_V:
			case ALIGN_EXTERIOR_V|ALIGN_CENTER_V:
				new_pos.y = target_pos.y + offset_par.y + (target_size.y - this_size.y) / 2;
				break;

			case ALIGN_BOTTOM:
				new_pos.y = target_pos.y + offset_par.y + target_size.y - this_size.y;
				break;

			// Exterior alignment
			case ALIGN_EXTERIOR_V|ALIGN_TOP:
				new_pos.y = target_pos.y + target_size.y + offset_par.y;
				break;

			case ALIGN_EXTERIOR_V|ALIGN_BOTTOM:
				new_pos.y = target_pos.y - this_size.y - offset_par.y;
				break;

			// if one of the alignments is NONE but offseted,
			// use offset as absolut position.
			case ALIGN_NONE:
				if (offset_par.y != 0) {
					new_pos.y = offset_par.y;
				}
				break;
		}

		// Do horizontal alignment
		switch(alignment_par & ( ALIGN_LEFT | ALIGN_CENTER_H | ALIGN_RIGHT | ALIGN_INTERIOR_H | ALIGN_EXTERIOR_H ) ) {

			// Interior alignment
			case ALIGN_LEFT:
				new_pos.x = target_pos.x + offset_par.x;
				break;

			// Interior and Exterior center alignment are the same
			case ALIGN_CENTER_H:
			case ALIGN_EXTERIOR_H|ALIGN_CENTER_H:
				new_pos.x = target_pos.x + offset_par.x + (target_size.x - this_size.x) / 2;
				break;

			case ALIGN_RIGHT:
				new_pos.x = target_pos.x + target_size.x - this_size.x - offset_par.x;
				break;

			// Exterior alignment
			case ALIGN_EXTERIOR_H|ALIGN_LEFT:
				new_pos.x = target_pos.x + target_size.x + offset_par.x;
				break;

			case ALIGN_EXTERIOR_H|ALIGN_RIGHT:
				new_pos.x = target_pos.x - this_size.x - offset_par.x;
				break;

			// if one of the alignments is NONE but offseted,
			// use offset as absolut position.
			case ALIGN_NONE:
				if (offset_par.x) {
					new_pos.x = offset_par.x;
				}
				break;

		}

		set_pos(new_pos);
	}

}
