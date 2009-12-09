/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
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
	virtual bool del_action(const char *filename);

	// returns extra file info
	virtual const char *get_info(const char *fname);

	// true, if valid
	virtual bool check_file( const char *filename, const char *suffix );

public:
	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char *get_hilfe_datei() const { return "load_relief.txt"; }

	load_relief_frame_t(einstellungen_t* sets);
};

#endif
