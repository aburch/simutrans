/*
 * simticker.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef SIMTICKER_H
#define SIMTICKER_H

#include "dataobj/koord.h"
#include "simcolor.h"

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
	void zeichnen();
};

#endif
