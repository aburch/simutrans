/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include <stdio.h>

#include "../simdebug.h"
#include "load_relief_frame.h"
#include "../dataobj/einstellungen.h"


/**
 * Aktion, die nach Knopfdruck gestartet wird.
 * @author Hansjörg Malthaner
 */
void load_relief_frame_t::action(const char *filename)
{
	sets->heightfield = filename;
}


void load_relief_frame_t::del_action(const char *filename)
{
	remove(filename);
}


load_relief_frame_t::load_relief_frame_t(einstellungen_t* sets) : savegame_frame_t(".ppm",NULL)
{
    setze_name("Laden");

    this->sets = sets;
    sets->heightfield = "";
}


/**
 * Manche Fenster haben einen Hilfetext assoziiert.
 * @return den Dateinamen für die Hilfe, oder NULL
 * @author Hj. Malthaner
 */
const char * load_relief_frame_t::gib_hilfe_datei() const
{
    return "load_relief.txt";
}
