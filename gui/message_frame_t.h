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
#include "button.h"

#include "message_frame_t.h"
#include "message_stats_t.h"
#include "../simmesg.h"
#include "ifc/action_listener.h"

class gui_scrollpane_t;

/**
 * All messages since the start of the program
 * @author prissi
 */
class message_frame_t : public gui_frame_t, private action_listener_t
{
private:
	karte_t *welt;
	gui_scrollpane_t *scrolly;
	button_t *option_bt;
	message_stats_t	*stats;

public:
  message_frame_t(karte_t * welt);
  ~message_frame_t();

  /* triggers action, when button clicked */
  virtual bool message_frame_t::action_triggered(gui_komponente_t *komp);

  /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const {return "mailbox.txt";};

  /**
   * resize window in response to a resize event
   * @author Hj. Malthaner
   */
  virtual void resize(const koord delta);
};

#endif //message_frame_h
