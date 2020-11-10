/*
 * Copyright 2013 Nathanael Nerode
 *
 * This file is part of the Simutrans-Extended project under the artistic license.
 * This file is also licensed under the Artistic License 2.0 and the GNU GPL 3.0.
 *
 * This is a dumb version of the logging facilities for use with unit tests.
 * It is designed to allow unit tests to be written for pieces of simutrans,
 * without linking in everything dragged in by the (highly complex) standard logging.
 *
 * This file must be updated when the interface in log.h is updated.
 *
 * Do NOT link this into simutrans; it is solely for use with unit tests.
 */

#include <stdlib.h> // for abort
#include <stdio.h>  // for fflush, fputs, fprintf, etc.
#include <stdarg.h> // for va_start, vfprintf, etc.

#include "log.h"
#include "../simdebug.h"

/**
 * writes important messages to stderr
 */
void log_t::important(const char* format, ...)
{
	va_list argptr;

	va_start( argptr, format );

	vfprintf( stderr, format, argptr );
	fprintf( stderr, "\n" );
	if (  force_flush  ) { fflush( stderr ); }

	va_end( argptr );
}

/**
 * writes a debug message to stderr
 */
void log_t::debug(const char *who, const char *format, ...)
{
	va_list argptr;

	va_start(argptr, format);
	fprintf(stderr ,"Debug: %s:\t",who);
	vfprintf(stderr, format, argptr);
	fprintf(stderr,"\n");

	if( force_flush ) {
		fflush(stderr);
	}

	va_end(argptr);
}


/**
 * writes a message to stderr
 */
void log_t::message(const char *who, const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	fprintf(stderr ,"Message: %s:\t",who);
	vfprintf(stderr, format, argptr);
	fprintf(stderr,"\n");

	if( force_flush ) {
		fflush(stderr);
	}
	va_end(argptr);
}


/**
 * writes a warning to stderr
 */
void log_t::warning(const char *who, const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);

	vfprintf(stderr, format, argptr);
	fprintf(stderr,"\n");

	if( force_flush ) {
		fflush(stderr);
	}
	va_end(argptr);
}


/**
 * writes an error to stderr
 */
void log_t::error(const char *who, const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);

	vfprintf(stderr, format, argptr);
	fprintf(stderr,"\n");

	if( force_flush ) {
		fflush(stderr);
	}

	va_end(argptr);
}


/**
 * writes an error to stderr and aborts the program.
 */
void log_t::fatal(const char *who, const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);

	static char formatbuffer[512];
	sprintf( formatbuffer, "FATAL ERROR: %s - %s\nAborting program execution ...\n", who, format );

	static char buffer[8192];
	int n = vsprintf( buffer, formatbuffer, argptr );

	fputs( buffer, stderr );
	if (  force_flush  ) {
		fflush( stderr );
	}

	va_end(argptr);

	abort();
}



void log_t::vmessage(const char *what, const char *who, const char *format, va_list args )
{
	fprintf(stderr ,"%s: %s:\t", what, who);
	vfprintf(stderr, format, args);
	fprintf(stderr,"\n");

	if( force_flush ) {
		fflush(stderr);
	}
}


// Set the force_flush setting.
// Everything else is unimplmented.
// Remember, this is for unit testing.
log_t::log_t( const char *logfilename, bool force_flush, bool log_debug, bool log_console, const char *greeting, const char* syslogtag )
{
	this->force_flush = force_flush;
}


void log_t::close()
{
}

log_t::~log_t()
{
}
