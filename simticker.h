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

// forward decl
template <class T> class slist_tpl;
template <class T> class slist_iterator_tpl;

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

    slist_tpl <node> * list;
    slist_iterator_tpl <node> * iter;
    int next_pos;

    // true, if also trigger background need redraw
    bool redraw_all;

public:

    ~ticker_t();

    int count() const;

    const node * first();
    const node * next();


    /**
     * Add a message to the message list
     * @param pos  position of the event
     * @author Hj. Malthaner
     */
    void add_msg(const char *, koord pos);


    /**
     * Add a message to the message list
     * @param pos    position of the event
     * @param color  message color 
     * @author Hj. Malthaner
     */
    void add_msg(const char *, koord pos, int color);


    void pop();

    void set_next_pos(int p) {next_pos = p;};
    void add_next_pos(int i) {next_pos += i;};


    static ticker_t * get_instance();

	/* Ticker infowin pops up
	 * @author Hj. Malthaner
	 */
	koord get_welt_pos(int x, int y);

    void zeichnen();
};

#endif
