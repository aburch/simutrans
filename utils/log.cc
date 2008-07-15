/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "log.h"
#include "../simdebug.h"
#include "../simsys.h"


static int make_this_a_division_by_zero = 0;

#ifdef MAKEOBJ
#define debuglevel (3)

#else
#define debuglevel (umgebung_t::verbose_debug)

// for display ...
#include "../gui/messagebox.h"
#include "../simgraph.h"
#include "../simwin.h"

#include "../dataobj/umgebung.h"
#endif

/**
 * writes a debug message into the log.
 * @author Hj. Malthaner
 */
void log_t::debug(const char *who, const char *format, ...)
{
  if(log_debug  &&  debuglevel>3) {
    va_list argptr;
    va_start(argptr, format);

    if( log ) {                         /* nur loggen wenn schon ein log */
		fprintf(log ,"Debug: %s:\t",who);      /* geoeffnet worden ist */
		vfprintf(log, format, argptr);
		fprintf(log,"\n");

        if( force_flush ) {
            fflush(log);
        }
    }

    if( tee ) {                         /* nur loggen wenn schon ein log */
		fprintf(tee, "Debug: %s:\t",who);      /* geoeffnet worden ist */
        vfprintf(tee, format, argptr);
        fprintf(tee,"\n");
    }

    va_end(argptr);
  }
}


/**
 * writes a message into the log.
 * @author Hj. Malthaner
 */
void log_t::message(const char *who, const char *format, ...)
{
	if(debuglevel>2) {
		va_list argptr;
		va_start(argptr, format);

		if( log ) {                         /* nur loggen wenn schon ein log */
			fprintf(log ,"Message: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(log, format, argptr);
			fprintf(log,"\n");

			if( force_flush ) {
				fflush(log);
			}
		}

		if( tee ) {                         /* nur loggen wenn schon ein log */
			fprintf(tee, "Message: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(tee, format, argptr);
			fprintf(tee,"\n");
		}

		va_end(argptr);
	}
}


/**
 * writes a warning into the log.
 * @author Hj. Malthaner
 */
void log_t::warning(const char *who, const char *format, ...)
{
	if(debuglevel>1) {
		va_list argptr;
		va_start(argptr, format);

		if( log ) {                         /* nur loggen wenn schon ein log */
			fprintf(log ,"Warning: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(log, format, argptr);
			fprintf(log,"\n");

			if( force_flush ) {
				fflush(log);
			}
		}
		if( tee ) {                         /* nur loggen wenn schon ein log */
			fprintf(tee, "Warning: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(tee, format, argptr);
			fprintf(tee,"\n");
		}
		va_end(argptr);
	}
}


/**
 * writes an error into the log.
 * @author Hj. Malthaner
 */
void log_t::error(const char *who, const char *format, ...)
{
	if(debuglevel>0) {
		va_list argptr;
		va_start(argptr, format);

		if( log ) {                         /* nur loggen wenn schon ein log */
			fprintf(log ,"ERROR: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(log, format, argptr);
			fprintf(log,"\n");

			if( force_flush ) {
				fflush(log);
			}

			fprintf(log ,"Please report all errors to\n");
			fprintf(log ,"team@64.simutrans.com\n");
		}
		if( tee ) {                         /* nur loggen wenn schon ein log */
			fprintf(tee, "ERROR: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(tee, format, argptr);
			fprintf(tee,"\n");

			fprintf(tee ,"Please report all errors to\n");
			fprintf(tee ,"team@64.simutrans.com\n");
		}
		va_end(argptr);
	}
}


/**
 * writes an error into the log, aborts the program.
 * @author Hj. Malthaner
 */
void log_t::fatal(const char *who, const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);

	static char buffer[8192];

	int n = sprintf( buffer, "FATAL ERROR: %s\n", who);
	n += vsprintf( buffer+n, format, argptr);
	strcpy( buffer+n, "\n" );

	if( log ) {
		fputs( buffer, log );
		fputs( "Aborting program execution ...\n\n", log );
		fputs( "Please report all fatal errors to\n", log );
		fputs( "team@64.simutrans.com\n", log );
        if( force_flush ) {
            fflush(log);
        }
    }

    if( tee ) {
		fputs( buffer, tee );
		fputs( "Aborting program execution ...\n\n", tee );
		fputs( "Please report all fatal errors to\n", tee );
		fputs( "team@64.simutrans.com\n", tee );
	}

	if(tee==NULL  &&  log==NULL) {
		puts( buffer );
	}

	va_end(argptr);

#ifdef MAKEOBJ
	// no display available
	puts( buffer );
#else
	int old_level = umgebung_t::verbose_debug;
	umgebung_t::verbose_debug = 0;	// no more window concerning messages
	if(is_display_init()) {
		// show notification
		destroy_all_win();

		strcpy( buffer+n+1, "PRESS ANY KEY\n" );
		news_img* sel = new news_img(buffer,IMG_LEER);
		sel->setze_fenstergroesse( sel->gib_fenstergroesse()+koord(150,0) );

		koord xy( display_get_width()/2 - 180, display_get_height()/2 - sel->gib_fenstergroesse().y/2 );
		event_t ev;

		create_win( xy.x, xy.y, sel, w_info, magic_none );

		while(win_is_top(sel)) {
			// do not move, do not close it!
			sel->zeichnen( xy, sel->gib_fenstergroesse() );
			dr_sleep(50);
			display_poll_event(&ev);
			// main window resized
			check_pos_win(&ev);
			if(ev.ev_class==EVENT_KEYBOARD) {
				break;
			}
			dr_flush();
		}
	}
	else {
		// use OS means, if there
		dr_fatal_notify( buffer, 0 );
	}

#ifdef DEBUG
	if(old_level>4) {
		// generate a division be zero error, if the user request it
		printf("%i",15/make_this_a_division_by_zero);
		make_this_a_division_by_zero &= 0xFF;
	}
#endif
#endif
	exit(0);
}



// create a logfile for log_debug=true
log_t::log_t(const char *logfilename, bool force_flush, bool log_debug)
{
	log = NULL;
	this->force_flush = force_flush;    /* wenn true wird jedesmal geflusht */
										/* wenn ein Eintrag ins log geschrieben wurde */
	this->log_debug = log_debug;

	if(logfilename == NULL) {
		log=NULL;                       /* kein log */
		tee = NULL;
	} else if(strcmp(logfilename,"stdio") == 0) {
		log = stdout;
		tee = NULL;
	} else if(strcmp(logfilename,"stderr") == 0) {
		log = stderr;
		tee = NULL;
	} else {
		log = fopen(logfilename,"wb");

		if(log == NULL) {
			fprintf(stderr,"log_t::log_t: can't open file '%s' for writing\n", logfilename);
		}
		tee = stderr;
	}
//	message("log_t::log_t","Starting logging to %s", logfilename);
}



void log_t::close()
{
	message("log_t::~log_t","stop logging, closing log file");

	if( log ) {
		fclose(log);
		log = NULL;
	}
}


// close all logs during cleanup
log_t::~log_t()
{
	if( log ) {
		close();
	}
}
