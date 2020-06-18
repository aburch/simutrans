/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
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
#include "components/gui_chart.h"
#include "components/gui_tab_panel.h"
#include "components/gui_container.h"


class factory_chart_t : public gui_container_t, private action_listener_t
{
private:
	const fabrik_t *factory;

	// GUI components for other production-related statistics
	gui_chart_t prod_chart;
	button_t prod_buttons[MAX_FAB_STAT];
	button_t prod_ref_line_buttons[MAX_FAB_REF_LINE];
	gui_label_t prod_labels[MAX_PROD_LABEL];

	// Variables for reference lines
	sint64 prod_ref_line_data[MAX_FAB_REF_LINE];

public:
	factory_chart_t(const fabrik_t *_factory);

	void set_factory(const fabrik_t *_factory);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	virtual void draw(scr_coord pos) OVERRIDE;

	void recalc_size();

	void rdwr( loadsave_t *file );
};


class factory_goods_chart_t : public gui_container_t, private action_listener_t
{
private:
	const fabrik_t *factory;

	// GUI components for input/output goods' statistics
	gui_chart_t goods_chart;
	button_t *goods_buttons;
	gui_label_t *goods_labels;
	gui_label_t lbl_consumption,  lbl_production;
	uint16 goods_button_count;
	uint16 goods_label_count;

	// Optimize goods color to color for chart
	COLOR_VAL goods_col_to_chart_col(COLOR_VAL col) const;

public:
	factory_goods_chart_t(const fabrik_t *_factory);
	virtual ~factory_goods_chart_t();

	void set_factory(const fabrik_t *_factory);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	virtual void draw(scr_coord pos) OVERRIDE;

	void recalc_size();

	void rdwr(loadsave_t *file);
};

#endif
