/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_scenario_frame_h
#define gui_scenario_frame_h


#include "savegame_frame.h"
#include "../utils/cbuffer_t.h"

class karte_t;


class scenario_frame_t : public savegame_frame_t
{
private:
	karte_t *welt;
	bool do_load;
	cbuffer_t path;

protected:
	/**
	 * Aktion, die nach Knopfdruck gestartet wird.
	 * @author Hansjörg Malthaner
	 */
	virtual void action(const char *filename);

	/**
	 * Aktion, die nach X-Knopfdruck gestartet wird.
	 * @author V. Meyer
	 */
	virtual bool del_action(const char *f) { action(f); return true; }

	// returns extra file info
	virtual const char *get_info(const char *fname);

public:
	/**
	* Manche Fenster haben einen Hilfetext assoziiert.
	* @return den Dateinamen für die Hilfe, oder NULL
	* @author Hj. Malthaner
	*/
	virtual const char * get_hilfe_datei() const { return "scenario.txt"; }

	scenario_frame_t(karte_t *welt);
};

#endif
