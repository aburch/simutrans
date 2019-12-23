/**
 * Curiosity list window
 * @author Hj. Malthaner
 */

#ifndef curiositylist_frame_t_h
#define curiositylist_frame_t_h

#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_scrolled_list.h"


class curiositylist_frame_t : public gui_frame_t, private action_listener_t
{
private:
	button_t	sortedby;
	button_t	sorteddir;
	gui_scrolled_list_t scrolly;
	gui_aligned_container_t list;

	uint32 attraction_count;

	void fill_list();
public:
	curiositylist_frame_t();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author V. Meyer
	 */
	const char * get_help_filename() const OVERRIDE {return "curiositylist_filter.txt"; }

	bool has_min_sizer() const OVERRIDE {return true;}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void draw(scr_coord pos, scr_size size) OVERRIDE;
};

#endif
