/*
 * citylist_stats_t.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef message_stats_t_h
#define message_stats_t_h

#include "../ifc/gui_komponente.h"
#include "../simmesg.h"
#include "../simimg.h"

class karte_t;
class message_t;

/**
 * City list stats display
 * @author Hj. Malthaner
 */
class message_stats_t : public gui_komponente_t
{
private:
	message_t *msg;
	karte_t *welt;
	unsigned	last_count;

public:
	message_stats_t(karte_t *welt);

  /**
   * Events werden hiermit an die GUI-Komponenten
   * gemeldet
   * @author Hj. Malthaner
   */
  void infowin_event(const event_t *);

  /**
   * Zeichnet die Komponente
   * @author Hj. Malthaner
   */
  void zeichnen(koord offset) const;
};

#endif // message_stats_n
