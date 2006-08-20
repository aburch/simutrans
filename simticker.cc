/*
 * simticker.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include <string.h>
#include <stdlib.h>

#include "simticker.h"
#include "simgraph.h"
#include "simcolor.h"

#include "tpl/slist_tpl.h"


ticker_t * ticker_t::single_instance= 0;


ticker_t * ticker_t::get_instance()
{
  if (single_instance == 0) {
    single_instance = new ticker_t();
  }
  return single_instance;
}


ticker_t::ticker_t()
{
  list = new slist_tpl <ticker_t::node>;
  iter = new slist_iterator_tpl <ticker_t::node> (list);

  next_pos = display_get_width();
}


ticker_t::~ticker_t()
{
  delete list;
  list = 0;

  delete iter;
  iter = 0;
}


int ticker_t::count() const
{
  return list->count();
}


const ticker_t::node * ticker_t::first()
{
  iter->begin(list);

  if(iter->next()) {
    return &(iter->get_current());
  } else {
    return 0;
  }
}


const ticker_t::node * ticker_t::next()
{
  if(iter->next()) {
    return &(iter->get_current());
  } else {
    return 0;
  }
}

void ticker_t::pop()
{
  if(list->count() > 0) {
    node p = list->at(0);
    list->remove_first();
    delete(const_cast<char *>(p.msg));
  }
}


/**
 * Add a message to the message list
 * @param pos  position of the event
 * @author Hj. Malthaner
 */
void ticker_t::add_msg(const char *txt, koord pos)
{
  add_msg(txt, pos, SCHWARZ);
}


/**
 * Add a message to the message list
 * @param pos    position of the event
 * @param color  message color 
 * @author Hj. Malthaner
 */
void ticker_t::add_msg(const char *txt, koord pos, int color)
{
  //    printf("Ticker: adding '%s' at %d\n", txt, next_pos);

  // don't store more than 4 messages, it's useless.

  const int count = list->count();

  if(count < 4) {
    // Don't repeat messages
    if(count == 0 || strncmp(txt, list->at(count-1).msg, strlen(txt)) != 0) {
      const int len = strlen(txt)+1;
      char * p = new char [len+3];  // reserve space for three more spaces
      strcpy(p, txt);
      strcat(p, "   ");   // add spaces

      node n;

      n.msg = p;
      n.pos = pos;
      n.color = color;
      n.xpos = next_pos;
      list->append(n);

      next_pos += proportional_string_width(n.msg) + 16;
    }
  }
}
