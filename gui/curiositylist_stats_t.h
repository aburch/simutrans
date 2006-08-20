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
//#include "../dings/gebaeude.h"
#include "button.h"

class karte_t;
class button_t;
class curiositylist_stats_t;
class gebaeude_t;

enum sort_mode_t { by_name=0, by_paxlevel, by_maillevel, SORT_MODES };

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
	curiositylist_stats_t(karte_t *welt,const sort_mode_t& sortby,const bool& sortreverse);
  	~curiositylist_stats_t();

  	void get_unique_attractions(const sort_mode_t& sortby,const bool& reverse);


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
