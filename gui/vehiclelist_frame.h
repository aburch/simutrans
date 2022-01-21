/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_VEHICLELIST_FRAME_H
#define GUI_VEHICLELIST_FRAME_H


#include "simwin.h"
#include "gui_frame.h"
#include "components/gui_scrollpane.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_label.h"
#include "components/gui_image.h"
#include "components/gui_waytype_tab_panel.h"
#include "components/gui_combobox.h"

class vehicle_desc_t;
class goods_desc_t;


class vehiclelist_frame_t : public gui_frame_t, private action_listener_t
{
private:
	button_t sorteddir, bt_obsolete, bt_future;
	gui_scrolled_list_t scrolly;
	gui_waytype_tab_panel_t tabs;
	gui_combobox_t sort_by, ware_filter;
	vector_tpl<const goods_desc_t *>idx_to_ware;

	void fill_list();

//	waytype_t current_wt;

public:
	vehiclelist_frame_t();

	const char *get_help_filename() const OVERRIDE {return "vehiclelist.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void rdwr(loadsave_t* file) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_vehiclelist; }
};


class vehiclelist_stats_t : public gui_scrolled_list_t::scrollitem_t
{
private:
	const vehicle_desc_t *veh;
	cbuffer_t part1, part2;
	int name_width;
	int col1_width;
	int col2_width;
	int height;

public:
	static int sort_mode;
	static bool reverse;
	static int img_width;

	vehiclelist_stats_t(const vehicle_desc_t *);

	char const* get_text() const OVERRIDE;
	scr_size get_size() const OVERRIDE { return scr_size( D_MARGIN_LEFT+img_width+max(col1_width+col2_width,name_width)+D_MARGIN_RIGHT, height ); }
	scr_size get_min_size() const OVERRIDE { return scr_size( D_MARGIN_LEFT+img_width+max(col1_width+col2_width,name_width)+D_MARGIN_RIGHT, height ); }
	scr_size get_max_size() const OVERRIDE { return scr_size( D_MARGIN_LEFT+img_width+max(col1_width+col2_width,name_width)+D_MARGIN_RIGHT, height ); }

	static bool compare(const gui_component_t *a, const gui_component_t *b );

	void draw( scr_coord offset ) OVERRIDE;
};

#endif
