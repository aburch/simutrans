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

#include "../utils/cbuffer_t.h"


/**
 * Aktion, die nach Knopfdruck gestartet wird.
 * @author Hansjörg Malthaner
 */
void scenario_frame_t::action(const char *fullpath)
{
	scenario_t *scn = new scenario_t(welt);
	const char* err = scn->init(this->get_basename(fullpath).c_str(), this->get_filename(fullpath).c_str(), welt );
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


scenario_frame_t::scenario_frame_t(karte_t *welt) : savegame_frame_t(NULL, true, NULL, false)
{
	static cbuffer_t pakset_scenario;
	static cbuffer_t addons_scenario;

	pakset_scenario.clear();
	pakset_scenario.printf("%s%sscenario/", umgebung_t::program_dir, umgebung_t::objfilename.c_str());

	addons_scenario.clear();
	addons_scenario.printf("addons/%sscenario/", umgebung_t::objfilename.c_str());

	this->add_path(addons_scenario);
	this->add_path(pakset_scenario);
	this->welt = welt;

	set_name(translator::translate("Load scenario"));
	set_focus(NULL);
}


const char *scenario_frame_t::get_info(const char *filename)
{
	static char info[1024];

	sprintf(info,"%s",this->get_filename(filename, false).c_str());

	return info;
}


bool scenario_frame_t::check_file( const char *filename, const char * )
{
	char buf[1024];
	sprintf( buf, "%s/scenario.nut", filename );
	if (FILE* const f = fopen(buf, "r")) {
		fclose(f);
		return true;
	}
	return false;
}
