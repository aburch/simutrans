/*
 * Copyright (c) 2010 Bernd Gabriel
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_component_table_h
#define gui_component_table_h

#include "gui_table.h"


/**
 * 
 *
 * @since 22-MAR-2010
 * @author Bernd Gabriel
 */
class gui_table_cell_list_t : public list_tpl<gui_komponente_t> {
protected:
	virtual gui_komponente_t *create_item() const { 
		// cannot create a default component, as gui_komponente_t is an abstract class.
		return NULL;
	}
};

/**
 * 
 *
 * @since 22-MAR-2010
 * @author Bernd Gabriel
 */
class gui_table_cell_matrix_t : public list_tpl<gui_table_cell_list_t> {
protected:
	virtual gui_table_cell_list_t *create_item() const { return new gui_table_cell_list_t(); }
public:
	/**
	 * operator[] gets a reference to the item at position 'index'.
	 */
	gui_table_cell_list_t& operator [] (uint32 index) const { return *get(index); }
};


/**
 * a table component
 *
 * @since 14-MAR-2010
 * @author Bernd Gabriel
 */
class gui_component_table_t : public gui_table_t
{
private:
	bool owns_cell_components;
	gui_table_cell_matrix_t gui_cells; // gui_cells[x][y]
protected:

	/**
	 * init_cell() is called in change_size(), whenever a cell is added, e.g. during set_size().
	 *
	 * It has to initialize the content of this cell. 
	 * The default implementation does nothing for cell(x,y) held in gui_cells[x][y].
	 * The position of a cell is relative to its position in the grid. 
	 * Position (0,0) shows the cell starting in the upper left corner of the cell.
	 * The cell must not be wider than get_column_width() and not higher than get_row_height().
	 * There is no clipping. If you'd like to merge cells, you resign a grid or override paint_grid().
	 */
	virtual void init_cell(coordinate_t x, coordinate_t y);

	/**
	 * paint_cell() is called in zeichnen(), whenever a cell has to be painted.
	 *
	 * It has to paint cell (x,y) at position offset. 
	 * The default implementation calls zeichnen() of the component of cell (x,y), if there is one.
	 */
	virtual void paint_cell(const koord &offset, coordinate_t x, coordinate_t y);

	/**
	 * remove_cell() is called in change_size(), before a cell is removed, e.g. during set_size().
	 * It has to finalize the content of this cell. 
	 * The default implementation deletes the component of cell (x,y), if there is one.
	 */
	virtual void remove_cell(coordinate_t x, coordinate_t y);

	/* overridden inherited methods */
	virtual void change_size(const coordinates_t &old_size, const coordinates_t &new_size);
public:
	//gui_component_table_t();
	//~gui_component_table_t();

	bool get_owns_cell_components() { return owns_cell_components; }
	void set_owns_cell_components(bool value) { owns_cell_components = value; }

	gui_komponente_t *get_cell_component(coordinate_t x, coordinate_t y);
	void set_cell_component(coordinate_t x, coordinate_t y, gui_komponente_t *component);

	virtual coordinate_t add_column(gui_table_column_t *column);
	virtual coordinate_t add_row(gui_table_row_t *row);
	virtual void remove_column(coordinate_t x);
	virtual void remove_row(coordinate_t y);

	virtual bool infowin_event(const event_t *ev);
};

#endif
