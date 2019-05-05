/*
 * Copyright (c) 2001 Hansjörg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_components_gui_divider_h
#define gui_components_gui_divider_h

#include "gui_component.h"
#include "../gui_theme.h"

/**
 * A horizontal divider line
 *
 * @date 30-Oct-01
 * @author Markus Weber
 */
class gui_divider_t : public gui_component_t
{
public:
	// TODO remove later
	void init( scr_coord xy, scr_coord_val width, scr_coord_val height = D_DIVIDER_HEIGHT ) {
		set_pos( xy );
		set_size( scr_size( width, height ) );
	};

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE;

	void draw(scr_coord offset) OVERRIDE;
};

#endif
