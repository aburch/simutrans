/*
 * fabrik_info.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef fabrikinfo_t_h
#define fabrikinfo_t_h

#include "gui_frame.h"
#include "thing_info.h"
#include "world_view_t.h"

#include "ifc/action_listener.h"

#include "gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "gui_container.h"
#include "../utils/cbuffer_t.h"

class fabrik_t;
class gebaeude_t;
class karte_t;

class button_t;

/**
 * Info window for factories
 * @author Hj. Malthaner
 */
class fabrik_info_t : public gui_frame_t, public ding_info_t, action_listener_t
{
 private:

  world_view_t view;
  fabrik_t * fab;
  karte_t  * welt;

  button_t * lieferbuttons;
  button_t * supplierbuttons;
  button_t * stadtbuttons;

  button_t * about;

  gui_scrollpane_t scrolly;
  gui_container_t cont;
  gui_textarea_t txt;
  cbuffer_t info_buf;

 public:

  fabrik_info_t(fabrik_t *fab, gebaeude_t *gb, karte_t *welt);


  ~fabrik_info_t();


  /*
   * Für den Aufruf der richtigen Methoden sorgen!
   */
  virtual const char * gib_name() const;


  /**
   * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
   * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
   * in dem die Komponente dargestellt wird.
   *
   * @author Hj. Malthaner
   */
  void zeichnen(koord pos, koord gr);


  /**
   * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
   * zurück
   */
  fensterfarben gib_fensterfarben() const;


  /**
   * This method is called if an action is triggered
   * @author Hj. Malthaner
   *
   * Returns true, if action is done and no more
   * components should be triggered.
   * V.Meyer
   */
  bool action_triggered(gui_komponente_t *komp);

};

#endif // fabrikinfo_t_h
