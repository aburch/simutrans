/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Track infowindow buttons //Ves
 */

#ifndef schiene_info_t_h
#define schiene_info_t_h

#include "obj_info.h"
#include "../boden/wege/schiene.h"
#include "components/action_listener.h"
#include "components/gui_numberinput.h"
#include "components/gui_container.h"

/**
 * Info window for tracks
 * @author Hj. Malthaner
 */
class schiene_info_t : public obj_infowin_t, public action_listener_t
{
 private:
	schiene_t* sch;
	button_t reserving_vehicle_button;
	/**
	* Bound when this block was successfully reserved by the convoi
	* @author prissi
	*/
	convoihandle_t cnv;
protected:
	convoihandle_t reserved;


 public:
	schiene_info_t(schiene_t* const s);
	
	/*
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char *get_help_filename() const { return "track_info.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	virtual void draw(scr_coord pos, scr_size size);

	// called, after external change
	//void update_data();
};

#endif
