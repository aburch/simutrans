/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_BUILDING_INFO_H
#define GUI_BUILDING_INFO_H


#include "base_info.h"
#include "../obj/gebaeude.h"

#include "components/gui_label.h"
#include "components/gui_location_view_t.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"

#include "../utils/cbuffer_t.h"
#include "../descriptor/goods_desc.h"

class gebaeude_t;
class player_t;

class gui_building_stats_t : public gui_aligned_container_t
{
	const gebaeude_t *building;
	PIXVAL frame_color;
	gui_label_buf_t p_class_names[5];
	gui_label_buf_t lb_visitor_arrivals[2], lb_commuter_arrivals[2], lb_mail_sent[2]; // for trip record
	gui_label_buf_t lb_commuting_success_rate[2], lb_visiting_success_rate[2], lb_mail_success_rate[2]; // for achievement rate


public:
	gui_building_stats_t(const gebaeude_t* gb, PIXVAL col);

	void init(const gebaeude_t* gb, PIXVAL col);

	void init_class_table();
	void init_stats_table();

	void update_stats();

	void draw(scr_coord offset) OVERRIDE;
};

class building_info_t : public base_infowin_t
{
	cbuffer_t building_tooltip;
	const gebaeude_t *building;
	player_t *owner;
	location_view_t building_view;
	gui_aligned_container_t cont_near_by_halt, cont_signalbox_info, signal_table;
	gui_building_stats_t cont_stats;
	gui_scrollpane_t scrolly_stats, scrolly_near_by_halt, scrolly_signalbox;
	gui_tab_panel_t tabs;
	gui_label_buf_t lb_signals;

	void update_near_by_halt();
	void update_signalbox_info();
public:
	building_info_t(gebaeude_t* gb, player_t* owner);

	const char *get_help_filename() const OVERRIDE { return "building_info.txt"; }

	koord3d get_weltpos(bool) OVERRIDE { return building->get_pos(); }

	bool is_weltpos() OVERRIDE;

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	void map_rotate90(sint16) OVERRIDE;
};

#endif
