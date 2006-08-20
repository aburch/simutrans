/*
 * factorylist_stats_t.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef factorylist_stats_t_h
#define factorylist_stats_t_h

#include "../tpl/vector_tpl.h"
#include "../ifc/gui_komponente.h"
#include "../dataobj/translator.h"
#include "../simgraph.h"
#include "../simcolor.h"
#include "button.h"
#include "fabrik_info.h"

class karte_t;
class fabrik_t;
class button_t;
class factorylist_stats_t;

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
	vector_tpl<fabrik_t*> *fab_list;

public:

	factorylist_stats_t(karte_t *welt,const factorylist::sort_mode_t& sortby,const bool& sortreverse);
	~factorylist_stats_t();

	/**
	* Events werden hiermit an die GUI-Komponenten
	* gemeldet
	* @author Hj. Malthaner
	*/
	virtual void infowin_event(const event_t *);

	void sort(const factorylist::sort_mode_t& sortby,const bool& sortreverse);

	/**
	* Zeichnet die Komponente
	* @author Hj. Malthaner
	*/
	void zeichnen(koord offset) const;
};

#endif // factorylist_stats_t_h
