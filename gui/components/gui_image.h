/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_IMAGE_H
#define GUI_COMPONENTS_GUI_IMAGE_H


#include "../../display/simimg.h"
#include "../../display/simgraph.h"
#include "gui_component.h"


/*
 * just displays an image
 */
class gui_image_t : public gui_component_t
{
		control_alignment_t alignment;
		image_id            id;
		uint16              player_nr;
		scr_coord           remove_offset;
		bool                remove_enabled;
		FLAGGED_PIXVAL      color_index;

	public:
		gui_image_t( const image_id i=IMG_EMPTY, const uint8 p=0, control_alignment_t alignment_par = ALIGN_NONE, bool remove_offset = false );
		void set_size( scr_size size_par ) OVERRIDE;
		void set_image( const image_id i, bool remove_offsets = false );

		void enable_offset_removal(bool remove_offsets) { set_image(id,remove_offsets); }

		void set_transparent(FLAGGED_PIXVAL c) { color_index = c; }

		/**
		 * Draw the component
		 */
		void draw( scr_coord offset ) OVERRIDE;

		scr_size get_min_size() const OVERRIDE;

		scr_size get_max_size() const OVERRIDE { return get_min_size(); }

};

#endif
