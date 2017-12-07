/*
 * Copyright (c) 2013 The Simutrans Community
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef siminteraction_h
#define siminteraction_h

class karte_ptr_t;
class viewport_t;
struct event_t;

/**
 * User interaction class, it collects and processes all user and system events from the OS.
 * @brief Event manager of the game.
 * @see world_t::interactive
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
	bool process_event( event_t &ev );

	bool is_dragging;

public:
	/**
	 * Processes all the pending system events.
	 * @brief Message Pump
	 */
	void check_events();

	/**
	 * @note Requires world() with it's viewport already attached!
	 */
	interaction_t();
};

#endif
