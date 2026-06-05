/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef SYSLOG
#include <syslog.h>
#endif

#include "log.h"
#include "cbuffer.h"
#include "../simdebug.h"
#include "../sys/simsys.h"


#ifdef MAKEOBJ
//debuglevel is global variable
#  define dr_fopen fopen
#else
#  ifdef NETTOOL
#    define debuglevel (0)
#    define dr_fopen fopen
#  else
#    define debuglevel (env_t::verbose_debug)

// for display
#    include "../gui/messagebox.h"
#    include "../display/simgraph.h"
#    include "../gui/simwin.h"
#    include "../dataobj/environment.h"
#  endif
#endif

#ifdef __ANDROID__
#  include <android/log.h>
#  define  LOG_TAG    "com.simutrans"
#endif

/**
 * writes a message into the log.
 */
void log_t::pakset(const char* who, const char* format, ...)
{
// never spam the Android buffer
#if !defined(__ANDROID__) && !defined (MAKEOBJ) && !defined(NETTOOL)
	if (env_t::pakset_debug) {
		va_list argptr;
		va_start(argptr, format);
		cbuffer_t msg;
		msg.vprintf(format, argptr);
		va_end(argptr);

		if (log) {
			fprintf(log, "%s%s\n", who, (const char *)msg);
			if (force_flush) fflush(log);
		}
		if (tee) {
			fprintf(tee, "%s%s\n", who, (const char *)msg);
		}
#ifdef SYSLOG
		if (syslog) {
			::syslog(LOG_INFO, "%s%s", who, (const char *)msg);
		}
#endif
	}
#else
	(void)who;
	(void)format;
#endif
}


/**
 * writes a debug message into the log.
 */
void log_t::debug(const char *who, const char *format, ...)
{
	if(log_debug  &&  debuglevel >= log_t::LEVEL_DEBUG) {
		va_list argptr;
		va_start(argptr, format);
		cbuffer_t msg;
		msg.vprintf(format, argptr);
		va_end(argptr);

		if (log) {
			fprintf(log, "Debug: %s:\t%s\n", who, (const char *)msg);
			if (force_flush) fflush(log);
		}
		if (tee) {
			fprintf(tee, "Debug: %s:\t%s\n", who, (const char *)msg);
		}
#ifdef SYSLOG
		if (syslog) {
			::syslog(LOG_DEBUG, "Debug: %s\t%s", who, (const char *)msg);
		}
#endif
#ifdef __ANDROID__
		__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "Debug: %s\t%s", who, (const char *)msg);
#endif
	}
}


/**
 * writes a message into the log.
 */
void log_t::message(const char *who, const char *format, ...)
{
	if(debuglevel >= log_t::LEVEL_MSG) {
		va_list argptr;
		va_start(argptr, format);
		cbuffer_t msg;
		msg.vprintf(format, argptr);
		va_end(argptr);

		if (log) {
			fprintf(log, "Message: %s:\t%s\n", who, (const char *)msg);
			if (force_flush) fflush(log);
		}
		if (tee) {
			fprintf(tee, "Message: %s:\t%s\n", who, (const char *)msg);
		}
#ifdef SYSLOG
		if (syslog) {
			::syslog(LOG_INFO, "Message: %s\t%s", who, (const char *)msg);
		}
#endif
#ifdef __ANDROID__
		__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Message: %s\t%s", who, (const char *)msg);
#endif
	}
}


/**
 * writes a warning into the log.
 */
void log_t::warning(const char *who, const char *format, ...)
{
	if(debuglevel >= log_t::LEVEL_WARN) {
		va_list argptr;
		va_start(argptr, format);
		cbuffer_t msg;
		msg.vprintf(format, argptr);
		va_end(argptr);

		if (log) {
			fprintf(log, "Warning: %s:\t%s\n", who, (const char *)msg);
			if (force_flush) fflush(log);
		}
		if (tee) {
			fprintf(tee, "Warning: %s:\t%s\n", who, (const char *)msg);
		}
#ifdef SYSLOG
		if (syslog) {
			::syslog(LOG_WARNING, "Warning: %s\t%s", who, (const char *)msg);
		}
#endif
#ifdef __ANDROID__
		__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Warning: %s\t%s", who, (const char *)msg);
#endif
	}
}


/**
 * writes an error into the log.
 */
