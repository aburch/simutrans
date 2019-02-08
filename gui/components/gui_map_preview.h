/*
 * Displays a minimap
 *
 * Copyright (c) 2013 Max Kielland, (Hj. Malthaner)
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_gui_map_preview_h
#define gui_gui_map_preview_h

#include "gui_komponente.h"
#include "../../simcolor.h"
#include "../../display/simgraph.h"
#include "../../tpl/array2d_tpl.h"

#define MAP_PREVIEW_SIZE_X ((scr_coord_val)(64))
#define MAP_PREVIEW_SIZE_Y ((scr_coord_val)(64))

/**
 * A map preview component
 *
 * @author Max Kielland
 * @date 2013-06-02
 *
 */
class gui_map_preview_t : public gui_component_t
{

	private:
		array2d_tpl<PIXVAL> *map_data;

	public:
		gui_map_preview_t();

		void set_map_data(array2d_tpl<PIXVAL> *map_data_par) {
			map_data = map_data_par;
		}

		/**
		 * Draws the component.
		 * @author Max Kielland, (Hj. Malthaner)
		 */
		void draw(scr_coord offset) OVERRIDE {
			display_ddd_box_clip_rgb(pos.x + offset.x, pos.y + offset.y, size.w, size.h, color_idx_to_rgb(MN_GREY0), color_idx_to_rgb(MN_GREY4));

			if(map_data) {
				display_array_wh(pos.x + offset.x + 1, pos.y + offset.y + 1, map_data->get_width(), map_data->get_height(), map_data->to_array());
			}
		}

		scr_size get_min_size() const OVERRIDE { return scr_size(MAP_PREVIEW_SIZE_X, MAP_PREVIEW_SIZE_Y); }

		scr_size get_max_size() const OVERRIDE { return scr_size(MAP_PREVIEW_SIZE_X, MAP_PREVIEW_SIZE_Y); }
};

#endif
