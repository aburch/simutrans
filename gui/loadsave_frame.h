/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_loadsave_frame_h
#define gui_loadsave_frame_h


#include "savegame_frame.h"

class karte_t;


class loadsave_frame_t : public savegame_frame_t
{
private:
	karte_t *welt;
	bool do_load;

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
	virtual void del_action(const char *filename);

public:
	/**
	* Manche Fenster haben einen Hilfetext assoziiert.
	* @return den Dateinamen für die Hilfe, oder NULL
	* @author Hj. Malthaner
	*/
	virtual const char * gib_hilfe_datei() const;

	loadsave_frame_t(karte_t *welt, bool do_load);
};

#endif
