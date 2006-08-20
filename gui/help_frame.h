/*
 * help_frame.h
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_help_frame_h
#define gui_help_frame_h

#include "gui_frame.h"
#include "components/gui_scrollpane.h"
#include "components/gui_flowtext.h"
#include "components/action_listener.h"

class help_frame_t : public gui_frame_t, action_listener_t
{
private:
    gui_scrollpane_t scrolly;
    gui_flowtext_t flow;

public:
    void setze_text(const char * text);

    help_frame_t();
    help_frame_t(cstring_t filename);

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
