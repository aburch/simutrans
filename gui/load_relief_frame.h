/*
 * load_relief_frame.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef gui_load_relief_frame_h
#define gui_load_relief_frame_h


#include "savegame_frame.h"


class einstellungen_t;

class load_relief_frame_t : public savegame_frame_t
{
private:
	einstellungen_t * sets;

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

	load_relief_frame_t(einstellungen_t* sets);

	virtual ~load_relief_frame_t() {}
};

#endif
