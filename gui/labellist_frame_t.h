/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_LABELLIST_FRAME_T_H
#define GUI_LABELLIST_FRAME_T_H


#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_combobox.h"


/**
 * label list window
 * @author Hj. Malthaner
 */
class labellist_frame_t : public gui_frame_t, private action_listener_t
{
private:
	gui_combobox_t	sortedby;
	button_t	sort_asc, sort_desc;
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
