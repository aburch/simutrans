/*
 * Copyright (c) 2001 Hansjörg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_components_gui_divider_h
#define gui_components_gui_divider_h

#include "gui_komponente.h"
#include "../../display/simgraph.h"
#include "../../simskin.h"

class skinverwaltung_t;

/**
 * A horizontal divider line
 *
 * @date 30-Oct-01
 * @author Markus Weber
 */
class gui_divider_t : public gui_komponente_t
{
public:
	gui_divider_t() { size.h = D_DIVIDER_HEIGHT; }

	void init( scr_coord xy, scr_coord_val width, scr_coord_val height = D_DIVIDER_HEIGHT ) {
		set_pos( xy );
		set_size( scr_size( width, height ) );
	};

	void set_width(scr_coord_val width) { set_size(scr_size(width, size.h)); }

	scr_size get_size() const { return scr_size(size.w,max(size.h, D_DIVIDER_HEIGHT)); }

	void draw(scr_coord offset)
	{
		display_img_stretch( gui_theme_t::divider, scr_rect( get_pos()+offset, get_size() ) );
	}
};

#endif
