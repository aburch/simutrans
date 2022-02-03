/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_DEPOTLIST_FRAME_H
#define GUI_DEPOTLIST_FRAME_H


#include "gui_frame.h"
#include "simwin.h"
#include "components/gui_scrollpane.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_waytype_tab_panel.h"
#include "components/gui_label.h"
#include "components/gui_image.h"

class depot_t;


class depotlist_frame_t : public gui_frame_t, private action_listener_t
{
private:
	gui_scrolled_list_t scrolly;

	gui_waytype_tab_panel_t tabs;

	uint32 last_depot_count;

	void fill_list();

	player_t *player;

public:
	depotlist_frame_t();

	depotlist_frame_t(player_t* player);

	const char *get_help_filename() const OVERRIDE {return "depotlist.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	void map_rotate90( sint16 ) OVERRIDE { fill_list(); }

	void rdwr(loadsave_t* file) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_depotlist; }
};


class depotlist_stats_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	depot_t *depot;
	gui_label_buf_t label;
	gui_image_t waytype_symbol;
	button_t gotopos;

	void update_label();

public:
	depotlist_stats_t(depot_t *);

	void draw( scr_coord pos) OVERRIDE;

	char const* get_text() const  OVERRIDE { return ""; /* label.buf().get_str(); */ }
	bool infowin_event(const event_t *) OVERRIDE;
	bool is_valid() const OVERRIDE;
	void set_size(scr_size size) OVERRIDE;

	static bool compare(const gui_component_t *a, const gui_component_t *b );
};

#endif
