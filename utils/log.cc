/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef SYSLOG
#include <syslog.h>
#endif //SYSLOG

#define NO_LOG_EXTERNALS

#include "log.h"
#include "../simdebug.h"
#include "../simsys.h"


#ifdef MAKEOBJ
//debuglevel is global variable
#else
#ifdef NETTOOL
#define debuglevel (0)

#else
#define debuglevel (env_t::verbose_debug)

// for display ...
#include "../gui/messagebox.h"
#include "../display/simgraph.h"
#include "../gui/simwin.h"

#include "../dataobj/environment.h"
#endif
#endif

/**
 * writes important messages to stdout/logfile
 * use instead of printf()
 */
void log_t::important(const char* format, ...)
{
	va_list argptr;

	va_start( argptr, format );
	if (  log  ) {
		// If logfile, output there
		vfprintf( log, format, argptr );
		fprintf( log, "\n" );
		if (  force_flush  ) { fflush( log ); }
	}
	va_end(argptr);

#ifdef SYSLOG
	va_start( argptr, format );
	if (  syslog  ) {
		// Send to syslog if available
		vsyslog( LOG_NOTICE, format, argptr );
	}
	va_end(argptr);
#endif //SYSLOG

	va_start( argptr, format );

	// Print to stdout for important messages
	if (  log != stderr  ) {
		vfprintf( stdout, format, argptr );
		fprintf( stdout, "\n" );
		if (  force_flush  ) { fflush( stdout ); }
	}

	va_end( argptr );
}

/**
 * writes a debug message into the log.
 * @author Hj. Malthaner
 */
