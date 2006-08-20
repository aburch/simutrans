/*
 * gui/ticker_view_t.cc
 *
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef ticker_view_t_h
#define ticker_view_t_h

#ifndef ifc_gui_fenster_h
#include "../ifc/gui_fenster.h"
#endif

class karte_t;

/**
 * View class for ticker tape messages
 * Data model: see "simticker.h/cc"
 * @author Hj. Malthaner
 */
class ticker_view_t : public gui_fenster_t
{
 private:

  karte_t * welt;

 public:

  ticker_view_t(karte_t * welt);


  /**
   * in top-level fenstern wird der Name in der Titelzeile dargestellt
   * @return den nicht uebersetzten Namen der Komponente
   * @author Hj. Malthaner
   */
  virtual const char * gib_name() const;


  /**
   * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
   * zurück
   * @author Hj. Malthaner
   */
  virtual fensterfarben gib_fensterfarben() const;


  /**
   * @return gibt wunschgroesse für das Darstellungsfenster zurueck
   * @author Hj. Malthaner
   */
  virtual koord gib_fenstergroesse() const;


  /**
   * Events werden hiermit an die GUI-Komponenten
   * gemeldet
   * @author Hj. Malthaner
   */
  virtual void infowin_event(const event_t *ev);


  /**
   * Fenster neu zeichnen.
   * @author Hj. Malthaner
   */
  virtual void zeichnen(koord pos, koord gr);

};

#endif // ticker_view_t_h
