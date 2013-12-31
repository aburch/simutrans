/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef SIMTICKER_H
#define SIMTICKER_H

#include "simcolor.h"

// ticker height
#define TICKER_HEIGHT      15
// ticker vertical position from bottom of screen
#define TICKER_YPOS_BOTTOM 32

class koord;

/**
 * A very simple news ticker.
 * The news are displayed by karte_vollansicht_t
 */
namespace ticker
{
	bool empty();

	/**
	 * Add a message to the message list
	 * @param pos    position of the event
	 * @param color  message color 
	 */
	void add_msg(const char*, koord pos, int color = COL_BLACK);

	/**
	 * Ticker infowin pops up
	 */
	koord get_welt_pos();

	/**
	 * Ticker redraw
	 */
	void draw();

	/**
	 * Set true if ticker has to be redrawn
	 */
	void set_redraw_all(const bool);

	/**
	 * Ticker text redraw after resize
	 */
	void redraw_ticker();

	void clear_ticker();
};

#endif
