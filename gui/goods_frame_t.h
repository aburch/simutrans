/*
 * goods_frame_t.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef goods_frame_t_h
#define goods_frame_t_h

#include "gui_frame.h"

class gui_scrollpane_t;

/**
 * Shows statistics. Only goods so far.
 * @author Hj. Malthaner
 */
class goods_frame_t : public gui_frame_t
{
 private:

  gui_scrollpane_t *scrolly;

 public:

  goods_frame_t(karte_t *welt);
  ~goods_frame_t();



  /**
   * resize window in response to a resize event
   * @author Hj. Malthaner
   * @date   16-Oct-2003
   */
  virtual void resize(const koord delta);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author V. Meyer
     */
    virtual const char * gib_hilfe_datei() const {return "goods_filter.txt"; }
};

#endif // goods_frame_t_h
