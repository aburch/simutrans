/*
 * Copyright (c) 2010 Bernd Gabriel
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_table_h
#define gui_table_h

#include "../../simtypes.h"
#include "../../tpl/list_tpl.h"
#include "../../tpl/vector_tpl.h"
#include "../../ifc/gui_action_creator.h"
#include "../../ifc/gui_komponente.h"
#include "../../simcolor.h"
#include "../gui_frame.h"

typedef KOORD_VAL koord_x;
typedef KOORD_VAL koord_y;
typedef KOORD_VAL coordinate_t;
typedef PLAYER_COLOR_VAL color_t;


/**
 * 
 *
 * @since 22-MAR-2010
 * @author Bernd Gabriel
 */
class coordinates_t
{
private:
	coordinate_t x;
	coordinate_t y;
public:
	coordinates_t() { x = y = 0; }
	coordinates_t(coordinate_t x, coordinate_t y) { this->x = x; this->y = y; }
	coordinate_t get_x() const { return x; }
	coordinate_t get_y() const { return y; }
	void set_x(coordinate_t value) { x = value; }
	void set_y(coordinate_t value) { y = value; }
	bool equals(const coordinates_t &value) const { return x == value.x && y == value.y; }
	bool operator == (const coordinates_t &value) { return equals(value); }
	bool operator != (const coordinates_t &value) { return !equals(value); }
};


class gui_table_t;
class gui_table_row_t;
class gui_table_column_t;


/**
 * 
 *
 * @since 22-MAR-2010
 * @author Bernd Gabriel
 */
class gui_table_column_t
{
private:
	char *name;
	coordinate_t width;
	bool sort_descendingly;
public:
	gui_table_column_t() { width = 99; name = NULL; sort_descendingly = false; }
	virtual int compare_rows(const gui_table_row_t &row1, const gui_table_row_t &row2) const { return 0; }
	const char *get_name() const { return name; }
	void set_name(const char *value) { if (name) free(name); name = NULL; if (value) name = strdup(value); }
	coordinate_t get_width() const { return width; }
	void set_width(coordinate_t value) { width = value; }
	bool get_sort_descendingly() { return sort_descendingly; }
	void set_sort_descendingly(bool value) { sort_descendingly = value; }
};


/**
 * 
 *
 * @since 22-MAR-2010
 * @author Bernd Gabriel
 */
class gui_table_row_t
{
private:
	char *name;
	coordinate_t height;
	bool sort_descendingly;
public:
	gui_table_row_t() { height = 14; name = NULL; sort_descendingly = false; }
	virtual int compare_columns(const gui_table_column_t &row1, const gui_table_column_t &row2) const { return 0; }
	const char *get_name() const { return name; }
	void set_name(const char *value) { if (name) free(name); name = NULL; if (value) name = strdup(value); }
	coordinate_t get_height() const { return height; }
	void set_height(coordinate_t value) { height = value; }
	bool get_sort_descendingly() { return sort_descendingly; }
	void set_sort_descendingly(bool value) { sort_descendingly = value; }
};


class gui_table_property_t 
{
private:
	gui_table_t *owner;
public:
	gui_table_property_t() { owner = NULL; }
	gui_table_t *get_owner() const { return owner; }
	void set_owner(gui_table_t *owner) {this->owner = owner; }
};


/**
 * 
 *
 * @since 22-MAR-2010
 * @author Bernd Gabriel
 */
class gui_table_column_list_t : public list_tpl<gui_table_column_t>, public gui_table_property_t {
protected:
	virtual int compare_items(gui_table_column_t *item1, gui_table_column_t *item2) const;
	virtual gui_table_column_t *create_item() { return new gui_table_column_t(); }
};


/**
 * 
 *
 * @since 22-MAR-2010
 * @author Bernd Gabriel
 */
class gui_table_row_list_t : public list_tpl<gui_table_row_t>, public gui_table_property_t {
protected:
	virtual int compare_items(gui_table_row_t *item1, gui_table_row_t *item2) const;
	virtual gui_table_row_t *create_item() { return new gui_table_row_t(); }
};


/**
 * gui_table_t::infowin_event() notifies each event to all listeners sending a pointer to an instance of this class.
 *
 * @since 22-MAR-2010
 * @author Bernd Gabriel
 */
class gui_table_event_t
{
private:
	gui_table_t *table;
	const event_t *ev;
public:
	gui_table_event_t(gui_table_t *table, const event_t *ev) {
		this->table = table;
		this->ev = ev;
	}

	/**
	 * the table that notifies.
	 */
	gui_table_t *get_table() const { return table; }

	/**
	 * the event that the notifying table received.
	 */
	const event_t *get_event() const { return ev; }

	/**
	 * Does the mouse (ev->mx, ev->my) point to a cell?
	 * False, if mouse is outside table or mouse points to grid/space around cells.
	 * True and the cell coordinates can be found in 'cell'.
	 */
	bool is_cell_hit;

	/**
	 * Contains the coordinates of the cell the mouse points to, if is_cell_hit is true.
	 * If is_cell_hit is false cell is undefined.
	 */
	coordinates_t cell;
};


/**
 * A table component. 
 * - allows any number of columns and rows with individual widths and heights.
 * - allows a grid of any width.
 *
 * @since 14-MAR-2010
 * @author Bernd Gabriel
 */
