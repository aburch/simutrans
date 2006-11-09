/*
 * stadt_info.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef message_frame_h
#define message_frame_h


#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_scrollpane.h"

#include "message_stats_t.h"
#include "../simmesg.h"
#include "components/action_listener.h"


/**
 * All messages since the start of the program
 * @author prissi
 */
class message_frame_t : public gui_frame_t, private action_listener_t
{
private:
	karte_t *welt;
	message_stats_t	stats;
	gui_scrollpane_t scrolly;
	button_t option_bt;

public:
  message_frame_t(karte_t * welt);

  /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    const char * gib_hilfe_datei() const {return "mailbox.txt";}

  /**
   * resize window in response to a resize event
   * @author Hj. Malthaner
   */
  void resize(const koord delta);

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     *
     * Returns true, if action is done and no more
     * components should be triggered.
     * V.Meyer
     */
    bool action_triggered(gui_komponente_t *komp, value_t extra);
};

#endif
