/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef siminteraction_h
#define siminteraction_h

class karte_t;
struct event_t;

/**
 * User interaction class, it collects and processes all user and system events from the OS.
 * @brief Event manager of the game.
 * @see karte_t::interactive
 */
class interaction_t
{
private:
	karte_t *world;

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

	interaction_t(karte_t *welt);
};

#endif
