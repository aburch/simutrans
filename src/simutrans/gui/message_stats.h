/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_MESSAGE_STATS_H
#define GUI_MESSAGE_STATS_H


#include "components/gui_aligned_container.h"
#include "components/gui_scrolled_list.h"
#include "../simmesg.h"


/**
 * Message display
 */
class message_stats_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	const message_t::node *msg;
	uint32 sortid; // sortid reflects the order in message_t

public:
	message_stats_t(const message_t::node *m, uint32 sid);

	const message_t::node* get_msg() const { return msg; }

	char const* get_text() const OVERRIDE { return msg->msg; }

	bool infowin_event(const event_t * ev) OVERRIDE;

	static bool compare(const gui_component_t *a, const gui_component_t *b );
};

#endif
