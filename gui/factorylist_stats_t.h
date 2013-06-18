/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Where factory stats are calculated for list dialog
 */

#ifndef factorylist_stats_t_h
#define factorylist_stats_t_h

#include "../tpl/vector_tpl.h"
#include "components/gui_komponente.h"

class karte_t;
class fabrik_t;


namespace factorylist {
    enum sort_mode_t { by_name=0, by_available, by_output, by_maxprod, by_status, by_power, SORT_MODES, by_input, by_transit,  };	// the last two not used
};

/**
 * Factory list stats display
 * @author Hj. Malthaner
 */
class factorylist_stats_t : public gui_komponente_t
{
private:
	karte_t *welt;
	vector_tpl<fabrik_t*> fab_list;
	uint32 line_selected;

	factorylist::sort_mode_t sortby;
	bool sortreverse;

public:
	factorylist_stats_t(karte_t* welt, factorylist::sort_mode_t sortby, bool sortreverse);

	void sort(factorylist::sort_mode_t sortby, bool sortreverse);

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
