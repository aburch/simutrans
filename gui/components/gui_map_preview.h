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
#include "../../simgraph.h"
#include "../../tpl/array2d_tpl.h"
//#include "../../simworld.h"

#define MAP_PREVIEW_SIZE_X ((KOORD_VAL)(64))
#define MAP_PREVIEW_SIZE_Y ((KOORD_VAL)(64))

/**
 * A map preview component
 *
 * @author Max Kielland
 * @date 2013-06-02
 *
 */
//class karte_t;
class gui_map_preview_t : public gui_komponente_t
{

	private:
		array2d_tpl<uint8> *map_data;
		koord map_size;

	public:
		gui_map_preview_t(void);

		void init( koord pos, koord size = koord(MAP_PREVIEW_SIZE_X,MAP_PREVIEW_SIZE_Y) ) {
			set_pos( pos );
			set_groesse( size );
		}

		void set_map_data(array2d_tpl<uint8> *map_data_par, koord max_size_par) {
			map_data = map_data_par; map_size = max_size_par;
		}

		/**
		 * Draws the component.
		 * @author Max Kielland, (Hj. Malthaner)
		 */
		virtual void zeichnen(koord offset) {
			display_ddd_box_clip(pos.x + offset.x, pos.y + offset.y, groesse.x, groesse.x, MN_GREY0, MN_GREY4);

			if(map_data) {
				display_array_wh(pos.x + offset.x + 1, pos.y + offset.y + 1, map_size.x, map_size.y, map_data->to_array());
			}
		}

};

#endif
