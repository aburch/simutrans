/*
 * simticker.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef simticker_h
#define simticker_h

#include "dataobj/koord.h"
#include "simcolor.h"
#include "tpl/slist_tpl.h"

/**
 * A very simple news ticker message storage class.
 * The news are displayed by karte_vollansicht_t
 *
 * @author Hj. Malthaner
 */
class ticker_t
{
public:
    struct node {
      char msg[256];
      koord pos;
      PLAYER_COLOR_VAL color;
      sint16 xpos;
      long w;
    };

private:
    static ticker_t * single_instance;

    ticker_t();

    slist_tpl<node> list;
    int next_pos;

    // true, if also trigger background need redraw
    bool redraw_all;

public:
    int count() const;

    /**
     * Add a message to the message list
     * @param pos    position of the event
     * @param color  message color 
     * @author Hj. Malthaner
     */
    void add_msg(const char*, koord pos, int color = COL_BLACK);


    static ticker_t * get_instance();

	/* Ticker infowin pops up
	 * @author Hj. Malthaner
	 */
	koord get_welt_pos();

    void zeichnen();
};

#endif
