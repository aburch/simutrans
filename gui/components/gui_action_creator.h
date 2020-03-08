/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef gui_ifc_action_creator_h
#define gui_ifc_action_creator_h

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
		FOR(slist_tpl<action_listener_t*>, const l, listeners) {
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
