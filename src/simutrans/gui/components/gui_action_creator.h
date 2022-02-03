/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_ACTION_CREATOR_H
#define GUI_COMPONENTS_GUI_ACTION_CREATOR_H


#include "action_listener.h"
#include "../../tpl/slist_tpl.h"


/**
 * This interface must be implemented by all classes which want to
 * send actions, i.e. button presses
 */
class gui_action_creator_t
{
protected:
	/**
	 * Our listeners.
	 */
	slist_tpl <action_listener_t *> listeners;

	/**
	 * Inform all listeners that an action was triggered.
	 */
	void call_listeners(value_t v)
	{
		for(action_listener_t* const l : listeners) {
			if (l->action_triggered(this, v)) break;
		}
	}

public:
	/**
	 * Add a new listener to this text input field.
	 */
	void add_listener(action_listener_t * l) { listeners.insert(l); }
};

#endif
