/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_FACTORY_CHART_H
#define GUI_FACTORY_CHART_H


#define MAX_PROD_LABEL      (7-1)

#include "../simfab.h"
#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_label.h"
#include "components/gui_button.h"
#include "components/gui_button_to_chart.h"
#include "components/gui_chart.h"
#include "components/gui_tab_panel.h"
#include "components/gui_aligned_container.h"
#include "components/gui_button_to_chart.h"

class factory_chart_t : public gui_aligned_container_t
{
private:
	const fabrik_t *factory;

	// Tab panel for grouping 2 sets of statistics
	gui_tab_panel_t tab_panel;

	// GUI components for input/output goods' statistics
	gui_aligned_container_t goods_cont;
	gui_chart_t goods_chart;

	// GUI components for other production-related statistics
	gui_aligned_container_t prod_cont;
	gui_chart_t prod_chart;

	// Variables for reference lines
	sint64 prod_ref_line_data[MAX_FAB_REF_LINE];

	gui_button_to_chart_array_t button_to_chart;

public:
	factory_chart_t(const fabrik_t *_factory);
	virtual ~factory_chart_t();

	void set_factory(const fabrik_t *_factory);

	void update();

	void rdwr( loadsave_t *file );

	/**
	 * factory window will take our tabs,
	 * we only initialize them and update charts
	 */
	gui_tab_panel_t* get_tab_panel() { return &tab_panel; }
};

#endif
