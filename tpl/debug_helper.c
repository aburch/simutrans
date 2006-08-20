#include "stdio.h"
#include "stdarg.h"

#include "debug_helper.h"


void out_error(const char *who, const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);

    fprintf(stderr, "Error: %s:\t",who);
    vfprintf(stderr, format, argptr);
    fprintf(stderr,"\n");
}

void out_warning(const char *who, const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);

    fprintf(stderr, "Warning: %s:\t",who);
    vfprintf(stderr, format, argptr);
    fprintf(stderr,"\n");
}

void out_message(const char *who, const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);

    fprintf(stderr, "Message: %s:\t",who);
    vfprintf(stderr, format, argptr);
    fprintf(stderr,"\n");
}
