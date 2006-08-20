/*
 * curiositylist_stats_t.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef curiositylist_stats_t_h
#define curiositylist_stats_t_h

#include "../tpl/vector_tpl.h"
#include "../ifc/gui_komponente.h"
#include "../dataobj/translator.h"
#include "../simgraph.h"
#include "../simcolor.h"
#include "button.h"
#include "../dings/gebaeude.h"

class karte_t;
class button_t;
class curiositylist_stats_t;

class curiositylist_header_t : public gui_komponente_t
{
 private:
	curiositylist_stats_t *stats;

 public:

  curiositylist_header_t(curiositylist_stats_t *s);
  ~curiositylist_header_t();


  /**
   * Zeichnet die Komponente
   * @author Hj. Malthaner
   */
  void zeichnen(koord offset) const;
};

/**
 * Curiosity list stats display
 * @author Hj. Malthaner
 */
class curiositylist_stats_t : public gui_komponente_t
{
 private:

	karte_t * welt;
	vector_tpl<gebaeude_t*> attractions;


 public:
	curiositylist_stats_t(karte_t *welt);
  	~curiositylist_stats_t();

  	void get_unique_attractions();


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

#endif // curiositylist_stats_t_h
