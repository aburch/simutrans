/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_component.h"
#include "../../display/scr_coord.h"

#include "../../world/simworld.h"

karte_ptr_t gui_world_component_t::welt;

void gui_component_t::align_to( gui_component_t* component_par, control_alignment_t alignment_par, scr_coord offset_par )
{
	// Don't process NULL components and complete NONE alignment (both vert and horiz)
	if(  component_par  &&  alignment_par != ALIGN_NONE  ) {

		scr_coord new_pos    = get_pos();
		scr_size new_size    = get_size();
		scr_coord target_pos = component_par->get_pos();
		scr_size target_size = component_par->get_size();

		// Do vertical streching
		if(  alignment_par & ALIGN_STRETCH_V  ) {
			new_size.h = target_size.h;
		}

		// Do horizontal streching
		if(  alignment_par & ALIGN_STRETCH_H  ) {
			new_size.w = target_size.w;
		}

		// Do vertical alignment
		switch(alignment_par & ( ALIGN_TOP | ALIGN_CENTER_V | ALIGN_BOTTOM | ALIGN_INTERIOR_V | ALIGN_EXTERIOR_V ) ) {

			// Interior alignment
			case ALIGN_TOP:
				new_pos.y = target_pos.y + offset_par.y;
				break;

			// Interior and Exterior center alignment are the same
			case ALIGN_CENTER_V:
			case ALIGN_EXTERIOR_V|ALIGN_CENTER_V:
				new_pos.y = target_pos.y + offset_par.y + (target_size.h - new_size.h) / 2;
				break;

			case ALIGN_BOTTOM:
				new_pos.y = target_pos.y + offset_par.y + target_size.h - new_size.h;
				break;

			// Exterior alignment
			case ALIGN_EXTERIOR_V|ALIGN_TOP:
				new_pos.y = target_pos.y + target_size.h + offset_par.y;
				break;

			case ALIGN_EXTERIOR_V|ALIGN_BOTTOM:
				new_pos.y = target_pos.y - new_size.h - offset_par.y;
				break;

			// if one of the alignments is NONE but offset,
			// use offset as absolute position.
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
				new_pos.x = target_pos.x + offset_par.x + (target_size.w - new_size.w) / 2;
				break;

			case ALIGN_RIGHT:
				new_pos.x = target_pos.x + target_size.w - new_size.w - offset_par.x;
				break;

			// Exterior alignment
			case ALIGN_EXTERIOR_H|ALIGN_LEFT:
				new_pos.x = target_pos.x + target_size.w + offset_par.x;
				break;

			case ALIGN_EXTERIOR_H|ALIGN_RIGHT:
				new_pos.x = target_pos.x - new_size.w - offset_par.x;
				break;

			// if one of the alignments is NONE but offset,
			// use offset as absolute position.
			case ALIGN_NONE:
				if (offset_par.x) {
					new_pos.x = offset_par.x;
				}
				break;

		}

		// apply new position and size
		set_pos(new_pos);
		set_size(new_size);
	}

}
