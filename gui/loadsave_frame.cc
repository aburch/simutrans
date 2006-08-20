/*
 * loadsave_frame.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "../simdebug.h"

#include "loadsave_frame.h"
#include "../simworld.h"


/**
 * Aktion, die nach Knopfdruck gestartet wird.
 * @author Hansjörg Malthaner
 */
void loadsave_frame_t::action(const char *filename)
{
    if(do_load) {
	welt->laden(filename);
    } else {
	welt->speichern(filename,false);
    }

    welt->setze_dirty();
}

void loadsave_frame_t::del_action(const char *filename)
{
    remove(filename);
}

/**
 * Konstruktor.
 * @author Hj. Malthaner
 */
loadsave_frame_t::loadsave_frame_t(karte_t *welt, bool do_load) : savegame_frame_t(".sve")
{
	this->welt = welt;
	this->do_load = do_load;

	if(do_load) {
		setze_name("Laden");
	} else {
		set_filename(welt->gib_einstellungen()->gib_filename());
		setze_name("Speichern");
	}
}


/**
 * Manche Fenster haben einen Hilfetext assoziiert.
 * @return den Dateinamen für die Hilfe, oder NULL
 * @author Hj. Malthaner
 */
const char * loadsave_frame_t::gib_hilfe_datei() const
{
    return do_load ? "load.txt" : "save.txt";
}
