/*
 * log.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef tests_log_h
#define tests_log_h

#include <stdio.h>

/**
 * Logging facility
 * @author Hj. Malthaner
 */
class log_t
{
private:
    /**
     * Primary log file.
     * @author Hj. Malthaner
     */
    FILE *log;


    /**
     * Secondary log file, currently fixed to stderr
     * @author Hj. Malthaner
     */
    FILE * tee;

    bool force_flush;

    /**
     * Logging level - include debug messages ?
     * @author Hj. Malthaner
     */
    bool log_debug;

public:


    /**
     * writes a debug message into the log.
     * @author Hj. Malthaner
     */
    void debug(const char *who, const char *format, ...);


    /**
     * writes a message into the log.
     * @author Hj. Malthaner
     */
    void message(const char *who, const char *format, ...);


    /**
     * writes a warning into the log.
     * @author Hj. Malthaner
     */
    void warning(const char *who, const char *format, ...);


    /**
     * writes an error into the log.
     * @author Hj. Malthaner
     */
    void error(const char *who, const char *format, ...);


    /**
     * writes an error into the log, aborts the program.
     * @author Hj. Malthaner
     */
    void fatal(const char *who, const char *format, ...);


    void close();


    log_t(const char *logname, bool force_flush, bool log_debug);
    ~log_t();
};


#endif
