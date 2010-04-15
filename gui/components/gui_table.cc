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
gui_table_t::gui_table_t() : default_cell_size(99,14)
{
	columns.set_owner(this);
	rows.set_owner(this);
	row_sort_column_order.set_owner(this);
	column_sort_row_order.set_owner(this);
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
	coordinate_t x = columns.add(column);
	return x;
}


// BG, 22.03.2010
coordinate_t gui_table_t::add_row(gui_table_row_t *row) {
	coordinate_t y = rows.add(row);
	return y;
}


// BG, 18.03.2010
void gui_table_t::change_size(const coordinates_t &old_size, const coordinates_t &new_size) {
	// change size of arrays
	rows.set_count(new_size.get_y());
	row_sort_column_order.set_count(new_size.get_x());
	columns.set_count(new_size.get_x());
	column_sort_row_order.set_count(new_size.get_y());
}


// BG, 26.03.2010
bool gui_table_t::get_column_at(koord_x x, coordinate_t &column, koord_x &offset) const
{
	coordinate_t n = columns.get_count();
	koord_x ref = 0;
	for (coordinate_t i = 0; i < n; i++) {
		ref += grid_width;
		if (x < ref) {
			return false;
		}
		koord_x width = get_column_width(i);
		ref += width;
		if (x < ref) {
			column = i;
			offset = ref - width; 
			return true;
		}
	}
	return false;
}


// BG, 26.03.2010
bool gui_table_t::get_row_at(koord_y y, coordinate_t &row, koord_y &offset) const
{
	coordinate_t n = rows.get_count();
	koord_y ref = 0;
	for (coordinate_t i = 0; i < n; i++) {
		ref += grid_width;
		if (y < ref) {
			return false;
		}
		koord_y height = get_row_height(i);
		ref += height;
		if (y < ref) {
			row = i;
			offset = ref - height;
			return true;
		}
	}
	return false;
}


// BG, 26.03.2010
bool gui_table_t::get_cell_at(koord_x x, koord_y y, coordinates_t &cell, koord &offset)
{
	coordinate_t cx, cy;
	if (get_column_at(x, cx, offset.x) && get_row_at(y, cy, offset.y)) {
		cell.set_x(cx);
		cell.set_y(cy);
		return true;
	}
	return false;	
}


// BG, 18.03.2010
koord_x gui_table_t::get_table_width() const {
	coordinate_t i = columns.get_count();
	koord_x width = (i + 1) * grid_width;
	for (; i-- > 0;) {
		width += get_column_width(i);
	}
	return width;
}


// BG, 18.03.2010
koord_y gui_table_t::get_table_height() const {
	coordinate_t i = rows.get_count();
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
	table_event.is_cell_hit = get_cell_at(ev->mx, ev->my, table_event.cell, table_event.offset);
	call_listeners(value_t(&table_event));
}


// BG, 18.03.2010
void gui_table_t::paint_cell(const koord &offset, coordinate_t x, coordinate_t y) {
}


// BG, 18.03.2010
void gui_table_t::paint_cells(const koord &offset) {
	coordinates_t size = get_size();
	koord pos = offset;

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


// BG, 27.03.2010
void gui_table_t::remove_column(coordinate_t x) 
{
	columns.remove(x);
}


// BG, 27.03.2010
void gui_table_t::remove_row(coordinate_t y)
{
	rows.remove(y);
}


// BG, 04.04.2010
void gui_table_t::set_column_sort_row_prio(coordinate_t y, int prio)
{
	assert(y >= 0 && (uint32) y < rows.get_count());
	gui_table_row_t *row = rows.get(y);
	int index = column_sort_row_order.index_of(row);
	if (index >= 0) 
	{
		column_sort_row_order.move(index, prio);
	}
	else 
	{
		if (column_sort_row_order.get_count() < rows.get_count()) {
			column_sort_row_order.set_count(rows.get_count());
		}
		column_sort_row_order.insert(prio, row);
	}
}


// BG, 04.04.2010
void gui_table_t::sort_columns()
{
	if (column_sort_row_order.get_count() > 0) {
		columns.sort();
	}
}


// BG, 04.04.2010
void gui_table_t::set_row_sort_column_prio(coordinate_t x, int prio)
{
	gui_table_column_t *column = columns.get(x);
	int index = row_sort_column_order.index_of(column);
	if (index >= 0) 
	{
		row_sort_column_order.move(index, prio);
	}
	else 
	{
		if (row_sort_column_order.get_count() < columns.get_count()) {
			row_sort_column_order.set_count(columns.get_count());
		}
		row_sort_column_order.insert(prio, column);
	}
}


// BG, 04.04.2010
void gui_table_t::sort_rows()
{
	if (row_sort_column_order.get_count() > 0) {
		rows.sort();
	}
}


// BG, 04.04.2010
int gui_table_column_list_t::compare_items(const gui_table_column_t *item1, const gui_table_column_t *item2) const
{
	gui_table_t *table = get_owner();
	assert(table);
	const gui_table_row_list_t &rows = table->column_sort_row_order;
	int n = rows.get_count();
	if (!n) {
		return -1;
	}
	int result = 0;
	for (uint32 i = 0; !result && i < n; i++)
	{
		gui_table_row_t *row = rows.get(i);
		result = row->compare_columns(*item1, *item2);
		if (result && row->get_sort_descendingly())
			result = -result;
	}
	return result;
}


// BG, 11.02.2010
gui_table_column_t *gui_table_column_list_t::create_item() const { 
	koord_x width = 99;
	gui_table_t *owner = get_owner();
	if (owner) {
		width = owner->get_default_column_width();
	}
	return new gui_table_column_t(width); 
}


// BG, 04.04.2010
int gui_table_row_list_t::compare_items(const gui_table_row_t *item1, const gui_table_row_t *item2) const
{
	gui_table_t *table = get_owner();
	assert(table);
	const gui_table_column_list_t &columns = table->row_sort_column_order;
	int n = columns.get_count();
	if (!n) {
		return -1;
	}
	int result = 0;
	for (uint32 i = 0; !result && i < n; i++)
	{
		gui_table_column_t *column = columns.get(i);
		result = column->compare_rows(*item1, *item2);
		if (result && column->get_sort_descendingly())
			result = -result;
	}
	return result;
}


// BG, 11.02.2010
gui_table_row_t *gui_table_row_list_t::create_item() const { 
	koord_y height = 14;
	gui_table_t *owner = get_owner();
	if (owner) {
		height = owner->get_default_row_height();
	}
	return new gui_table_row_t(height); 
}


// BG, 18.03.2010
void gui_table_t::zeichnen(koord offset) {
	coordinates_t size = get_size();
	koord pos = get_pos() + offset;

	if (get_grid_visible())
	{
		paint_grid(offset);
	}
	paint_cells(pos);
	if (*tooltip)
	{
		win_set_tooltip(get_maus_x() + 16, get_maus_y() - 16, tooltip );
	}
}
