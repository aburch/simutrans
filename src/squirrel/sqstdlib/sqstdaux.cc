/* see copyright notice in squirrel.h */
#include "../squirrel.h"
#include "../sqstdaux.h"
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

void sqstd_printcallstack(HSQUIRRELVM v)
{
	SQPRINTFUNCTION pf = sq_geterrorfunc(v);
	if(pf) {
		SQStackInfos si;
		SQInteger i;
		SQFloat f;
		const SQChar *s;
		SQInteger level=1; //1 is to skip this function that is level 0
		const SQChar *name=0;
		SQInteger seq=0;
		pf(v,_SC("\nCALLSTACK\n"));
		while(SQ_SUCCEEDED(sq_stackinfos(v,level,&si)))
		{
			const SQChar *fn=_SC("unknown");
			const SQChar *src=_SC("unknown");
			if(si.funcname)fn=si.funcname;
			if(si.source)src=si.source;
			pf(v,_SC("<em>* FUNCTION [%s()] <br>* %s"), fn,src);
			if (si.line > 0) {
				pf(v,_SC("* line [%d]"),si.line);
			}
			pf(v,_SC("</em>\n"));

			pf(v,_SC("- - LOCALS\n"));

			seq=0;
			while((name = sq_getlocal(v,level,seq)))
			{
				seq++;
				switch(sq_gettype(v,-1))
				{
				case OT_NULL:
					pf(v,_SC("- - - [%s] NULL\n"),name);
					break;
				case OT_INTEGER:
					sq_getinteger(v,-1,&i);
					pf(v,_SC("- - - [%s] %d\n"),name,i);
					break;
				case OT_FLOAT:
					sq_getfloat(v,-1,&f);
					pf(v,_SC("- - - [%s] %.14g\n"),name,f);
					break;
				case OT_USERPOINTER:
					pf(v,_SC("- - - [%s] USERPOINTER\n"),name);
					break;
				case OT_STRING:
					sq_getstring(v,-1,&s);
					pf(v,_SC("- - - [%s] \"%s\"\n"),name,s);
					break;
				case OT_TABLE:
					pf(v,_SC("- - - [%s] TABLE (%d entries)\n"),name, sq_getsize(v, -1));
					break;
				case OT_ARRAY:
					pf(v,_SC("- - - [%s] ARRAY (%d entries)\n"),name, sq_getsize(v, -1));
					break;
				case OT_CLOSURE:
					pf(v,_SC("- - - [%s] CLOSURE\n"),name);
					break;
				case OT_NATIVECLOSURE:
					pf(v,_SC("- - - [%s] NATIVECLOSURE\n"),name);
					break;
				case OT_GENERATOR:
					pf(v,_SC("- - - [%s] GENERATOR\n"),name);
					break;
				case OT_USERDATA:
					pf(v,_SC("- - - [%s] USERDATA\n"),name);
					break;
				case OT_THREAD:
					pf(v,_SC("- - - [%s] THREAD\n"),name);
					break;
				case OT_CLASS:
					pf(v,_SC("- - - [%s] CLASS\n"),name);
					break;
				case OT_INSTANCE:
				{
					// try to obtain class name
					sq_getclass(v, -1);
					sq_pushnull(v);
					if (SQ_SUCCEEDED(sq_getattributes(v, -2))) {
						// stack: instance, class, attributes
						sq_pushstring(v, "classname", -1);
						if (SQ_SUCCEEDED(sq_get(v, -2))) {
							const char* cn;
							if (SQ_SUCCEEDED(sq_getstring(v, -1, &cn))) {
								pf(v,_SC("- - - [%s] INSTANCE(%s)\n"),name, cn);
								sq_pop(v, 3);
								break;
							}
						}
						sq_poptop(v);
					}
					sq_poptop(v);
					pf(v,_SC("- - - [%s] INSTANCE\n"),name);
					break;
				}
				case OT_WEAKREF:
					pf(v,_SC("- - - [%s] WEAKREF\n"),name);
					break;
				case OT_BOOL:{
					SQBool bval;
					sq_getbool(v,-1,&bval);
					pf(v,_SC("- - - [%s] %s\n"),name,bval == SQTrue ? _SC("true"):_SC("false"));
							 }
					break;
				default: assert(0); break;
				}
				sq_pop(v,1);
			}
			level++;
		}
	}
}

static SQInteger _sqstd_aux_printerror(HSQUIRRELVM v)
{
	SQPRINTFUNCTION pf = sq_geterrorfunc(v);
	if(pf) {
		pf(v,_SC("<error>"));
		const SQChar *sErr = 0;
		if(sq_gettop(v)>=1) {
			if(SQ_SUCCEEDED(sq_getstring(v,2,&sErr)))	{
				pf(v,_SC("\n<st>Error: [%s]</st>\n"),sErr);
			}
			else{
				pf(v,_SC("\n<st>Error: [unknown]</st>\n"));
			}
			sqstd_printcallstack(v);
		}
		pf(v,_SC("</error>"));
	}
	return 0;
}

void _sqstd_compiler_error(HSQUIRRELVM v,const SQChar *sErr,const SQChar *sSource,SQInteger line,SQInteger column)
{
	SQPRINTFUNCTION pf = sq_geterrorfunc(v);
	if(pf) {
		pf(v,_SC("<error>"));
		pf(v,_SC("%s line = (%d) column = (%d) : error %s\n"),sSource,line,column,sErr);
		pf(v,_SC("</error>"));
	}
}

void sqstd_seterrorhandlers(HSQUIRRELVM v)
{
	sq_setcompilererrorhandler(v,_sqstd_compiler_error);
	sq_newclosure(v,_sqstd_aux_printerror,0);
	sq_seterrorhandler(v);
}

SQRESULT sqstd_throwerrorf(HSQUIRRELVM v,const SQChar *err,...)
{
	SQInteger n=256;
	va_list args;
begin:
	va_start(args,err);
	SQChar *b=sq_getscratchpad(v,n);
	SQInteger r=scvsprintf(b,n,err,args);
	va_end(args);
	if (r>=n) {
		n=r+1;//required+null
		goto begin;
	} else if (r<0) {
		return sq_throwerror(v,_SC("@failed to generate formatted error message"));
	} else {
		return sq_throwerror(v,b);
	}
}
