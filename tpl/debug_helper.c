#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include "debug_helper.h"


void c_out_error(const char *who, const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);

    fprintf(stderr, "Error: %s:\t",who);
    vfprintf(stderr, format, argptr);
    fprintf(stderr,"\n");
}

void c_out_warning(const char *who, const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);

    fprintf(stderr, "Warning: %s:\t",who);
    vfprintf(stderr, format, argptr);
    fprintf(stderr,"\n");
}

void c_out_message(const char *who, const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);

    fprintf(stderr, "Message: %s:\t",who);
    vfprintf(stderr, format, argptr);
    fprintf(stderr,"\n");
}

// generate a division be zero error
void trap()
{
#ifdef DEBUG
	int i=32, j;
	for( j=1; j>=0;  j-- )
	{
		i += (i/(j-1));
		printf("%d",i);
	}
#else
	assert(0);
#endif
}
