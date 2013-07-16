/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string>
#include <stdio.h>

#include "../simworld.h"
#include "load_relief_frame.h"
#include "../dataobj/translator.h"
#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"

/**
 * Aktion, die nach Knopfdruck gestartet wird.
 * @author Hansjörg Malthaner
 */
void load_relief_frame_t::action(const char *fullpath)
{
	sets->heightfield = fullpath;
}


load_relief_frame_t::load_relief_frame_t(settings_t* const sets) : savegame_frame_t(NULL, false, "maps/")
{
	static char extra_path[1024];

	sprintf(extra_path,"%s%smaps/", umgebung_t::program_dir, umgebung_t::objfilename.c_str());
	//sprintf(extra_path,"%smaps/", umgebung_t::program_dir);

	this->add_path(extra_path);

	set_name(translator::translate("Lade Relief"));
	this->sets = sets;
	sets->heightfield = "";
}


const char *load_relief_frame_t::get_info(const char *fullpath)
{
	static char size[64];

	sint16 w, h;
	sint8 *h_field ;
	if(karte_t::get_height_data_from_file(fullpath, (sint8)sets->get_grundwasser(), h_field, w, h, true )) {
		sprintf( size, "%i x %i", w, h );
		return size;
	}
	return "";
}


bool load_relief_frame_t::check_file( const char *fullpath, const char * )
{
	sint16 w, h;
	sint8 *h_field;

	if(karte_t::get_height_data_from_file(fullpath, (sint8)sets->get_grundwasser(), h_field, w, h, true )) {
		return w>0  &&  h>0;
	}
	return false;
}
