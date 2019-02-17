#ifndef labellist_frame_t_h
#define labellist_frame_t_h

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_scrolled_list.h"


/**
 * label list window
 * @author Hj. Malthaner
 */
class labellist_frame_t : public gui_frame_t, private action_listener_t
{
private:
	button_t	sortedby;
	button_t	sorteddir;
	button_t	filter;

	gui_scrolled_list_t scrolly;

public:
	labellist_frame_t();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author V. Meyer
	 */
	const char * get_help_filename() const {return "labellist_filter.txt"; }


	bool action_triggered( gui_action_creator_t *komp,value_t /* */);

	void draw(scr_coord pos, scr_size size);

	/**
	 * This function refreshes the list
	 * @author Markus Weber
	 */
	void fill_list();
};

#endif
