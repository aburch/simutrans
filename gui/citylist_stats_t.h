/*
 * citylist_stats_t.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef citylist_stats_t_h
#define citylist_stats_t_h

#include "../ifc/gui_komponente.h"

class karte_t;

/**
 * City list stats display
 * @author Hj. Malthaner
 */
class citylist_stats_t : public gui_komponente_t
{
 private:

  karte_t * welt;

 public:

  citylist_stats_t(karte_t *welt);


  /**
   * Events werden hiermit an die GUI-Komponenten
   * gemeldet
   * @author Hj. Malthaner
   */
  virtual void infowin_event(const event_t *);


  /**
   * Zeichnet die Komponente
   * @author Hj. Malthaner
   */
  void zeichnen(koord offset) const;
};

#endif // citylist_stats_t_h
