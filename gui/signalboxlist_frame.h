/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SIGNALBOXLIST_FRAME_H
#define GUI_SIGNALBOXLIST_FRAME_H


#include "gui_frame.h"
#include "components/gui_scrollpane.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_label.h"
#include "components/gui_image.h"
#include "components/gui_combobox.h"

class signalbox_t;


class signalboxlist_frame_t : public gui_frame_t, private action_listener_t
{
private:
	gui_combobox_t sortedby;
	button_t sort_asc, sort_desc;
	gui_scrolled_list_t scrolly;

	uint32 last_signalbox_count;

	void fill_list();

	player_t *player;

public:
	signalboxlist_frame_t(player_t *player);

	const char *get_help_filename() const OVERRIDE {return "signalboxlist.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool has_min_sizer() const { return true; }
};

class signalboxlist_stats_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	signalbox_t *sb;
	gui_label_buf_t label, lb_connected, lb_region;
	button_t	gotopos;

	void update_label();

public:
	static int sort_mode;
	static bool reverse;

	signalboxlist_stats_t(signalbox_t *);

	void draw( scr_coord pos) OVERRIDE;

	char const* get_text() const { return ""; /* label.buf().get_str(); */ }
	bool infowin_event(const event_t *) OVERRIDE;
	bool is_valid() const OVERRIDE;
	void set_size(scr_size size) OVERRIDE;

	static bool compare(const gui_component_t *a, const gui_component_t *b );
};

#endif
