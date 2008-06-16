/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simdebug.h"

#include "scenario_frame.h"

#include "../simworld.h"

#include "../dataobj/umgebung.h"
#include "../dataobj/scenario.h"


/**
 * Aktion, die nach Knopfdruck gestartet wird.
 * @author Hansjörg Malthaner
 */
void scenario_frame_t::action(const char *filename)
{
	scenario_t scn(welt);
	char path[1024], path2[1024];
	sprintf( path, "%s%sscenario/%s.tab", umgebung_t::program_dir, (const char *)umgebung_t::objfilename, filename );
	scn.init( path, welt );
	sprintf( path2, "%s%sscenario/%s", umgebung_t::program_dir, (const char *)umgebung_t::objfilename, scn.get_filename() );
	welt->laden( path2 );
	welt->get_scenario()->init( path, welt );
}

void scenario_frame_t::del_action(const char *filename)
{
}


scenario_frame_t::scenario_frame_t(karte_t *welt, bool do_load) : savegame_frame_t(".tab","./")
{
	this->welt = welt;
	setze_name("Load scenario");
}


