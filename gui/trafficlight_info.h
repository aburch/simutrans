/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Traffic light phase buttons
 */

#ifndef trafficlight_info_t_h
#define trafficlight_info_t_h

#include "thing_info.h"
#include "components/action_listener.h"
#include "components/gui_numberinput.h"
#include "gui_container.h"

class roadsign_t;

/**
 * Info window for factories
 * @author Hj. Malthaner
 */
class trafficlight_info_t : public ding_infowin_t, public action_listener_t
{
 private:
	roadsign_t* ampel;
	gui_numberinput_t ns, ow, offset;

 public:
	trafficlight_info_t(roadsign_t* s);

	/*
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char *get_hilfe_datei() const {return "trafficlight_info.txt";}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	// called, after external change
	void update_data();
};

#endif
