/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_BUTTON_TO_CHART_H
#define GUI_COMPONENTS_GUI_BUTTON_TO_CHART_H


#include "action_listener.h"
#include "gui_button.h"
#include "gui_chart.h"
#include "../../tpl/vector_tpl.h"

class loadsave_t;

/**
 * Class that handles button -> curve actions.
 */
class gui_button_to_chart_t : public action_listener_t
{
	button_t *button;
	gui_chart_t *chart;
	int curve;

public:
	gui_button_to_chart_t(button_t *b, gui_chart_t *c, int i) : button(b), chart(c), curve(i)
	{
		b->add_listener(this);
	}

	bool action_triggered(gui_action_creator_t *comp, value_t) OVERRIDE
	{
		if (comp == button) {
			update();
		}
		// return false, the same button can be associated to more than one chart
		return false;
	}

	void update() const
	{
		if (button->pressed) {
			chart->show_curve(curve);
		}
		else {
			chart->hide_curve(curve);
		}
	}

	button_t* get_button() const   { return button; }
	gui_chart_t* get_chart() const { return chart; }
	int get_curve() const          { return curve; }
};

/**
 * Saves button-chart connections.
 */
class gui_button_to_chart_array_t
{
	vector_tpl<gui_button_to_chart_t*> array;
public:
	~gui_button_to_chart_array_t()
	{
		clear();
	}

	void clear()
	{
		clear_ptr_vector(array);
	}

	void append(button_t *b, gui_chart_t *c, int i)
	{
		array.append(new gui_button_to_chart_t(b, c, i) );
	}

	const vector_tpl<gui_button_to_chart_t*> list() const { return array; }

	/**
	 * Load & save for array of gui_button_to_chart_t.
	 * Assumes array is filled and has always the same order of elements.
	 */
	void rdwr(loadsave_t *file);

	gui_button_to_chart_t* operator [](uint i) { return array[i]; }
};

#endif
