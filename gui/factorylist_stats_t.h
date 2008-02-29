/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef factorylist_stats_t_h
#define factorylist_stats_t_h

#include "../tpl/vector_tpl.h"
#include "../ifc/gui_komponente.h"
#include "../simcolor.h"
#include "components/gui_button.h"
#include "fabrik_info.h"

class karte_t;
class fabrik_t;


namespace factorylist {
    enum sort_mode_t { by_name=0, by_input, by_output, by_maxprod, by_status, by_power, SORT_MODES };
};

/**
 * Factory list stats display
 * @author Hj. Malthaner
 */
class factorylist_stats_t : public gui_komponente_t
{
private:

	karte_t * welt;
	vector_tpl<fabrik_t*> fab_list;
	uint32 line_selected;

public:
	factorylist_stats_t(karte_t* welt, factorylist::sort_mode_t sortby, bool sortreverse);

	/**
	* Events werden hiermit an die GUI-Komponenten
	* gemeldet
	* @author Hj. Malthaner
	*/
	void infowin_event(const event_t *);

	void sort(factorylist::sort_mode_t sortby, bool sortreverse);

	/**
	* Zeichnet die Komponente
	* @author Hj. Malthaner
	*/
	void zeichnen(koord offset);
};

#endif
