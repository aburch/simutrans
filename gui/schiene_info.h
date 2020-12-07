/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SCHIENE_INFO_H
#define GUI_SCHIENE_INFO_H


#include "obj_info.h"
#include "../boden/wege/schiene.h"
#include "components/action_listener.h"
#include "components/gui_numberinput.h"
#include "components/gui_container.h"


/**
 * Info window for tracks
 */
class schiene_info_t : public obj_infowin_t, public action_listener_t
{
 private:
	schiene_t* sch;
	button_t reserving_vehicle_button;
	/**
	* Bound when this block was successfully reserved by the convoi
	*/
	convoihandle_t cnv;
protected:
	convoihandle_t reserved;


 public:
	schiene_info_t(schiene_t* const s);

	const char *get_help_filename() const OVERRIDE { return "track_info.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	virtual void draw(scr_coord pos, scr_size size) OVERRIDE;

	// called, after external change
	//void update_data();
};

#endif
