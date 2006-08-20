#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include "debug_helper.h"
#include "../simdebug.h"

static char tmp[2048];

void out_error(const char *who, const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);

	if(dbg) {
		vsprintf( tmp, format, argptr );
		dbg->error(who,tmp);
		return;
	}
	fprintf(stderr, "Error: %s:\t",who);
	vfprintf(stderr, format, argptr);
	fprintf(stderr,"\n");
}

void out_warning(const char *who, const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);

  	if(dbg) {
		vsprintf( tmp, format, argptr );
		dbg->warning(who,tmp);
		return;
	}
	fprintf(stderr, "Warning: %s:\t",who);
	vfprintf(stderr, format, argptr);
	fprintf(stderr,"\n");
}

void out_message(const char *who, const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	if(dbg) {
		vsprintf( tmp, format, argptr );
		dbg->message(who,tmp);
		return;
	}
	fprintf(stderr, "Message: %s:\t",who);
	vfprintf(stderr, format, argptr);
	fprintf(stderr,"\n");
}

/*
// generate a division be zero error
void trap()
{
#ifdef DEBUG
	int i=32, j;
	for( j=1; j>=0;  j-- )
	{
		i += (i/j);
		printf("%*d",i);
	}
#else
	assert(0);
#endif
}
*/
