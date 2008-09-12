/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include <stdio.h>

#include "../simio.h"
#include "../simdebug.h"
#include "load_relief_frame.h"
#include "../dataobj/einstellungen.h"


/**
 * Aktion, die nach Knopfdruck gestartet wird.
 * @author Hansjörg Malthaner
 */
void load_relief_frame_t::action(const char *filename)
{
	cstring_t p("maps/");
	sets->heightfield = p+filename;
}


void load_relief_frame_t::del_action(const char *filename)
{
	cstring_t p("maps/");
	remove(p+filename);
}


load_relief_frame_t::load_relief_frame_t(einstellungen_t* sets) : savegame_frame_t("*.*","maps/")
{
    setze_name("Laden");

    this->sets = sets;
    sets->heightfield = "";
}


const char *load_relief_frame_t::get_info(const char *filename)
{
	static char size[64];
	char path[1024];
	sprintf( path, "save/%s", filename );
	FILE *file = fopen(path, "rb");
	if(file) {
		char buf [256];
		int w, h;

		read_line(buf, 255, file);

		if(strncmp(buf, "P6", 2)) {
			fclose(file);
			return "";
		}

		read_line(buf, 255, file);
		sscanf(buf, "%d %d", &w, &h);
		sprintf( size, "%i x %i", w, h );

		fclose(file);
		return size;
	}
	return "";
}



bool load_relief_frame_t::check_file( const char *filename, const char * )
{
	char path[1024];
	sprintf( path, "maps/%s", filename );
	FILE *file = fopen(path, "rb");
	if(file) {
		char buf [256];

		read_line(buf, 255, file);
		fclose(file);

		return strncmp(buf, "P6", 2)==0  ||  strncmp(buf, "BM", 2)==0 ;
	}
	return false;
}


