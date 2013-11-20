/*
 * Copyright (c) 2011 Knightly
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Where the current factory chart statistics are calculated
 */

#ifndef factory_chart_h
#define factory_chart_h

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

	// Tab panel for grouping 2 sets of statistics
	gui_tab_panel_t tab_panel;

	// GUI components for input/output goods' statistics
	gui_container_t goods_cont;
	gui_chart_t goods_chart;
	button_t *goods_buttons;
	gui_label_t *goods_labels;
	uint16 goods_button_count;
	uint16 goods_label_count;

	// GUI components for other production-related statistics
	gui_container_t prod_cont;
	gui_chart_t prod_chart;
	button_t prod_buttons[MAX_FAB_STAT];
	button_t prod_ref_line_buttons[MAX_FAB_REF_LINE];
	gui_label_t prod_labels[MAX_PROD_LABEL];

	// Variables for reference lines
	sint64 prod_ref_line_data[MAX_FAB_REF_LINE];

public:
	factory_chart_t(const fabrik_t *_factory);
	virtual ~factory_chart_t();

	void set_factory(const fabrik_t *_factory);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	virtual void draw(scr_coord pos);

	void rdwr( loadsave_t *file );
};

#endif
