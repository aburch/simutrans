/*
 * kennfarbe.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_kennfarbe_h
#define gui_kennfarbe_h

#include "gui_frame.h"
#include "components/gui_textarea.h"

/**
 * Hierueber kann der Spieler seine Kennfarbe einstellen
 *
 * @author Hj. Malthaner
 */
class farbengui_t : public gui_frame_t
{
private:
	spieler_t *sp;
	gui_textarea_t txt;

public:
    farbengui_t(spieler_t *sp);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const { return "color.txt"; }

    void infowin_event(const event_t *ev);

  /**
   * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
   * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
   * in dem die Komponente dargestellt wird.
   *
   * @author Hj. Malthaner
   */
  void zeichnen(koord pos, koord gr);
};

#endif
