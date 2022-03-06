/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_TRAFFICLIGHT_INFO_H
#define GUI_TRAFFICLIGHT_INFO_H


#include "obj_info.h"
#include "components/action_listener.h"
#include "components/gui_numberinput.h"
#include "components/gui_container.h"

class roadsign_t;

/**
 * Traffic light phase buttons
 */
class trafficlight_info_t : public obj_infowin_t, public action_listener_t
{
private:
	roadsign_t* roadsign;
	gui_numberinput_t ns, ow, offset, yellow_ns, yellow_ow;

public:
	trafficlight_info_t(roadsign_t* s);

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char *get_help_filename() const OVERRIDE {return "trafficlight_info.txt";}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	// called, after external change
	void update_data();
};

#endif
