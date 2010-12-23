/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string>
#include <stdio.h>

#include "../simio.h"
#include "../simdebug.h"
#include "../simworld.h"
#include "load_relief_frame.h"
#include "../dataobj/einstellungen.h"


/**
 * Aktion, die nach Knopfdruck gestartet wird.
 * @author Hansjörg Malthaner
 */
void load_relief_frame_t::action(const char *filename)
{
	std::string p("maps/");
	sets->heightfield = p+filename;
}


bool load_relief_frame_t::del_action(const char *filename)
{
	std::string p("maps/");
	remove((p+filename).c_str());
	return false;
}


load_relief_frame_t::load_relief_frame_t(einstellungen_t* sets) : savegame_frame_t("*.*","maps/")
{
	set_name("Laden");
	this->sets = sets;
	sets->heightfield = "";
}


const char *load_relief_frame_t::get_info(const char *filename)
{
	static char size[64];
	char path[1024];
	sprintf( path, "maps/%s", filename );

	sint16 w, h;
	sint8 *h_field ;
	if(karte_t::get_height_data_from_file(path, sets->get_grundwasser(), h_field, w, h, true )) {
		sprintf( size, "%i x %i", w, h );
		return size;
	}
	return "";
}



bool load_relief_frame_t::check_file( const char *filename, const char * )
{
	char path[1024];
	sprintf( path, "maps/%s", filename );
	sint16 w, h;
	sint8 *h_field ;

	if(karte_t::get_height_data_from_file(path, sets->get_grundwasser(), h_field, w, h, true )) {
		return w>0  &&  h>0;
	}
	return false;
}
