/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef message_stats_t_h
#define message_stats_t_h

#include "components/gui_komponente.h"
#include "../display/simimg.h"
#include "../simmesg.h"
#include "../tpl/slist_tpl.h"


/**
 * City list stats display
 * @author Hj. Malthaner
 */
class message_stats_t : public gui_world_component_t
{
private:
	message_t *msg;
	sint32 message_type;								// Knightly : message type for filtering; -1 indicates no filtering
	uint32 last_count;
	sint32 message_selected;
	const slist_tpl<message_t::node *> *message_list;	// Knightly : points to the active message list (original or filtered)
	slist_tpl<message_t::node *> filtered_messages;		// Knightly : cache the list of messages belonging to a certain type

public:
	message_stats_t();
	~message_stats_t() { filtered_messages.clear(); }

	/**
	 * Filter messages by type
	 * @return whether there is a change in message filtering
	 * @author Knightly
	 */
	bool filter_messages(const sint32 msg_type);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Recalc the size required to display everything and set size.
	 */
	void recalc_size();

	/**
	 * Draw the component
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord offset);
};

#endif
