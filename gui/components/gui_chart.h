/*
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_chart_h
#define gui_chart_h

#include "../../simtypes.h"
#include "../../ifc/gui_komponente.h"
#include "../../tpl/slist_tpl.h"

// CURVE TYPES
#define STANDARD 0
#define MONEY 1

/**
 * Draws a group of curves.
 * @author Hendrik Siegeln
 */
class gui_chart_t : public gui_komponente_t
{
public:
	/**
	 * Set background color. -1 means no background
	 * @author Hj. Malthaner
	 */
	void set_background(int color);

	gui_chart_t();

	/*
	 * paint chart
	 * @author hsiegeln
	 */
		void zeichnen(koord offset);

	/**
	 * Events werden hiermit an die GUI-Komponenten
	 * gemeldet
	 * @author Hj. Malthaner
	 */
	void infowin_event(const event_t *);

	/*
	 * set dimension
	 * @author hsiegeln
	 */
	void set_dimension(int x, int y)
	{
		x_elements = x;
		y_elements = y;
	}

	/*
	 * adds a curve to the graph
	 * paramters:
	 * @color: color for this curve; default 0
	 * @values: reference to values
	 * @size: elements to skip before next valid entry (only usefull in multidimensional arrays)
	 * @offset: element to start with
	 * @elements: elements in values
	 * returns curve's id
	 * @author hsiegeln
	 */
	int add_curve(int color, sint64 *values, int size, int offset, int elements, int type, bool show, bool show_value);

	void remove_curves() { curves.clear(); }

	/**
	 * Hide a curve of the set
	 */
	void hide_curve(unsigned int id);


	/**
	 * Show a curve of the set
	 */
	void show_curve(unsigned int id);

	/*
	 * set starting value for x-axis of chart
	 * example: set_seed(1930) will make a graph starting in year 1930; use set_seed(-1) to display nothing
	 * @author hsiegeln
	 */
	void set_seed(int seed) { this->seed = seed; }

	void set_show_x_axis(bool yesno) { show_x_axis = yesno; }

	void set_show_y_axis(bool yesno) { show_y_axis = yesno; }

	int get_curve_count() { return curves.count(); }

private:

	void calc_gui_chart_values(sint64 *baseline, float *scale, char *, char *) const;

	/*
	 * curve struct
	 * @author hsiegeln
	 */
	struct curve_t {
		int color;
		sint64 *values;
		int size;
		int offset;
		int elements;
		bool show;
		bool show_value; // show first value of curve as number on chart?
		int type; // 0 = standard, 1 = money
	};

	slist_tpl <curve_t> curves;

	int x_elements, y_elements;

	int seed;

	koord tooltipkoord;

	bool show_x_axis, show_y_axis;

	/**
	 * Background color, -1 for transparent background
	 * @author Hj. Malthaner
	 */
	int background;
};

#endif
