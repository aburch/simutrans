/*
 * stadt_info.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef gui_stadt_info_h
#define gui_stadt_info_h


#include "gui_frame.h"
#include "components/gui_textinput.h"

class stadt_t;

/**
 * Dies stellt ein Fenster mit den Informationen
 * ueber eine Stadt dar.
 *
 * @author Hj. Malthaner
 */
class stadt_info_t : public gui_frame_t
{
private:
  stadt_t *stadt;

  gui_textinput_t name_input;

public:

  stadt_info_t(stadt_t *stadt);


  /**
   * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
   * das Fenster, d.h. es sind die Bildschirmkoordinaten des Fensters
   * in dem die Komponente dargestellt wird.
   */
  virtual void zeichnen(koord pos, koord gr);
};

#endif
