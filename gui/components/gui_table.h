/*
 * Copyright (c) 2010 Bernd Gabriel
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_table_h
#define gui_table_h

#include "../../simtypes.h"
#include "../../tpl/vector_tpl.h"
#include "../../ifc/gui_action_creator.h"
#include "../../ifc/gui_komponente.h"
#include "../../simcolor.h"
#include "../gui_frame.h"


/**
 * 
 *
 * @since 22-MAR-2010
 * @author Bernd Gabriel
 */
template<class item_t> class list_tpl 
{
private:
	item_t **data;
	uint32 capacity;
	uint32 count;
	bool owns_items;

	void set_capacity(uint32 value) {
		assert(value <= (uint32)(UINT32_MAX_VALUE >> 1));
		if (capacity > value) {
			for (; capacity > value; ) {
				if (data[--capacity]) {
					delete_item(data[capacity]);
					data[capacity] = NULL;
				}
			}
			if (count > value)
			{
				count = value;
			}
		}
		else if (capacity < value) {
			data = (item_t**) realloc(data, sizeof(item_t*) * value);
			for (; capacity < value; capacity++) {
				data[capacity] = NULL;
			}
		}
	}

	void grow() {
		// resulting capacities: 16, 24, 36, 54, 81, 121, 181, 271, 406, 609, 913, ...
		set_capacity(capacity < 16 ? 16 : capacity + capacity / 2);
	}
protected:
	virtual signed compare_item(item_t *item1, item_t *item2) {
		return (signed) (item1 - item2);
	}
	virtual item_t *create_item() { 
		return NULL;
	}
	virtual void delete_item(item_t *item) { 
		if (item && owns_items) {
			delete [] item; 
		}
	}
public:
	explicit list_tpl() { data = NULL; count = capacity = 0; this->owns_items = false; }
	explicit list_tpl(uint32 capacity) { list_tpl(); set_capacity(capacity); }

	uint32 get_capacity() const { return capacity; }
	uint32 get_count() const { return count; }
	void resize(uint32 new_count) {
		set_capacity(new_count);
		while (count < new_count) {
			data[count++] = create_item();
		}
	}
	bool get_owns_items() { return owns_items; }
	void set_owns_items(bool value) { owns_items = value; }

	item_t *get(uint32 index) {
		assert(index < count);
		return data[index];
	}

	item_t *set(uint32 index, item_t *item) {
		assert(index < count);
		item_t *old = data[index];
		data[count] = item;
		if (owns_items)
		{
			delete_item(old);
			old = NULL;
		}
		return old;
	}

	sint32 add(item_t *item) {
		if (count == capacity) {
			grow();
		}
		data[count] = item;
		return (sint32) count++;
	}

	void insert(uint32 index, item_t *item) {
		assert(index <= count);
		if (count == capacity) {
			grow();
		}
		for (uint32 i = count++; i > index; i--) {
			data[i] = data[i-1];
		}
		data[index] = item;
	}

	sint32 index_of(item_t *item) {
		sint32 index = (sint32) count; 
		while (--index >= 0){
			if (!compare_item(data[index], item)){
				break;
			}
		}
		return index;
	}

	item_t *extract(uint32 index) {
		assert(index < count);
		item_t *item = data[index];
		while (++index < count)
		{
			data[index-1] = data[index];
		}
		data[--count] = NULL;
		return item;
	}
	void remove(uint32 index) {	delete_item(extract(index)); }
	void remove(item_t *) {	delete_item(extract(index)); }
	void sort() { qsort(data, count, sizeof(item_t*), compare_item); }

	item_t* &operator [] (uint32 index) { 		
		assert(index < count);
		return data[index];
	}
};

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


/**
 * 
 *
 * @since 22-MAR-2010
 * @author Bernd Gabriel
 */
class gui_table_column_t
{
private:
	coordinate_t width;
public:
	gui_table_column_t() { width = 99; }
	coordinate_t get_width() const { return width; }
	void set_width(coordinate_t value) { width = value; }
};


/**
 * 
 *
 * @since 22-MAR-2010
 * @author Bernd Gabriel
 */
class gui_table_column_list_t : public list_tpl<gui_table_column_t> {
protected:
	virtual gui_table_column_t *create_item() { 
		return new gui_table_column_t();
	}
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
	coordinate_t height;
public:
	gui_table_row_t() { height = 14; }
	coordinate_t get_height() const { return height; }
	void set_height(coordinate_t value) { height = value; }
};


/**
 * 
 *
 * @since 22-MAR-2010
 * @author Bernd Gabriel
 */
class gui_table_row_list_t : public list_tpl<gui_table_row_t> {
protected:
	virtual gui_table_row_t *create_item() { 
		return new gui_table_row_t();
	}
};


class gui_table_t;


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
private:
	uint16 grid_width;
	color_t grid_color;
	bool grid_visible;
	char tooltip[200];
	// arrays controlled by change_size() via set_size() or add_column()/remove_column()/add_row()/remove_row()
	gui_table_column_list_t columns;
	gui_table_row_list_t rows;
	//
	bool get_column_at(koord_x x, coordinate_t *column);
	bool get_row_at(koord_y y, coordinate_t *row);
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
	bool get_owns_columns() { return columns.get_owns_items(); }
	bool get_owns_rows() { return rows.get_owns_items(); }
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
	koord_x const get_column_width(coordinate_t x) { return columns[x]->get_width(); }
	koord_x get_table_width();
	koord_y const get_row_height(coordinate_t y) { return rows[y]->get_height(); }
	koord_y get_table_height();
	koord const get_cell_size(coordinate_t x, coordinate_t y) { return koord(get_column_width(x), get_row_height(y)); }
	koord const get_table_size() { return koord(get_table_width(), get_table_height()); }

	bool get_cell_at(koord_x x, koord_y y, coordinates_t *cell);

	void infowin_event(const event_t *ev);

	/**
	 * zeichnen() paints the table.
	 */
	virtual void zeichnen(koord offset);
};

#endif