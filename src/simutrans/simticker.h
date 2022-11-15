/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMTICKER_H
#define SIMTICKER_H


#include "simcolor.h"
#include "display/simgraph.h"

#define TICKER_V_SPACE (2) // Vertical offset of ticker text
#define TICKER_HEIGHT  (LINESPACE+2*TICKER_V_SPACE)


class koord3d;
struct message_node_t;
/**
 * A very simple scrolling news ticker.
 */
namespace ticker
{
	bool empty();

	/**
	 * Add a message to the message list
	 * @param pos    position of the event
	 * @param color  message color
	 */
	void add_msg(const char*, koord3d pos, FLAGGED_PIXVAL color);

	/**
	 * Add a message in message_node_t format
	 */
	void add_msg_node(const message_node_t& msg);

	/**
	 * Remove all messages and mark for redraw
	 */
	void clear_messages();

	/**
	 * Update message positions and remove old messages
	 */
	void update();

	/**
	 * Redraw the ticker partially or fully (if set_redraw_all() was called)
	 */
	void draw();

	/**
	 * Set true if ticker has to be redrawn fully
	 * @sa redraw
	 */
	void set_redraw_all(bool redraw);

	/**
	 * Force a ticker redraw (e.g. after a window resize)
	 */
	void redraw();

	/**
	 * Process click into ticker bar: jump to coordinate or open message window.
	 */
	void process_click(int x);
};

#endif
