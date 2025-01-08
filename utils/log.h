/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef UTILS_LOG_H
#define UTILS_LOG_H


#include <stdarg.h>
#include <stdio.h>
#include <string>
#include "../simtypes.h"


/**
 * Logging facility
 */
class log_t
{
public:
	enum level_t
	{
		LEVEL_FATAL = 0,
		LEVEL_ERROR = 1,
		LEVEL_WARN  = 2,
		LEVEL_MSG   = 3,
		LEVEL_DEBUG = 4
	};

private:
	/**
	 * Primary log file.
	 */
	FILE *log;

	/**
	 * Secondary log file, currently fixed to stderr
	 */
	FILE * tee;

	bool force_flush;

	/**
	 * Logging level - include debug messages ?
	 */
	bool log_debug;

	/**
	 * Syslog tag
	 */
	const char* tag;

#ifdef SYSLOG
	/**
	 * Log to syslog?
	 */
	bool syslog;
#endif

	std::string doublettes;

public:
	/**
	 * writes a debug message into the log.
	 */
	void debug(const char *who, const char *format, ...);

	/**
	 * writes a message into the log.
	 */
	void message(const char *who, const char *format, ...);

	/**
	 * writes a warning into the log.
	 */
	void warning(const char *who, const char *format, ...);

	/* special error handling for double objects */
	void doubled( const char *what, const char *name );
	bool had_overlaid() { return !doublettes.empty(); }
	void clear_overlaid() { doublettes.clear(); }
	std::string get_overlaid() { return doublettes; }

	/**
	 * writes an error into the log.
	 */
	void error(const char *who, const char *format, ...);

	/**
	 * writes an error into the log, aborts the program.
	 */
	void NORETURN fatal(const char* who, const char* format, ...);

	/**
	 * writes an error into the log, aborts the program, use own messsgae.
	 */
	void NORETURN custom_fatal(char* error_str);

	/**
	 * writes an error into the log, aborts the program.
	 */
	void NORETURN custom_fatal(const char* error_str);

	/**
	 * writes to log using va_lists
	 */
	void vmessage(const char *what, const char *who, const char *format,  va_list args );


	void close();


	/**
	 * @param syslogtag only used with syslog
	 */
	log_t(const char* logname, bool force_flush, bool log_debug, bool log_console, const char* greeting, const char* syslogtag=NULL);

	~log_t();
};

#if defined(MAKEOBJ)
extern log_t::level_t debuglevel;
#endif

#endif
