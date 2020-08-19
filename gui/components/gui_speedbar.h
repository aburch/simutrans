/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_SPEEDBAR_H
#define GUI_COMPONENTS_GUI_SPEEDBAR_H


#include "gui_component.h"
#include "../../tpl/slist_tpl.h"


/**
 *
 * @author Volker Meyer
 * @date  12.06.2003
 */
class gui_speedbar_t : public gui_component_t
{
private:
	struct info_t {
		sint32 color;
		const sint32 *value;
		sint32 last;
	};

	slist_tpl <info_t> values;

	sint32 base;
	bool vertical;

public:
	gui_speedbar_t() { base = 100; vertical = false; }

	void add_color_value(const sint32 *value, uint8 color);

	void set_base(sint32 base);

	void set_vertical(bool vertical) { this->vertical = vertical; }

	/**
	 * Draw the component
	 */
	void draw(scr_coord offset);
};


/**
 *
 * @author Ranran
 * @date  January 2020
 */
class gui_tile_occupancybar_t : public gui_component_t
{
private:
	uint32 convoy_length;
	bool insert_mode = false;
	bool incomplete = false;
	sint8 new_veh_length = 0;

	// returns convoy length including extra margin.
	// for some reason, convoy may have "extra margin"
	// this correction corresponds to the correction in convoi_t::get_tile_length()
	uint32 adjust_convoy_length(uint32 total_len, uint8 last_veh_len);
	// these two are needed for adding automatic margin
	uint8 last_veh_length;
	uint8 switched_last_veh_length = -1;

	// specify fill width and color of specified tile
	void fill_with_color(scr_coord offset, uint8 tile_index, uint8 from, uint8 to, COLOR_VAL color, uint8 length_to_pixel);

public:
	void set_base_convoy_length(uint32 convoy_length, uint8 last_veh_length);

	// 3rd argument - If remove the last vehicle, need to set the value of the previous vehicle length.
	// This is necessary to calculate the correct value of length related adding automatic margin.
	void set_new_veh_length(sint8 new_veh_length, bool insert = false, uint8 new_last_veh_length = 0xFFu);

	void set_assembling_incomplete(bool incomplete);

	void draw(scr_coord offset);
};

#endif
