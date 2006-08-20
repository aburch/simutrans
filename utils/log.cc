/*
 * log.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "log.h"


/**
 * writes a debug message into the log.
 * @author Hj. Malthaner
 */
void log_t::debug(const char *who, const char *format, ...)
{
  if(log_debug) {
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


/**
 * writes a warning into the log.
 * @author Hj. Malthaner
 */
void log_t::warning(const char *who, const char *format, ...)
{
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


/**
 * writes an error into the log.
 * @author Hj. Malthaner
 */
void log_t::error(const char *who, const char *format, ...)
{
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
	fprintf(log ,"markus@pristovsek.de\n");
    }

    if( tee ) {                         /* nur loggen wenn schon ein log */
	fprintf(tee, "ERROR: %s:\t",who);      /* geoeffnet worden ist */
        vfprintf(tee, format, argptr);
        fprintf(tee,"\n");

	fprintf(tee ,"Please report all errors to\n");
	fprintf(tee ,"markus@pristovsek.de\n");
    }
    va_end(argptr);
}


/**
 * writes an error into the log, aborts the program.
 * @author Hj. Malthaner
 */
void log_t::fatal(const char *who, const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);

    if( log ) {                         /* nur loggen wenn schon ein log */
	fprintf(log ,"FATAL ERROR: %s:\t",who);      /* geoeffnet worden ist */
        vfprintf(log, format, argptr);
        fprintf(log,"\n");
        fprintf(log,"Aborting program execution ...\n\n");


	fprintf(log ,"Please report all fatal errors to\n");
	fprintf(log ,"markus@pristovsek.de\n");


        if( force_flush ) {
            fflush(log);
        }
    }

    if( tee ) {                         /* nur loggen wenn schon ein log */
	fprintf(tee, "FATAL ERROR: %s:\t",who);      /* geoeffnet worden ist */
        vfprintf(tee, format, argptr);
        fprintf(tee,"\n");
        fprintf(tee,"Aborting program execution ...\n\n");

	fprintf(tee ,"Please report all fatal errors to\n");
	fprintf(tee ,"markus@pristovsek.de\n");
    }

    va_end(argptr);

    abort();
}

log_t::log_t(const char *logfilename,
	     bool force_flush,
	     bool log_debug)      /* eine logdatei anlegen */
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


    message("log_t::log_t","Starting logging to %s", logfilename);
}


void log_t::close()
{
    message("log_t::~log_t","stop logging, closing log file");

    if( log ) {
        fclose(log);
        log = NULL;
    }
}


log_t::~log_t()                         /* die logdatei schliessen */
{
    if( log ) {
	close();
    }
}
