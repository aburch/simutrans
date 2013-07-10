/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/**
 * Where curiosity (attractions) stats are calculated for list dialog
 * @author Hj. Malthaner
 */

#ifndef curiositylist_stats_t_h
#define curiositylist_stats_t_h

#include "../tpl/vector_tpl.h"
#include "components/gui_komponente.h"
#include "components/gui_button.h"

class karte_t;
class gebaeude_t;

namespace curiositylist {
    enum sort_mode_t { by_name=0, by_paxlevel/*, by_maillevel*/, SORT_MODES };
};

class curiositylist_stats_t : public gui_komponente_t
{
private:
	karte_t * welt;
	vector_tpl<gebaeude_t*> attractions;
	uint32 line_selected;

	uint32 last_world_curiosities;
	curiositylist::sort_mode_t sortby;
	bool sortreverse;

public:
	curiositylist_stats_t(karte_t* welt, curiositylist::sort_mode_t sortby, bool sortreverse);

	void get_unique_attractions(curiositylist::sort_mode_t sortby, bool reverse);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	* Recalc the current size required to display everything, and set komponente groesse
	*/
	void recalc_size();

	/**
	* Draw the component
	* @author Hj. Malthaner
	*/
	void zeichnen(koord offset);
};

#endif
