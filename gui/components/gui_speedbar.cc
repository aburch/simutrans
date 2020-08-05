/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>

#include "gui_speedbar.h"
#include "../../display/simgraph.h"
#include "../../simcolor.h"
#include "../../simtypes.h"
#include "../../simunits.h"


void gui_speedbar_t::set_base(sint32 base)
{
	this->base = base!=0 ? base : 1;
}


void gui_speedbar_t::add_color_value(const sint32 *value, uint8 color)
{
	info_t  next =  { color, value, -1 };
	values.insert(next);
}


void gui_speedbar_t::draw(scr_coord offset)
{
	offset += pos;

	if(vertical) {
		sint32 from = size.h;
		FOR(slist_tpl<info_t>, const& i, values) {
			sint32 const to = size.h - min(*i.value, base) * size.h / base;
			if(to < from) {
				display_fillbox_wh_clip(offset.x, offset.y + to, size.w, from - to, i.color, true);
				from = to - 1;
			}
		}
		if(from > 0) {
			display_fillbox_wh_clip( offset.x, offset.y, size.w, from, MN_GREY0, true);
		}
	}
	else {
		sint32 from = 0;
		FOR(slist_tpl<info_t>, const& i, values) {
			sint32 const to = min(*i.value, base) * size.w / base;
			if(to > from) {
				display_fillbox_wh_clip(offset.x + from, offset.y, to - from, size.h, i.color, true);
				from = to + 1;
			}
		}
		if(from < size.w) {
			display_fillbox_wh_clip(offset.x + from, offset.y, size.w - from, size.h, MN_GREY0, true);
		}
	}
}


void gui_tile_occupancybar_t::set_base_convoy_length(uint32 convoy_length, uint8 last_veh_length)
{
	this->convoy_length = convoy_length;
	this->last_veh_length = last_veh_length;
}

void gui_tile_occupancybar_t::set_assembling_incomplete(bool incomplete)
{
	this->incomplete = incomplete;
}

void gui_tile_occupancybar_t::set_new_veh_length(sint8 new_veh_length, bool insert, uint8 new_last_veh_length)
{
	this->new_veh_length = new_veh_length;
	this->insert_mode = insert;
	if (new_last_veh_length != 0xFFu) {
		this->switched_last_veh_length = new_last_veh_length;
	}
}

uint32 gui_tile_occupancybar_t::adjust_convoy_length(uint32 total_len, uint8 last_veh_len)
{
	if (!total_len) {
		return 0;
	}
	return total_len + (max(CARUNITS_PER_TILE / 2, last_veh_len) - last_veh_len);
}

void gui_tile_occupancybar_t::fill_with_color(scr_coord offset, uint8 tile_index, uint8 from, uint8 to, COLOR_VAL color, uint8 tile_scale)
{
	display_fillbox_wh_clip(offset.x + (CARUNITS_PER_TILE * tile_scale + 4)*tile_index + 1 + from * tile_scale, offset.y + 1, (to - from) * tile_scale, size.h - 2, color, true);
}


void gui_tile_occupancybar_t::draw(scr_coord offset)
{
	uint8 length_to_pixel = 2;// One tile is represented by 16 times longer (1tile = 16length) pixels
	offset += pos;

	// calculate internal convoy length (include margin)
	const uint32 current_length = adjust_convoy_length(convoy_length, last_veh_length);
	const uint8	extra_margin = current_length - convoy_length;
	uint32 new_length = 0;

	if (insert_mode && new_veh_length != 0) {
		new_length = adjust_convoy_length(convoy_length + new_veh_length, last_veh_length);
	}
	else if (new_veh_length <= 0) {
		new_length = adjust_convoy_length(convoy_length + new_veh_length, switched_last_veh_length);
	}
	else if(new_veh_length != 0) {
		new_length = adjust_convoy_length(convoy_length + new_veh_length, new_veh_length);
	}

	const sint8 len_diff = new_length - current_length;
	COLOR_VAL col = len_diff < 0 ? COL_REDUCED : COL_ADDITIONAL;
	uint8 last_tile_occupancy = max(current_length, new_length) % CARUNITS_PER_TILE ? max(current_length, new_length) % CARUNITS_PER_TILE : CARUNITS_PER_TILE;
	const uint8 tiles = (max(current_length, new_length) + CARUNITS_PER_TILE - 1) / CARUNITS_PER_TILE;

	// tile size scaling
	if ((CARUNITS_PER_TILE*2+4)*(tiles+1) > size.w) {
		length_to_pixel = 1;
	}
	uint8 tilebar_width = CARUNITS_PER_TILE * length_to_pixel;

	for (int i = 0; i < tiles; i++)
	{
		// draw frame and base color
		display_ddd_box_clip(offset.x + (tilebar_width + 4) * i, offset.y, tilebar_width + 2, size.h, 8, 8);
		fill_with_color(offset, i, 0, i == tiles - 1 ? last_tile_occupancy : CARUNITS_PER_TILE, COL_GREY4, length_to_pixel);
		if (insert_mode && len_diff > 0 && len_diff > CARUNITS_PER_TILE*i) {
			// insert mode, paint the front tile
			fill_with_color(offset, i, 0, len_diff - CARUNITS_PER_TILE*(i+1) < 0 ? len_diff % CARUNITS_PER_TILE : CARUNITS_PER_TILE, col, length_to_pixel);
		}
		else if (!insert_mode && len_diff != 0 && i >= min(current_length + extra_margin, new_length + extra_margin)/ CARUNITS_PER_TILE){
			fill_with_color(offset, i, min(current_length + extra_margin, new_length + extra_margin)/ CARUNITS_PER_TILE == i ? min(current_length + extra_margin, new_length + extra_margin) % CARUNITS_PER_TILE : 0,
				max(current_length, new_length) / CARUNITS_PER_TILE == i ? max(current_length, new_length) % CARUNITS_PER_TILE : CARUNITS_PER_TILE, col, length_to_pixel);
		}
	}
	if(incomplete){
		fill_with_color(offset, last_tile_occupancy == CARUNITS_PER_TILE ? tiles : tiles-1, max(current_length, new_length) % CARUNITS_PER_TILE, max(current_length, new_length) % CARUNITS_PER_TILE + 1, COL_YELLOW, length_to_pixel);
		//add one more tile
		//if (last_tile > 0 && last_tile_occupancy == CARUNITS_PER_TILE) {
		//	display_ddd_box_clip(offset.x + (tilebar_width + 4) * last_tile, offset.y, tilebar_width + 2, size.h, 8, 8);
		//}
	}
}

