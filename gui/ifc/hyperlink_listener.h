/*
 * hyperlink_listener.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef hyperlink_listener_t_h
#define hyperlink_listener_t_h

#include "../../utils/cstring_t.h"

/**
 * Listener if a hyper link was activated
 * @author Hj. Malthaner
 */

class hyperlink_listener_t
{

 public:

  /**
   * Called upon link activation
   * @param the hyper ref of the link
   * @author Hj. Malthaner
   */
  virtual void hyperlink_activated(const cstring_t &txt) = 0;

};

#endif // hyperlink_listener_t_h
