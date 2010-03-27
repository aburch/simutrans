/*
 * Copyright (c) 2010 Bernd Gabriel
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include "gui_table.h"
#include "gui_label.h"
#include "gui_button.h"
#include "../../simgraph.h"
#include "../../simwin.h"

// BG, 18.03.2010
gui_table_t::gui_table_t() {
	set_owns_columns(true);
	set_owns_rows(true);
	grid_width = 1;
	grid_color = COL_BLACK;
	tooltip[0] = 0;
}

// BG, 18.03.2010
gui_table_t::~gui_table_t()
{
	set_size(coordinates_t(0, 0));
}

// BG, 22.03.2010
coordinate_t gui_table_t::add_column(gui_table_column_t *column) {
	coordinate_t x = column_defs.add(column);
	coordinate_t h = row_defs.get_count();
	for (coordinate_t y = 0; y < h; y++) {
		init_cell(x, y);
	}
	return x;
}

// BG, 22.03.2010
coordinate_t gui_table_t::add_row(gui_table_row_t *row) {
	coordinate_t w = column_defs.get_count();
	coordinate_t y = row_defs.add(row);
	for (coordinate_t x = 0; x < w; x++) {
		init_cell(x, y);
	}
	return y;
}


// BG, 18.03.2010
void gui_table_t::change_size(const coordinates_t &new_size) {
	coordinates_t old_size = get_size();

	// remove no longer needed cells
	if (gui_cells.get_count())
	{
		for (coordinate_t x = new_size.get_x(); x < old_size.get_x(); x++) {
			for (coordinate_t y = 0; y < old_size.get_y(); y++) {
				remove_cell(x, y);
			}
		}
		for (coordinate_t y = new_size.get_y(); y < old_size.get_y(); y++) {
			for (coordinate_t x = 0; x < min(old_size.get_x(), new_size.get_x()); x++) {
				remove_cell(x, y);
			}
		}
	}

	// change size of arrays
	row_defs.resize(new_size.get_y());
	column_defs.resize(new_size.get_x());
	gui_cells.resize(new_size.get_x());
	for (coordinate_t x = 0; x < new_size.get_x(); x++) {
		gui_cells[x].resize(new_size.get_y());
	}

	// initialize new cells
	for (coordinate_t x = old_size.get_x(); x < new_size.get_x(); x++) {
		for (coordinate_t y = 0; y < new_size.get_y(); y++) {
			init_cell(x, y);
		}
	}
	for (coordinate_t y = old_size.get_y(); y < new_size.get_y(); y++) {
		for (coordinate_t x = 0; x < min(old_size.get_x(), new_size.get_x()); x++) {
			init_cell(x, y);
		}
	}
}

// BG, 26.03.2010
bool gui_table_t::get_column_at(koord_x x, coordinate_t *column)
{
	coordinate_t n = column_defs.get_count();
	koord_x ref = 0;
	for (coordinate_t i = 0; i < n; i++) {
		ref += grid_width;
		if (x < ref) {
			return false;
		}
		ref += get_column_width(i);
		if (x < ref) {
			*column = i;
			return true;
		}
	}
	return false;
}

// BG, 26.03.2010
bool gui_table_t::get_row_at(koord_y y, coordinate_t *row)
{
	coordinate_t n = row_defs.get_count();
	koord_y ref = 0;
	for (coordinate_t i = 0; i < n; i++) {
		ref += grid_width;
		if (y < ref) {
			return false;
		}
		ref += get_row_height(i);
		if (y < ref) {
			*row = i;
			return true;
		}
	}
	return false;
}

// BG, 26.03.2010
bool gui_table_t::get_cell_at(koord_x x, koord_y y, coordinates_t *cell)
{
	coordinate_t cx, cy;
	if (get_column_at(x, &cx) && get_row_at(y, &cy)) {
		cell->set_x(cx);
		cell->set_y(cy);
		return true;
	}
	return false;	
}

// BG, 18.03.2010
koord_x gui_table_t::get_table_width() {
	coordinate_t i = column_defs.get_count();
	koord_x width = (i + 1) * grid_width;
	for (; i-- > 0;) {
		width += get_column_width(i);
	}
	return width;
}

// BG, 18.03.2010
koord_y gui_table_t::get_table_height() {
	coordinate_t i = row_defs.get_count();
	koord_y height = (i + 1) * grid_width;
	for (; i-- > 0;) {
		height += get_row_height(i);
	}
	return height;
}

// BG, 26.03.2010
void gui_table_t::infowin_event(const event_t *ev)
{
	gui_table_event_t table_event(this, ev);
	table_event.is_cell_hit = get_cell_at(ev->mx, ev->my, &table_event.cell);
	call_listeners(value_t(&table_event));
}

// BG, 18.03.2010
void gui_table_t::init_cell(coordinate_t x, coordinate_t y) {
}

// BG, 18.03.2010
void gui_table_t::paint_cell(const koord &offset, coordinate_t x, coordinate_t y) {
	gui_komponente_t *component = get_cell_component(x, y);
	if (component)
	{
		component->zeichnen(offset);
	}
}

// BG, 18.03.2010
void gui_table_t::paint_grid(const koord &offset) {
	coordinates_t size = get_size();
	koord s(get_table_width(), get_table_height());
	koord h = get_pos() + offset;
	koord v = h;

	// paint horizontal grid lines
	for (coordinate_t y = 0; y < size.get_y(); y++) {
		display_fillbox_wh_clip(h.x, h.y, s.x, grid_width, grid_color, true);
		h.y += grid_width + get_row_height(y);
	}
	display_fillbox_wh_clip(h.x, h.y, s.x, grid_width, grid_color, true);

	// paint vertical grid lines
	for (coordinate_t x = 0; x < size.get_x(); x++) {
		display_fillbox_wh_clip(v.x, v.y, grid_width, s.y, grid_color, true);
		v.x += grid_width + get_column_width(x);
	}
	display_fillbox_wh_clip(v.x, v.y, grid_width, s.y, grid_color, true);
}

// BG, 18.03.2010
void gui_table_t::remove_cell(coordinate_t x, coordinate_t y) {
	gui_komponente_t *cell = get_cell_component(x, y);
	if (cell) {
		set_cell_component(x, y, NULL);
		delete cell;
	}
}

// BG, 27.03.2010
void gui_table_t::remove_column(coordinate_t x) 
{
	// remove cells
	for (coordinate_t y = get_size().get_y(); y > 0; ) {
		remove_cell(x, --y);
	}

	// change size of arrays
	column_defs.remove(x);
	gui_cells.remove_at(x);
}

// BG, 27.03.2010
void gui_table_t::remove_row(coordinate_t y)
{
	// remove cells and change size of arrays
	for (coordinate_t x = get_size().get_x(); x > 0; ) {
		remove_cell(--x, y);
		gui_cells[x].remove(y);
	}
	row_defs.remove(y);
}

// BG, 18.03.2010
void gui_table_t::zeichnen(koord offset) {
	coordinates_t size = get_size();
	koord pos = get_pos() + offset;

	// paint cells
	koord cell_pos;
	cell_pos.y = pos.y + grid_width;
	for (coordinate_t y = 0; y < size.get_y(); y++) {
		cell_pos.x = pos.x + grid_width;
		for (coordinate_t x = 0; x < size.get_x(); x++) {
			paint_cell(cell_pos, x, y);
			cell_pos.x += grid_width + get_column_width(x);
		}
		cell_pos.y += grid_width + get_row_height(y);
	}
	if (get_grid_visible())
	{
		paint_grid(offset);
	}
	if (*tooltip)
	{
		win_set_tooltip(get_maus_x() + 16, get_maus_y() - 16, tooltip );
	}
}
