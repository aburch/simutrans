/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
void *sq_vm_malloc(SQUnsignedInteger size){	return malloc(size); }

void *sq_vm_realloc(void *p, SQUnsignedInteger, SQUnsignedInteger size){ return realloc(p, size); }

void sq_vm_free(void *p, SQUnsignedInteger){	free(p); }
