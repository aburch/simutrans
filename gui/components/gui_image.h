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

#include "../../display/simimg.h"
#include "../../display/simgraph.h"
#include "gui_komponente.h"



class gui_image_t : public gui_komponente_t
{
		control_alignment_t alignment;
		image_id            id;
		uint16              player_nr;
		scr_coord           remove_offset;
		bool                remove_enabled;

	public:
		gui_image_t( const image_id i=IMG_LEER, const uint8 p=0, control_alignment_t alignment_par = ALIGN_NONE, bool remove_offset = false );
		void set_size( scr_size size_par ) OVERRIDE;
		void set_image( const image_id i, bool remove_offsets = false );

		void enable_offset_removal(bool remove_offsets) { set_image(id,remove_offsets); }

		/**
		 * Draw the component
		 * @author Hj. Malthaner
		 */
		void draw( scr_coord offset );
};

#endif
