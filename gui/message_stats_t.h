/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_MESSAGE_STATS_T_H
#define GUI_MESSAGE_STATS_T_H


#include "components/gui_component.h"
#include "../simmesg.h"
#include "../tpl/slist_tpl.h"


/**
 * City list stats display
 */
class message_stats_t : public gui_world_component_t
{
private:
	message_t *msg;
	sint32 message_type;								// message type for filtering; -1 indicates no filtering
	uint32 last_count;
	sint32 message_selected;
	const slist_tpl<message_t::node *> *message_list;	// points to the active message list (original or filtered)
	slist_tpl<message_t::node *> filtered_messages;		// cache the list of messages belonging to a certain type

	scr_size min_size;

public:
	message_stats_t();
	~message_stats_t() { filtered_messages.clear(); }

	/**
	 * Filter messages by type
	 * @return whether there is a change in message filtering
	 */
	bool filter_messages(const sint32 msg_type);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Recalc the size required to display everything and set size and min_size.
	 */
	void recalc_size();

	/**
	 * Draw the component
	 */
	void draw(scr_coord offset) OVERRIDE;

	scr_size get_max_size() const OVERRIDE {
		return get_min_size();
	}

	scr_size get_min_size() const {
		return min_size;
	}
};

#endif
