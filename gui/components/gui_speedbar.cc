/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>

#include "gui_speedbar.h"
#include "../gui_theme.h"
#include "../../display/simgraph.h"
#include "../../simcolor.h"
#include "../../simtypes.h"
#include "../../simunits.h"


void gui_speedbar_t::set_base(sint32 base)
{
	this->base = base!=0 ? base : 1;
}


void gui_speedbar_t::add_color_value(const sint32 *value, PIXVAL color)
{
	info_t  next =  { color, value, -1 };
	values.insert(next);
}


scr_size gui_speedbar_t::get_min_size() const
{
	return D_INDICATOR_SIZE;
}

scr_size gui_speedbar_t::get_max_size() const
{
	return scr_size(scr_size::inf.w, D_INDICATOR_HEIGHT);
}

void gui_speedbar_t::draw(scr_coord offset)
{
	offset += pos;

	if(vertical) {
		sint32 from = size.h;
		FOR(slist_tpl<info_t>, const& i, values) {
			sint32 const to = size.h - min(*i.value, base) * size.h / base;
			if(to < from) {
				display_fillbox_wh_clip_rgb(offset.x, offset.y + to, size.w, from - to, i.color, true);
				from = to - 1;
			}
		}
		if(from > 0) {
			display_fillbox_wh_clip_rgb( offset.x, offset.y, size.w, from, color_idx_to_rgb(MN_GREY0), true);
		}
	}
	else {
		sint32 from = 0;
		FOR(slist_tpl<info_t>, const& i, values) {
			sint32 const to = min(*i.value, base) * size.w / base;
			if(to > from) {
				display_fillbox_wh_clip_rgb(offset.x + from, offset.y, to - from, size.h, i.color, true);
				from = to + 1;
			}
		}
		if(from < size.w) {
			display_fillbox_wh_clip_rgb(offset.x + from, offset.y, size.w - from, size.h, color_idx_to_rgb(MN_GREY0), true);
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

void gui_tile_occupancybar_t::fill_with_color(scr_coord offset, uint8 tile_index, uint8 from, uint8 to, PIXVAL color, uint8 tile_scale)
{
	display_fillbox_wh_clip_rgb(offset.x + (CARUNITS_PER_TILE * tile_scale + 4)*tile_index + 1 + from * tile_scale, offset.y + 1, (to - from) * tile_scale, size.h - 2, color, true);
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
	PIXVAL col = len_diff < 0 ? COL_REDUCED : COL_ADDITIONAL;
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
		display_ddd_box_clip_rgb(offset.x + (tilebar_width + 4) * i, offset.y, tilebar_width + 2, size.h, color_idx_to_rgb(8), color_idx_to_rgb(8));
		fill_with_color(offset, i, 0, i == tiles - 1 ? last_tile_occupancy : CARUNITS_PER_TILE, color_idx_to_rgb(COL_GREY4), length_to_pixel);
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
		fill_with_color(offset, last_tile_occupancy == CARUNITS_PER_TILE ? tiles : tiles-1, max(current_length, new_length) % CARUNITS_PER_TILE, max(current_length, new_length) % CARUNITS_PER_TILE + 1, COL_CAUTION, length_to_pixel);
		//add one more tile
		//if (last_tile > 0 && last_tile_occupancy == CARUNITS_PER_TILE) {
		//	display_ddd_box_clip(offset.x + (tilebar_width + 4) * last_tile, offset.y, tilebar_width + 2, size.h, 8, 8);
		//}
	}
}


void gui_routebar_t::set_base(sint32 base)
{
	this->base = base != 0 ? base : 1;
}

void gui_routebar_t::init(const sint32 *value, uint8 state)
{
	this->value = value;
	this->state = state;
}

void gui_routebar_t::set_state(uint8 state)
{
	this->state = state;
}


void gui_routebar_t::draw(scr_coord offset)
{
	uint8  h = size.h % 2 ? size.h : size.h-1;
	if (h < 5) { h = 5; set_size( scr_size(size.w, h) ); }
	const uint16 w = size.w - h;

	offset += pos;
	display_fillbox_wh_clip_rgb(offset.x+h/2, offset.y+h/2-1, w, 3, color_idx_to_rgb(MN_GREY1), true);

	PIXVAL col;
	for (uint8 i = 0; i<5; i++) {
		col = i % 2 ? COL_GREY4-1 : MN_GREY0;
		display_vline_wh_rgb(offset.x + h/2 + w*i/4, offset.y+i%2, h-(i%2)*2, color_idx_to_rgb(col), true);
	}
	sint32 const to = min(*value, base) * w / base;

	display_fillbox_wh_clip_rgb(offset.x+h/2, offset.y+h/2-1, to, 3, color_idx_to_rgb(43), true);

	switch (state)
	{
		case 1:
			display_fillbox_wh_clip_rgb(offset.x + to + 1, offset.y + 1, h - 2, h - 2, COL_CAUTION, true);
			break;
		case 2:
			display_fillbox_wh_clip_rgb(offset.x + to + 1, offset.y + 1, h - 2, h - 2, COL_WARNING, true);
			break;
		case 3:
			display_fillbox_wh_clip_rgb(offset.x + to + 1, offset.y + 1, h - 2, h - 2, COL_DANGER, true);
			break;
		case 0:
		default:
			display_right_triangle_rgb(offset.x + to, offset.y, h, COL_CLEAR, true);
			break;
	}
}

scr_size gui_routebar_t::get_min_size() const
{
	return scr_size(D_INDICATOR_WIDTH, size.h);
}

scr_size gui_routebar_t::get_max_size() const
{
	return scr_size(scr_size::inf.w, size.h);
}


void gui_bandgraph_t::add_color_value(const sint32 *value, PIXVAL color)
{
	info_t next = { color, value };
	values.insert(next);
}

scr_size gui_bandgraph_t::get_min_size() const
{
	return D_INDICATOR_SIZE;
}

scr_size gui_bandgraph_t::get_max_size() const
{
	return scr_size(scr_size::inf.w, D_INDICATOR_HEIGHT);
}

void gui_bandgraph_t::draw(scr_coord offset)
{
	offset += pos;
	total = 0;
	FOR(slist_tpl<info_t>, const& i, values) {
		total += *i.value;
	}

	if (total==0) {
		display_fillbox_wh_clip_rgb(offset.x, offset.y, size.w, size.h, COL_INACTIVE, true);
	}
	else{
		sint32 temp = 0;
		KOORD_VAL end = 0;
		FOR(slist_tpl<info_t>, const& i, values) {
			if (*i.value>0) {
				temp += (*i.value);
				const KOORD_VAL from = size.w * temp / total + 0.5;
				const KOORD_VAL width = from-end;
				if (width) {
					display_fillbox_wh_clip_rgb(offset.x + size.w - from, offset.y, width, size.h, i.color, true);
				}
				end += width;
			}
		}
	}
}
