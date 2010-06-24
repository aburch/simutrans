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
	sprintf( path, "%s%sscenario/%s.tab", umgebung_t::program_dir, umgebung_t::objfilename.c_str(), filename );
	scn.init( path, welt );
	sprintf( path2, "%s%sscenario/%s", umgebung_t::program_dir, umgebung_t::objfilename.c_str(), scn.get_filename() );
	welt->laden( path2 );
	welt->get_scenario()->init( path, welt );
	// finally set game name to scenario name ...
	sprintf( path, "%s.sve", filename );
	welt->get_einstellungen()->set_filename( path );
}


scenario_frame_t::scenario_frame_t(karte_t *welt) : savegame_frame_t(".tab","./")
{
	this->welt = welt;
	set_name("Load scenario");
}


const char *scenario_frame_t::get_info(const char *filename)
{
	scenario_t scn(NULL);
	char path[1024];
	sprintf( path, "%s%sscenario/%s", umgebung_t::program_dir, umgebung_t::objfilename.c_str(), filename );
	scn.init( path, NULL );
	return scn.get_description();
}