class gui_table_t : public gui_komponente_t, public gui_action_creator_t
{
	friend gui_table_column_list_t;
	friend gui_table_row_list_t;
private:
	uint16 grid_width;
	color_t grid_color;
	bool grid_visible;
	char tooltip[200];
	// arrays controlled by change_size() via set_size() or add_column()/remove_column()/add_row()/remove_row()
	gui_table_column_list_t columns;
	gui_table_column_list_t row_sort_column_order;
	gui_table_row_list_t rows;
	gui_table_row_list_t column_sort_row_order;
	//
	bool get_column_at(koord_x x, coordinate_t *column) const;
	bool get_row_at(koord_y y, coordinate_t *row) const;
protected:
	/**
	 * change_size() is called in set_size(), whenever the size actually changes.
	 */
	virtual void change_size(const coordinates_t &old_size, const coordinates_t &new_size);

	virtual gui_table_column_t *init_column(coordinate_t x) { return new gui_table_column_t(); }
	virtual gui_table_row_t *init_row(coordinate_t y) { return new gui_table_row_t(); }

	/**
	 * paint_cell() is called in paint_cells(), whenever a cell has to be painted.
	 *
	 * It has to paint cell (x,y) at position offset. 
	 * The default implementation calls zeichnen() of the component of cell (x,y), if there is one.
	 */
	virtual void paint_cell(const koord &offset, coordinate_t x, coordinate_t y);

	/**
	 * paint_cells() is called in zeichnen() after painting the grid.
	 *
	 * It has to paint the cell content. 
	 * The default implementation calls paint_cell() with the correct cell offset for each cell.
	 */
	virtual void paint_cells(const koord &offset);

	/**
	 * paint_grid() is called in zeichnen() before painting the cells.
	 *
	 * The default implementation draws grid_color lines of grid_width, if the grid is set to be visible.
	 */
	virtual void paint_grid(const koord &offset);
public:
	gui_table_t();
    ~gui_table_t();

	/**
	 * Get/set grid size.
	 *
	 * size.x is the number of cells horizontally.
	 * size.y is the number of cells vertically.
	 */
	const coordinates_t get_size() const {
		return coordinates_t(columns.get_count(), rows.get_count()); 
	}
	void set_size(const coordinates_t &value) { 
		const coordinates_t &old_size = get_size(); 
		if (!old_size.equals(value)) {
			change_size(old_size, value);
		}
	}
	bool get_owns_columns() const { return columns.get_owns_items(); }
	bool get_owns_rows() const { return rows.get_owns_items(); }
	void set_owns_columns(bool value) { columns.set_owns_items(value); }
	void set_owns_rows(bool value) { rows.set_owns_items(value); }
	virtual coordinate_t add_column(gui_table_column_t *column);
	virtual coordinate_t add_row(gui_table_row_t *row);
	virtual void remove_column(coordinate_t x);
	virtual void remove_row(coordinate_t y);
	gui_table_column_t *get_column(coordinate_t x) { return columns[x]; }
	gui_table_row_t *get_row(coordinate_t y) { return rows[y]; }

	/**
	 * Get/set grid width / space around cells.
	 *
	 * 0: draws no grid,
	 * 1: a 1 pixel wide line, ...
	 */
	uint16 get_grid_width() { return grid_width; }
	void set_grid_width(uint16 value) {	grid_width = value;	}

	/**
	 * Get/set grid color.
	 */
	color_t get_grid_color() { return grid_color; }
	void set_grid_color(color_t value) { grid_color = value; }

	/**
	 * Get/set grid visibility.
	 * if grid is not visible, grid is not painted, there is space around cells.
	 */
	bool get_grid_visible() { return grid_visible; }
	void set_grid_visible(bool value) { grid_visible = value; }
	
	/**
	 * Get/set width of columns and heights of rows.
	 */
	koord_x get_column_width(coordinate_t x) const { return columns[x]->get_width(); }
	koord_x get_table_width() const;
	koord_y get_row_height(coordinate_t y) const { return rows[y]->get_height(); }
	koord_y get_table_height() const;
	koord get_cell_size(coordinate_t x, coordinate_t y) const { return koord(get_column_width(x), get_row_height(y)); }
	koord get_table_size() const { return koord(get_table_width(), get_table_height()); }

	bool get_cell_at(koord_x x, koord_y y, coordinates_t *cell);

	void infowin_event(const event_t *ev);

	/**
	 * Set row sort order of a column. 
	 * @param x index of column, whose position in the sort order is to be set.
	 * @param prio 0: main sort column, prio > 0: sort column, if rows are the same in column prio - 1.
	 */
	void set_row_sort_column_prio(coordinate_t x, int prio = 0);
	void sort_rows();

	/**
	 * Set column sort order of a row. 
	 * @param y index of row, whose position in the sort order is to be set.
	 * @param prio 0: main sort row, prio > 0: sort row, if columns are the same in row prio - 1.
	 */
	void set_column_sort_row_prio(coordinate_t y, int prio = 0);
	void sort_columns();

	/**
	 * zeichnen() paints the table.
	 */
	virtual void zeichnen(koord offset);
};

#endif