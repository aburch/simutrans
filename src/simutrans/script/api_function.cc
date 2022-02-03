/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api_function.h"
#include <stdio.h>

#include "../sys/simsys.h"

#include "../dataobj/environment.h"

/**
 * Auxiliary function to register function in table/class at stack top
 */
void script_api::register_function(HSQUIRRELVM vm, SQFUNCTION funcptr, const char *name, int nparamcheck, const char* typemask, bool staticmethod)
{
	sq_pushstring(vm, name, -1);
	sq_newclosure(vm, funcptr, 0); //create a new function
	sq_setnativeclosurename(vm, -1, name);
	sq_setparamscheck(vm, nparamcheck, typemask);
	sq_newslot(vm, -3, staticmethod);
}

static FILE* file = NULL;
static const char* suffix = NULL;

void script_api::start_squirrel_type_logging(const char* s)
{
	if (env_t::verbose_debug < 2) {
		return;
	}
	suffix = s;
	cbuffer_t buf;
	buf.printf("squirrel_types_%s.awk", suffix);
	file = dr_fopen(buf, "w");
	if (file) {
		fprintf(file, "#\n");
		fprintf(file, "# This file is part of the Simutrans project under the Artistic License.\n");
		fprintf(file, "# (see LICENSE.txt)\n");
		fprintf(file, "#\n");
		fprintf(file, "# file used to generate doxygen documentation of squirrel API\n");
		fprintf(file, "# needs to be copied to trunk/script/api\n");
		fprintf(file, "BEGIN {\n");
	}
}

void script_api::end_squirrel_type_logging()
{
	if (file) {
		fprintf(file, "}\n");
		fclose(file);
		file = NULL;
		suffix = NULL;
	}
}

static plainstring current_class;

void script_api::set_squirrel_type_class(const char* classname)
{
	current_class = classname;
}

void script_api::log_squirrel_type(std::string classname, const char* name, std::string squirrel_type)
{
	if (file) {
		fprintf(file, "\texport_types_%s[\"%s::%s\"] = \"%s\"\n",
			suffix ? suffix : "",
			classname.empty() ? current_class.c_str() : classname.c_str(),
			name,
			squirrel_type.c_str()
		);
	}
}
