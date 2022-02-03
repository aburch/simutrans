/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_LABEL_INFO_H
#define GUI_LABEL_INFO_H


#include "gui_frame.h"
#include "components/gui_textinput.h"
#include "components/gui_location_view.h"
#include "components/action_listener.h"

class label_t;

/**
 * Label creation/edition window
 */
class label_info_t : public gui_frame_t, private action_listener_t
{
private:
	label_t *label;

	gui_textinput_t input;
	location_view_t view;
	char edit_name[256];

public:
	label_info_t(label_t* l);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	// rotated map need new info ...
	void map_rotate90( sint16 ) OVERRIDE;
};

#endif
