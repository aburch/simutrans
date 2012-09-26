/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simdebug.h"

#include "scenario_frame.h"
#include "scenario_info.h"
#include "messagebox.h"

#include "../simwin.h"
#include "../simworld.h"

#include "../dataobj/umgebung.h"
#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"


/**
 * Aktion, die nach Knopfdruck gestartet wird.
 * @author Hansjörg Malthaner
 */
void scenario_frame_t::action(const char *filename)
{
	scenario_t *scn = new scenario_t(welt);
	path.append(filename);
	const char* err = scn->init( path, welt );
	if (err == NULL) {
		// start the game
		welt->set_pause(false);
		// open scenario info window
		destroy_win(magic_scenario_info);
		create_win(new scenario_info_t(welt), w_info, magic_scenario_info);
	}
	else {
		create_win(new news_img(err), w_info, magic_none);
		delete scn;
	}
}


scenario_frame_t::scenario_frame_t(karte_t *welt) : savegame_frame_t(".nut","./")
{
	this->welt = welt;
	path.printf("%s%sscenario/", umgebung_t::program_dir, umgebung_t::objfilename.c_str() );
	// set search path
	fullpath = path;
	set_name(translator::translate("Load scenario"));
	set_focus(NULL);
}


const char *scenario_frame_t::get_info(const char *filename)
{
	return filename;
	/*
	scenario_t scn(NULL);
	char path[1024];
	sprintf( path, "%s%sscenario/%s", umgebung_t::program_dir, umgebung_t::objfilename.c_str(), filename );
	scn.init( path, NULL );
	return scn.get_description();*/
}
