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
#include "components/gui_world_view_t.h"

class spieler_t;

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

	//Credit limit, right column
	//@author: jamespetts
	gui_label_t credit_limit;
	gui_label_t clamount;
	
	gui_label_t tylabel2; // this year, right column

	gui_label_t gtmoney; // balance (current)
	gui_label_t vtmoney;
	gui_label_t money;
	gui_label_t margin;
	gui_label_t transport, old_transport;
	gui_label_t powerline, old_powerline;

	gui_label_t maintenance_label;
	gui_label_t maintenance_money;

	gui_label_t warn;
	gui_label_t scenario;

	/**
	 * fills buffer (char array) with finance info
	 * @author Owen Rudge, Hj. Malthaner
	 */
	const char *display_money(int, char * buf, int);

	/**
	 * Returns the appropriate colour for a certain finance type
	 * @author Owen Rudge
	 */
	int get_money_colour(int type, int old);

	spieler_t *sp;

	//@author hsiegeln
	sint64 money_tmp, money_min, money_max;
	sint64 baseline, scale;
	char cmoney_min[128], cmoney_max[128];
	button_t filterButtons[MAX_PLAYER_COST];
	void calc_chart_values();
	static const char cost_type[MAX_PLAYER_COST][64];
	static const int cost_type_color[MAX_PLAYER_COST];
	static char digit[4];
	gui_tab_panel_t year_month_tabs;

	button_t headquarter, goto_headquarter;
	char headquarter_tooltip[1024];
	world_view_t headquarter_view;

	// last remembered HQ pos
	sint16 old_level;
	koord old_pos;

public:
	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const {return "finances.txt";}

	/**
	 * Konstruktor. Erzeugt alle notwendigen Subkomponenten.
	 * @author Hj. Malthaner, Owen Rudge
	 */
	money_frame_t(spieler_t *sp);

	/**
	 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	 * in dem die Komponente dargestellt wird.
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord pos, koord gr);

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered( gui_action_creator_t *komp, value_t extra);
};

#endif
