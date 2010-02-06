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
	set_fenstergroesse(koord(360+36, get_fenstergroesse().y));

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
		// add pak extension
		size_t n = sprintf( date, "%s - ", test.get_pak_extension() );

		// add the time too
		struct tm *tm = localtime(&sb.st_mtime);
		if(tm) {
			strftime(date+n, 18, "%Y-%m-%d %H:%M", tm);
		}
		else {
			tstrncpy(date, "??.??.???? ??:??", 15);
		}
	}
	return date;
}




