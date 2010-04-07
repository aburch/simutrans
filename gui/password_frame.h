/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_password_frame_h
#define gui_password_frame_t_h


#include "components/action_listener.h"
#include "gui_frame.h"
#include "gui_container.h"
#include "components/gui_textinput.h"
#include "components/gui_label.h"


class password_frame_t : public gui_frame_t, action_listener_t
{
private:
	char ibuf[1024];

protected:
	spieler_t *sp;

	gui_hidden_textinput_t input;
	gui_label_t fnlabel;        //filename                // 31-Oct-2001  Markus Weber    Added

public:
	/**
	 * @param suffix Filename suffix, i.e. ".sve", must be four characters
	 * @author Hj. Malthaner
	 */
	password_frame_t( spieler_t *sp );

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	virtual bool action_triggered( gui_action_creator_t *komp, value_t extra);

	// must catch open messgae to uptade list, since I am using virtual functions
//	virtual void infowin_event(const event_t *ev);
};

#endif
