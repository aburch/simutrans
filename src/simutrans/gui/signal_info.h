/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SIGNAL_INFO_H
#define GUI_SIGNAL_INFO_H


#include "../simconst.h"
#include "obj_info.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_container.h"

class signal_t;

/**
 * Info window for factories
 */
class signal_info_t : public obj_infowin_t, public action_listener_t
{
 private:
	button_t remove;
	signal_t *sig;

 public:
	signal_info_t(signal_t *s);

	const char *get_help_filename() const OVERRIDE {return "signal_info.txt";}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void draw( scr_coord pos, scr_size size ) OVERRIDE;
};

#endif
