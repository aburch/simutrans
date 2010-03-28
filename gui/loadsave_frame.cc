/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simdebug.h"

#ifndef _MSC_VER
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#else
#include <io.h>
#include <direct.h>
#endif
#include <sys/stat.h>
#include <time.h>

#include "loadsave_frame.h"

#include "../simworld.h"
#include "../dataobj/loadsave.h"
#include "../pathes.h"
#include "../utils/simstring.h"


/**
 * Aktion, die nach Knopfdruck gestartet wird.
 * @author Hansjörg Malthaner
 */
void loadsave_frame_t::action(const char *filename)
{
	if(do_load) {
		welt->laden(filename);
	}
	else {
		welt->speichern(filename,false);
		welt->set_dirty();
		welt->reset_timer();
	}
}

bool loadsave_frame_t::del_action(const char *filename)
{
	remove(filename);
	return false;
}


loadsave_frame_t::loadsave_frame_t(karte_t *welt, bool do_load) : savegame_frame_t(".sve",NULL)
{
	this->welt = welt;
	this->do_load = do_load;

	if(do_load) {
		set_name("Laden");
	}
	else {
		set_filename(welt->get_einstellungen()->get_filename());
		set_name("Speichern");
	}

	set_min_windowsize(get_fenstergroesse());
	set_resizemode(diagonal_resize);
	set_fenstergroesse(koord(640+36, get_fenstergroesse().y));

}


/**
 * Manche Fenster haben einen Hilfetext assoziiert.
 * @return den Dateinamen für die Hilfe, oder NULL
 * @author Hj. Malthaner
 */
const char * loadsave_frame_t::get_hilfe_datei() const
{
	return do_load ? "load.txt" : "save.txt";
}




const char *loadsave_frame_t::get_info(const char *fname)
{
	static char date[1024];
	// first get pak name
	loadsave_t test;
	char path[1024];
	sprintf( path, SAVE_PATH_X "%s", fname );
	test.rd_open(path);
	// then get date
	date[0] = 0;
	struct stat  sb;
	if(stat(path, &sb)==0) {
		// time 
		struct tm *tm = localtime(&sb.st_mtime);
		if(tm) {
			strftime(date, 18, "%Y-%m-%d %H:%M", tm);
		}
		else {
			tstrncpy(date, "??.??.???? ??:??", 16);
		}
		size_t n = strlen(date);

		// file version 
		uint32 v2 = test.get_version();
		uint32 v1 = v2 / 1000;
		uint32 v0 = v1 / 1000;
		v1 %= 1000;
		v2 %= 1000;
		n += sprintf( date + n, " - v %d.%d.%d", v0, v1, v2);
		uint32 v3 = test.get_experimental_version();
		if (v3)
		{
			n += sprintf( date + n, " e %d", v3);
		}
		else
		{
			n += sprintf( date + n, "     ");
		}

		// pak extension
		sprintf( date + n, " - %s", test.get_pak_extension() );
	}
	return date;
}




