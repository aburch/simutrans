/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef money_frame_h
#define money_frame_h

#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/gui_tab_panel.h"
#include "components/gui_chart.h"
#include "components/gui_location_view_t.h"
#include "components/gui_combobox.h"

#include "../player/finance.h"
#include "../player/simplay.h"

#define MAX_PLAYER_COST_BUTTON (13)

/**
 * Finances dialog
 *
 * @author Hj. Malthaner, Owen Rudge
 * @date 09-Jun-01
 * @update 29-Jun-02
 */
class money_frame_t : public gui_frame_t, private action_listener_t
{
private:
	char money_frame_title[80];

	gui_chart_t chart, mchart;

	gui_label_t tylabel; // this year
	gui_label_t lylabel; // last year

	gui_label_t conmoney;
	gui_label_t nvmoney;
	gui_label_t vrmoney;
	gui_label_t imoney;
	gui_label_t tmoney;
	gui_label_t mmoney;
	gui_label_t omoney;

	gui_label_t old_conmoney;
	gui_label_t old_nvmoney;
	gui_label_t old_vrmoney;
	gui_label_t old_imoney;
	gui_label_t old_tmoney;
	gui_label_t old_mmoney;
	gui_label_t old_omoney;

	gui_label_t tylabel2; // this year, right column

	gui_label_t gtmoney; // balance (current)
	gui_label_t vtmoney;
	gui_label_t money;
	gui_label_t margin;
	gui_label_t transport, old_transport;
	gui_label_t toll, old_toll;

	gui_label_t maintenance_label;
	gui_label_t maintenance_money;

	gui_label_t warn;
	gui_label_t scenario;

	gui_container_t month_dummy, year_dummy;

	int transport_types[TT_MAX];
	int transport_type_option;
	gui_combobox_t transport_type_c;

	/// Helper method to update number label text and color
	void update_label(gui_label_t &label, char *buf, int transport_type, uint8 type, int yearmonth, int label_type = MONEY);

	spieler_t *sp;

	//@author hsiegeln
	sint64 money_tmp, money_min, money_max;
	sint64 baseline, scale;
	char cmoney_min[128], cmoney_max[128];
	button_t filterButtons[MAX_PLAYER_COST_BUTTON];
	void calc_chart_values();
	static const char *cost_type_name[MAX_PLAYER_COST_BUTTON];
	static const COLOR_VAL cost_type_color[MAX_PLAYER_COST_BUTTON];
	static const uint8 cost_type[3*MAX_PLAYER_COST_BUTTON];
	static const char * transport_type_values[TT_MAX];
	gui_tab_panel_t year_month_tabs;

	button_t headquarter, goto_headquarter;
	char headquarter_tooltip[1024];
	location_view_t headquarter_view;

	// last remembered HQ pos
	sint16 old_level;
	koord old_pos;

	/// Helper method to query data from players statistics
	sint64 get_statistics_value(int transport_type, uint8 type, int yearmonth, bool monthly);

	sint64 chart_table_month[MAX_PLAYER_HISTORY_MONTHS][MAX_PLAYER_COST_BUTTON];
	sint64 chart_table_year[ MAX_PLAYER_HISTORY_YEARS ][MAX_PLAYER_COST_BUTTON];

	void fill_chart_tables();

	bool is_chart_table_zero(int ttoption);


public:
	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const {return "finances.txt";}

	/**
	 * Constructor. Adds all necessary Subcomponents.
	 * @author Hj. Malthaner, Owen Rudge
	 */
	money_frame_t(spieler_t *sp);

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord pos, scr_size size);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	// saving/restore stuff
	uint32 get_rdwr_id();

	void rdwr( loadsave_t *file );
};

#endif
