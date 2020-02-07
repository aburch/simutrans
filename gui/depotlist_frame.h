/*
 * Factory list window
 * @author Hj. Malthaner
 */

#ifndef depotlist_frame_t_h
#define depotlist_frame_t_h

#include "gui_frame.h"
#include "components/gui_scrollpane.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_label.h"
#include "components/gui_image.h"

class depot_t;


class depotlist_frame_t : public gui_frame_t, private action_listener_t
{
private:
	button_t	sortedby;
	button_t	sorteddir;
	gui_scrolled_list_t scrolly;

	void fill_list();

	player_t *player;

public:
	depotlist_frame_t(player_t *player);

	const char *get_help_filename() const OVERRIDE {return "depotlist.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool has_min_sizer() const { return true; }
};

class depotlist_stats_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	depot_t *depot;
	gui_label_buf_t label;
	gui_image_t waytype_symbol;

	void update_label();

public:
	static int sort_mode;
	static bool reverse;

	depotlist_stats_t(depot_t *);

	void draw( scr_coord pos) OVERRIDE;

	char const* get_text() const { return ""; /* label.buf().get_str(); */ }
	bool infowin_event(const event_t *) OVERRIDE;
	bool is_valid() const OVERRIDE;
	void set_size(scr_size size) OVERRIDE;

	static bool compare(const gui_component_t *a, const gui_component_t *b );
};

#endif