void log_t::error(const char *who, const char *format, ...)
{
	if(debuglevel >= log_t::LEVEL_ERROR) {
		va_list argptr;
		va_start(argptr, format);
		cbuffer_t msg;
		msg.vprintf(format, argptr);
		va_end(argptr);

		static const char forum_hint[] =
			"For help with this error or to file a bug report please see the Simutrans forum:\n"
			"https://forum.simutrans.com\n";

		if (log) {
			fprintf(log, "ERROR: %s:\t%s\n%s", who, (const char *)msg, forum_hint);
			if (force_flush) fflush(log);
		}
		if (tee) {
			fprintf(tee, "ERROR: %s:\t%s\n%s", who, (const char *)msg, forum_hint);
		}
#ifdef SYSLOG
		if (syslog) {
			::syslog(LOG_ERR, "ERROR: %s\t%s", who, (const char *)msg);
		}
#endif
#ifdef __ANDROID__
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "ERROR: %s\t%s", who, (const char *)msg);
#endif
	}
}


/**
 * writes an error into the log, aborts the program.
 */
void log_t::fatal(const char* who, const char* format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	cbuffer_t msg;
	msg.vprintf(format, argptr);
	va_end(argptr);

	char buffer[8192];
	snprintf(buffer, sizeof(buffer),
		"FATAL ERROR: %s - %s\n"
		"Aborting program execution ...\n"
		"\n"
		"For help with this error or to file a bug report please see the Simutrans forum at\n"
		"https://forum.simutrans.com\n",
		who, (const char *)msg);

	custom_fatal(buffer);
}



void log_t::custom_fatal(char *buffer)
{
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
		::syslog( LOG_ERR, "%s", buffer );
	}
#endif

	if (  tee == NULL  &&  log == NULL  &&  tag == NULL  ) {
		fputs( buffer, stderr );
	}

#if defined MAKEOBJ
	exit(EXIT_FAILURE);
#elif defined NETTOOL
	// no display available
	puts( buffer );
	exit(1);
#else

	env_t::verbose_debug = log_t::LEVEL_FATAL; // no more window concerning messages

	if (gfx->is_display_init()) {
		// show notification
		destroy_all_win( true );

		strcat( buffer, "PRESS ANY KEY\n" );
		fatal_news* sel = new fatal_news(buffer);

		const scr_size screen = gfx->get_screen_size();
		scr_coord xy( screen.w/2 - sel->get_windowsize().w/2, screen.h/2 - sel->get_windowsize().h/2 );
		event_t ev;

		create_win( xy, sel, w_info, magic_none );

		while(win_is_top(sel)) {
			// do not move, do not close it!
			dr_sleep(50);
			dr_prepare_flush();
			sel->draw( xy, sel->get_windowsize() );
			dr_flush();
			display_poll_event(&ev);
			// main window resized
			check_pos_win(&ev,true);

			if (IS_KEYDOWN(&ev)) {
				break;
			}
		}
	}
	else {
		// use OS means, if there
		dr_fatal_notify(buffer);
	}

	abort();
#endif
}



void log_t::vmessage(const char *what, const char *who, const char *format, va_list args )
{
	if(debuglevel >= LEVEL_ERROR) {
		cbuffer_t msg;
		msg.vprintf(format, args);

		if (log) {
			fprintf(log, "%s: %s:\t%s\n", what, who, (const char *)msg);
			if (force_flush) fflush(log);
		}
		if (tee) {
			fprintf(tee, "%s: %s:\t%s\n", what, who, (const char *)msg);
		}
	}
}


// create a logfile for log_debug=true
log_t::log_t( const char *logfilename, bool force_flush, bool log_debug, bool log_console, const char *greeting, const char* syslogtag ) :
	log(NULL),
	tee(NULL),
	force_flush(force_flush), // if true will always flush when an entry is written to the log
	log_debug(log_debug),
	tag(NULL)
#ifdef SYSLOG
	, syslog(false)
#endif
{
	if(logfilename == NULL) {
		log = NULL;                       /* not a log */
		tee = NULL;
	}
	else if(strcmp(logfilename,"stdio") == 0) {
		log = stdout;
		tee = NULL;
	}
	else if(strcmp(logfilename,"stderr") == 0) {
		log = stderr;
		tee = NULL;
#ifdef SYSLOG
	}
	else if(  strcmp( logfilename, "syslog" ) == 0  ) {
		syslog = true;
		if (  syslogtag  ) {
			tag = syslogtag;
			// Set up syslog
			openlog( tag, LOG_PID, LOG_DAEMON );
		}
		log = NULL;
		tee = NULL;
#endif
	}
	else {
		log = dr_fopen(logfilename,"wb");

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
			::syslog( LOG_NOTICE, "%s", greeting );
		}
#else
		(void)syslogtag;
#endif
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