void log_t::debug(const char *who, const char *format, ...)
{
	if(log_debug  &&  debuglevel==4) {
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
		va_end(argptr);

		va_start(argptr, format);
		if( tee ) {                         /* nur loggen wenn schon ein log */
			fprintf(tee, "Debug: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(tee, format, argptr);
			fprintf(tee,"\n");
		}
		va_end(argptr);

#ifdef SYSLOG
		va_start( argptr, format );
		if (  syslog  ) {
			// Replace with dynamic memory allocation
			char buffer[256];
			sprintf( buffer, "Debug: %s\t%s", who, format );
			vsyslog( LOG_DEBUG, buffer, argptr );
		}
		va_end( argptr );
#endif //SYSLOG
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
		va_end(argptr);

		va_start(argptr, format);
		if( tee ) {                         /* nur loggen wenn schon ein log */
			fprintf(tee, "Message: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(tee, format, argptr);
			fprintf(tee,"\n");
		}
		va_end(argptr);

#ifdef SYSLOG
		va_start( argptr, format );
		if (  syslog  ) {
			// Replace with dynamic memory allocation
			char buffer[256];
			sprintf( buffer, "Message: %s\t%s", who, format );
			vsyslog( LOG_INFO, buffer, argptr );
		}
		va_end( argptr );
#endif //SYSLOG
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
		va_end(argptr);

		va_start(argptr, format);
		if( tee ) {                         /* nur loggen wenn schon ein log */
			fprintf(tee, "Warning: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(tee, format, argptr);
			fprintf(tee,"\n");
		}
		va_end(argptr);

#ifdef SYSLOG
		va_start( argptr, format );
		if (  syslog  ) {
			// Replace with dynamic memory allocation
			char buffer[256];
			sprintf( buffer, "Warning: %s\t%s", who, format );
			vsyslog( LOG_WARNING, buffer, argptr );
		}
		va_end( argptr );
#endif //SYSLOG
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

			fprintf(log ,"For help with this error or to file a bug report please see the Simutrans forum:\n");
			fprintf(log ,"http://forum.simutrans.com\n");
		}
		va_end(argptr);

		va_start(argptr, format);
		if( tee ) {                         /* nur loggen wenn schon ein log */
			fprintf(tee, "ERROR: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(tee, format, argptr);
			fprintf(tee,"\n");

			fprintf(tee ,"For help with this error or to file a bug report please see the Simutrans forum:\n");
			fprintf(tee ,"http://forum.simutrans.com\n");
		}
		va_end(argptr);

#ifdef SYSLOG
		va_start( argptr, format );
		if (  syslog  ) {

			// Replace with dynamic memory allocation
			char buffer[256];
			sprintf( buffer, "ERROR: %s\t%s", who, format );
			vsyslog( LOG_ERR, buffer, argptr );
		}
		va_end( argptr );
#endif //SYSLOG
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

	static char formatbuffer[512];
	sprintf( formatbuffer, "FATAL ERROR: %s - %s\nAborting program execution ...\n\nFor help with this error or to file a bug report please see the Simutrans forum at\nhttp://forum.simutrans.com\n", who, format );

	static char buffer[8192];
	int n = vsprintf( buffer, formatbuffer, argptr );

	if(  log  ) {
		fputs( buffer, log );
		if (  force_flush  ) {
			fflush( log );
		}
	}

	if(  tee  &&  log!=tee  ) {
		fputs( buffer, tee );
	}

#ifdef SYSLOG
	if (  syslog  ) {
		::syslog( LOG_ERR, buffer );
	}
#endif //SYSLOG

	if (  tee == NULL  &&  log == NULL  &&  tag == NULL  ) {
		fputs( buffer, stderr );
	}

	va_end(argptr);

#if defined MAKEOBJ
	exit(1);
#elif defined NETTOOL
	// no display available
	puts( buffer );
#else
#  ifdef DEBUG
	int old_level = env_t::verbose_debug;
#  endif
	env_t::verbose_debug = 0;	// no more window concerning messages
	if(is_display_init()) {
		// show notification
		destroy_all_win( true );

		strcpy( buffer+n+1, "PRESS ANY KEY\n" );
		news_img* sel = new news_img(buffer,IMG_LEER);
		sel->extend_window_with_component( NULL, scr_size(display_get_width()/2,120) );

		scr_coord xy( display_get_width()/2 - sel->get_windowsize().w/2, display_get_height()/2 - sel->get_windowsize().h/2 );
		event_t ev;

		create_win( xy.x, xy.y, sel, w_info, magic_none );

		while(win_is_top(sel)) {
			// do not move, do not close it!
			dr_sleep(50);
			dr_prepare_flush();
			sel->draw( xy, sel->get_windowsize() );
			dr_flush();
			display_poll_event(&ev);
			// main window resized
			check_pos_win(&ev);
			if(ev.ev_class==EVENT_KEYBOARD) {
				break;
			}
		}
	}
	else {
		// use OS means, if there
		dr_fatal_notify(buffer);
	}

#ifdef DEBUG
	if (old_level > 4) {
		// generate a division be zero error, if the user request it
		static int make_this_a_division_by_zero = 0;
		printf("%i", 15 / make_this_a_division_by_zero);
		make_this_a_division_by_zero &= 0xFF;
	}
#endif
#endif
	abort();
}



void log_t::vmessage(const char *what, const char *who, const char *format, va_list args )
{
	if(debuglevel>0) {
		va_list args2;
#ifdef __va_copy
		__va_copy(args2, args);
#else
		// HACK: this is undefined behavior but should work ... hopefully ...
		args2 = args;
#endif
		if( log ) {                         /* nur loggen wenn schon ein log */
			fprintf(log ,"%s: %s:\t", what, who);      /* geoeffnet worden ist */
			vfprintf(log, format, args);
			fprintf(log,"\n");

			if( force_flush ) {
				fflush(log);
			}
		}
		if( tee ) {                         /* nur loggen wenn schon ein log */
			fprintf(tee,"%s: %s:\t", what, who);      /* geoeffnet worden ist */;
			vfprintf(tee, format, args2);
			fprintf(tee,"\n");
		}
		va_end(args2);
	}
}


// create a logfile for log_debug=true
log_t::log_t( const char *logfilename, bool force_flush, bool log_debug, bool log_console, const char *greeting, const char* syslogtag )
{
	log = NULL;
	syslog = false;
	this->force_flush = force_flush; /* wenn true wird jedesmal geflusht */
					 /* wenn ein Eintrag ins log geschrieben wurde */
	this->log_debug = log_debug;

	if(logfilename == NULL) {
		log = NULL;                       /* kein log */
		tee = NULL;
	} else if(strcmp(logfilename,"stdio") == 0) {
		log = stdout;
		tee = NULL;
	} else if(strcmp(logfilename,"stderr") == 0) {
		log = stderr;
		tee = NULL;
#ifdef SYSLOG
	} else if(  strcmp( logfilename, "syslog" ) == 0  ) {
		syslog = true;
		if (  syslogtag  ) {
			tag = syslogtag;
			// Set up syslog
			openlog( tag, LOG_PID, LOG_DAEMON );
		}
		log = NULL;
		tee = NULL;
#endif //SYSLOG
	} else {
		log = fopen(logfilename,"wb");

		if(log == NULL) {
			fprintf(stderr,"log_t::log_t: can't open file '%s' for writing\n", logfilename);
		}
		tee = stderr;
	}
	if (!log_console) {
	    tee = NULL;
	}

	if(  greeting  ) {
		if( log ) {
			fputs( greeting, log );
//			message("log_t::log_t","Starting logging to %s", logfilename);
		}
		if( tee ) {
			fputs( greeting, tee );
		}
#ifdef SYSLOG
		if (  syslog  ) {
			::syslog( LOG_NOTICE, greeting );
		}
#else
		(void)syslogtag;
#endif //SYSLOG
	}
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
