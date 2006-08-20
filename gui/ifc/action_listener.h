/*
 * action_listener.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_ifc_action_listener_h
#define gui_ifc_action_listener_h

class gui_komponente_t;

/**
 * This interface must be implemented by all classes which want to
 * listen actions, i.e. button presses
 * @author Hj. Malthaner
 */
class action_listener_t
{
public:

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     *
     * Returns true, if action is done and no more
     * components should be triggered.
     * V.Meyer
     */
    virtual bool action_triggered(gui_komponente_t *komp) = 0;
};

#endif
