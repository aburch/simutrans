/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMINTERACTION_H
#define SIMINTERACTION_H


class karte_ptr_t;
class viewport_t;
struct event_t;


/**
 * User interaction class, it collects and processes all user and system events from the OS.
 * @brief Event manager of the game.
 * @see karte_t::interactive
 */
class interaction_t
{
private:
	/**
	 * The associated world.
	 */
	static karte_ptr_t world;

	/**
	 * Associated viewport of the world, this is a cached value.
	 * @note Since a world in our project just has one viewport, we cache its value.
	 */
	viewport_t *viewport;

	bool is_dragging;

private:
	/**
	 * Processes a mouse event that's moving the camera.
	 */
	void move_view(const event_t &ev);

	/**
	 * Processes a cursor movement event, related to the tool pointer in-map.
	 * @see zeiger_t
	 */
	void move_cursor(const event_t &ev);

	/**
	 * Processes a user event on the map, like a keyclick, or a mouse event.
	 */
	void interactive_event(const event_t &ev);

	/**
	 * Processes a single event.
	 * @return true if we need to stop processing events.
	 */
	bool process_event(event_t &ev);

public:
	/**
	 * Processes all the pending system events.
	 * @brief Message Pump
	 */
	void check_events();

	explicit interaction_t(viewport_t *viewport);
};

#endif
